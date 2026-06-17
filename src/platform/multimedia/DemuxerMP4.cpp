/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "StarfishConfig.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/threading/Mutex.h"
#include "platform/multimedia/Demuxer.h"
#include "platform/multimedia/DemuxerSource.h"
#include "platform/multimedia/PacketGenerator.h"

#include "mp4.h"
#include "atoms.h"
#include "MP4.BinaryStream.h"
#include "MP4.Parser.h"

#ifdef STARFISH_MEDIAPLAYER_DEBUG
#define DEMUXERMP4_LOG(STR, ...) \
    STARFISH_LOG_INFO(           \
        "[DemuxerMP4] "          \
        "" STR,                  \
        ##__VA_ARGS__);
#else
#define DEMUXERMP4_LOG(...)
#endif

class MP4BinaryStreamAdapter : public MP4::BinaryStream {
public:
    static const size_t kCacheSize = 16 * 1024;
    static const size_t kBypassLen = kCacheSize / 2;

    MP4BinaryStreamAdapter(Starfish::DemuxerSource* source,
                           std::vector<uint8_t>* cache)
        : m_source(source)
        , m_cache(cache)
        , m_windowOffset(0)
        , m_windowLen(0)
    {
    }
    virtual void ignore(std::streamsize n = 1)
    {
        m_source->onSeek(n, Starfish::DemuxerSource::SeekWhenceCurrent);
    }
    virtual void read(char* s, std::streamsize n)
    {
        if ((size_t)n >= kBypassLen) {
            readDirect(s, n);
            return;
        }
        size_t p = (size_t)m_source->onSeek(
            0, Starfish::DemuxerSource::SeekWhenceCurrent);
        if (p >= m_windowOffset &&
            p + (size_t)n <= m_windowOffset + m_windowLen) {
            memcpy(s, m_cache->data() + (p - m_windowOffset), (size_t)n);
            m_source->onSeek(n, Starfish::DemuxerSource::SeekWhenceCurrent);
            return;
        }
        // Miss: refill at p. onRead errors on any short read, so request
        // exactly what the source can provide.
        size_t total = (size_t)m_source->onSeek(
            0, Starfish::DemuxerSource::SeekWhenceLookSize);
        size_t avail = total > p ? total - p : 0;
        if (avail < (size_t)n) {
            readDirect(s, n);
            return;
        }
        size_t want = avail < kCacheSize ? avail : kCacheSize;
        if (m_cache->size() < kCacheSize) {
            m_cache->resize(kCacheSize);
        }
        size_t got = 0;
        int error = 0;
        m_source->onRead(want, got, error, m_cache->data());
        if (error || got != want) {
            m_windowLen = 0;
            m_source->onSeek(p, Starfish::DemuxerSource::SeekWhenceSet);
            readDirect(s, n);
            return;
        }
        m_windowOffset = p;
        m_windowLen = got;
        memcpy(s, m_cache->data(), (size_t)n);
        m_source->onSeek(p + n, Starfish::DemuxerSource::SeekWhenceSet);
    }

    virtual bool eof() const
    {
        return m_source->onSeek(0,
                                Starfish::DemuxerSource::SeekWhenceLookSize) !=
               m_source->onSeek(0, Starfish::DemuxerSource::SeekWhenceCurrent);
    }

    virtual size_t pos()
    {
        return m_source->onSeek(0, Starfish::DemuxerSource::SeekWhenceCurrent);
    }

private:
    void readDirect(char* s, std::streamsize n)
    {
        size_t sizeWantToRead = n;
        size_t out;
        int error = 0;
        m_source->onRead(n, out, error, (uint8_t*)s);
        STARFISH_ASSERT(error == 0);
    }
    virtual void get(char& c)
    {
        read(&c, 1);
    }
    virtual void get(char* s, std::streamsize n)
    {
        read(s, n);
    }
    Starfish::DemuxerSource* m_source;
    std::vector<uint8_t>* m_cache;
    size_t m_windowOffset;
    size_t m_windowLen;
};

namespace Starfish {

static uint32_t readBigEndianUnsignedInteger(DemuxerSource* source, int& error)
{
    error = 0;
    uint8_t c[4];
    uint32_t n = 0;

    size_t t1;
    int err;
    source->onRead(4, t1, err, c);
    if (t1 != 4) {
        error = -1;
        source->onSeek(-t1, DemuxerSource::SeekWhenceCurrent);
    } else {
        n = (uint32_t)c[0] << 24 | (uint32_t)c[1] << 16 | (uint32_t)c[2] << 8 |
            (uint32_t)c[3];
    }

    return n;
}

static bool parseMP4(DemuxerSource* source, bool findPacket,
                     std::vector<uint8_t>* readCache,
                     const std::function<bool(MP4::Atom* atom)>& fn)
{
    uint32_t length;
    uint64_t dataLength;
    size_t lastUnpairedMoofPos = SIZE_MAX;
    char type[5];
    MP4BinaryStreamAdapter src(source, readCache);
    memset(type, 0, 5);

#ifdef STARFISH_MEDIAPLAYER_DEBUG
#define PRINT_STOP_REASON(...)                                               \
    DEMUXERMP4_LOG("Stop parsing at %d and rewind to %d (end:%d)",           \
                   (int)source->onSeek(0, DemuxerSource::SeekWhenceCurrent), \
                   (int)orgPos, (int)maxPos);                                \
    DEMUXERMP4_LOG("> Reason: ");                                            \
    STARFISH_LOG_INFO(__VA_ARGS__);
#else
#define PRINT_STOP_REASON(...)
#endif

    int64_t maxPos = source->onSeek(0, DemuxerSource::SeekWhenceLookSize);
    while (source->onSeek(0, DemuxerSource::SeekWhenceCurrent) < maxPos) {
        int err;
        size_t orgPos = source->onSeek(0, DemuxerSource::SeekWhenceCurrent);
        size_t orgLength = length = readBigEndianUnsignedInteger(source, err);
        if (err < 0) {
            PRINT_STOP_REASON("4byte requried to read \"DataLength\"");
            source->onSeek(orgPos, DemuxerSource::SeekWhenceSet);
            break;
        }
        dataLength = 0;

        size_t t1;
        source->onRead(4, t1, err, (uint8_t*)type);
        if (err < 0) {
            PRINT_STOP_REASON("4byte requried to read \"DataType\"");
            source->onSeek(orgPos, DemuxerSource::SeekWhenceSet);
            break;
        }

        if (length == 1) {
            dataLength = readBigEndianUnsignedInteger(source, err) - 16;
            if (err < 0) {
                PRINT_STOP_REASON(
                    "4byte requried to read \"DataLength\" (case 2)");
                source->onSeek(orgPos, DemuxerSource::SeekWhenceSet);
                break;
            }
        } else {
            dataLength = length - 8;
        }

        if (source->onSeek(0, DemuxerSource::SeekWhenceCurrent) +
                (int64_t)dataLength >
            maxPos) {
            PRINT_STOP_REASON("%dbyte requried to read \"Data\"",
                              (int)dataLength);
            source->onSeek(orgPos, DemuxerSource::SeekWhenceSet);
            break;
        }

        uint32_t typeInt = MP4_PARSER_DEFINE_TYPE_STRING(type);
        if (typeInt == MP4_PARSER_DEFINE_TYPE_STRING("meta")) {
            source->onSeek(orgLength, DemuxerSource::SeekWhenceCurrent);
            continue;
        }

        if (typeInt == MP4_PARSER_DEFINE_TYPE_STRING("moov")) {
            if (!findPacket) {
                maxPos = orgPos + orgLength;
                // DEMUXERMP4_LOG("Found movv. (endpos %d)", (int)maxPos);
            }
        }

        if (typeInt == MP4_PARSER_DEFINE_TYPE_STRING("moof")) {
            if (!findPacket) {
                // PRINT_STOP_REASON("Found MOOF while parsing stream info");
                source->onSeek(orgPos, DemuxerSource::SeekWhenceSet);
                break;
            }
            lastUnpairedMoofPos = orgPos;
        }

        if (typeInt == MP4_PARSER_DEFINE_TYPE_STRING("mdat")) {
            if (!findPacket) {
                // PRINT_STOP_REASON("Found MDAT while parsing stream info");
                source->onSeek(orgPos, DemuxerSource::SeekWhenceSet);
                break;
            }
            lastUnpairedMoofPos = SIZE_MAX;
        }

        MP4::Atom* atom = MP4::atomFactory(typeInt);
        ((MP4::DataAtom*)atom)->processData(&src, dataLength);
        bool success = fn(atom);
        delete atom;
        if (!success) {
            return false;
        }
    }
    if (lastUnpairedMoofPos != SIZE_MAX) {
        DEMUXERMP4_LOG("Rewind to last unpaired MOOF position %d (end:%d)",
                       (int)lastUnpairedMoofPos, (int)maxPos);
        source->onSeek(lastUnpairedMoofPos, DemuxerSource::SeekWhenceSet);
    }
    return true;
#undef PRINT_STOP_REASON
}

class StreamInfoMP4 : public StreamInfo {
public:
    StreamInfoMP4()
        : StreamInfo()
        , m_mediaTime(0)
        , m_trexSampleSize(0)
        , m_trexSampleDuration(0)
    {
    }
    uint64_t mediaTime()
    {
        return m_mediaTime;
    }
    void setMediaTime(uint64_t value)
    {
        m_mediaTime = value;
    }
    void setTrexData(MP4::TREX* trex)
    {
        m_trexSampleSize = trex->sample_size;
        m_trexSampleDuration = trex->sample_duration;
    }
    size_t trexSampleSize()
    {
        return m_trexSampleSize;
    }
    size_t trexSampleDuration()
    {
        return m_trexSampleDuration;
    }

protected:
    uint64_t m_mediaTime;
    size_t m_trexSampleSize;
    size_t m_trexSampleDuration;
};

class DemuxerMP4 : public Demuxer {
public:
    DemuxerMP4()
        : Demuxer()
        , m_demuxingMutex(new Mutex())
        , m_packetGenerator(new MP4PacketGenerator())
    {
        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                DEMUXERMP4_LOG("DemuxerMP4::~DemuxerMP4");
                DemuxerMP4* self = (DemuxerMP4*)obj;
                self->m_streamInfo.clear();
                std::vector<uint8_t>().swap(self->m_readCache);
            },
            NULL, NULL, NULL);
    }

    virtual bool findStreamInfo(DemuxerSource* source, String* formatHint);
    virtual bool findStreamPacket(DemuxerSource* source);

    std::unordered_map<size_t, StreamInfoMP4> m_streamInfo;
    Mutex* m_demuxingMutex;
    MP4PacketGenerator* m_packetGenerator;
    // Read-through window storage for MP4BinaryStreamAdapter: absorbs the
    // 4-byte TRUN/box-header reads in parseMP4. Grow-once to 16KB, reused
    // across calls; window state lives in the stack-local adapter. Released
    // by the GC finalizer (DemuxerWebM::m_readCache pattern).
    std::vector<uint8_t> m_readCache;
};

static StreamInfoMP4* handleTKHD(MP4::TKHD* tkhd,
                                 std::unordered_map<size_t, StreamInfoMP4>& map)
{
    if (tkhd->track_id < 1) {
        DEMUXERMP4_LOG("Unexpected structure of MP4");
        DEMUXERMP4_LOG("> TRAK(TKHD): Found wrong track number");
        return nullptr;
    }
    size_t streamIndex = tkhd->track_id - 1;
#ifdef STARFISH_MEDIAPLAYER_DEBUG
    auto exist = map.find(streamIndex);
    if (exist != map.end()) {
        DEMUXERMP4_LOG("TRAK(TKHD): Replace previous stream at index %d",
                       (int)streamIndex);
    }
#endif
    auto& info = map[streamIndex];
    if (tkhd->track_width && tkhd->track_height) {
        DEMUXERMP4_LOG("TRAK(TKHD): Found new VIDEO stream");
        info.setType(StreamTypeVideo);
        info.setVideoWidth(tkhd->track_width);
        info.setVideoHeight(tkhd->track_height);
        info.setVideoHasFramerate(false);
    } else {
        DEMUXERMP4_LOG("TRAK(TKHD): Found new AUDIO stream");
        info.setType(StreamTypeAudio);
    }
    info.setRawDuration(tkhd->duration);
    info.setStreamIndex(tkhd->track_id - 1);
    return &info;
}

static bool handleMDHD(MP4::MDHD* mdhd, StreamInfoMP4* stream)
{
    if (!stream) {
        DEMUXERMP4_LOG("Unexpected structure of MP4");
        DEMUXERMP4_LOG("> MDHD: Need TRAK before MDHD");
        return false;
    }
    stream->setTimescale(mdhd->_timeScale);
    return true;
}

static bool handleELST(MP4::ELST* elst, StreamInfoMP4* stream)
{
    if (!stream) {
        DEMUXERMP4_LOG("Unexpected structure of MP4");
        DEMUXERMP4_LOG("> ELST: Need TRAK before ELST");
        return false;
    }
    uint64_t mediaTime = 0;
    if (elst->edit_entries.size() > 0 && elst->edit_entries[0].media_time > 0) {
        mediaTime = (uint64_t)elst->edit_entries[0].media_time;
    }
    stream->setMediaTime(mediaTime);
    return true;
}

static bool handleMetAVC(StreamInfoMP4* stream, MP4PacketGenerator* generator)
{
    if (!stream || !stream->isVideo()) {
        DEMUXERMP4_LOG("Unexpected structure of MP4");
        DEMUXERMP4_LOG("> AVC1/AVC3");
        return false;
    }
    if (!generator->isCodec(MediaCodecUnknown) &&
        !generator->isCodec(MediaCodecVideoH264)) {
        STARFISH_UNSUPPORTED("Media: unsupported codec");
        DEMUXERMP4_LOG("Unexpected structure of MP4");
        DEMUXERMP4_LOG("> AVC1/AVC3: OTHER -> MP4H264");
        return false;
    }
    generator->setCodec(MediaCodecVideoH264);
    stream->setCodec(MediaCodecVideoH264);
    return true;
}

static bool handleMetHVC(StreamInfoMP4* stream, MP4PacketGenerator* generator)
{
    if (!stream || !stream->isVideo()) {
        DEMUXERMP4_LOG("Unexpected structure of MP4");
        DEMUXERMP4_LOG("> HVC1/HEV1");
        return false;
    }
    if (generator->isCodec(MediaCodecUnknown)) {
        generator->setCodec(MediaCodecVideoHEVC);
    } else if (!generator->isCodec(MediaCodecVideoHEVC)) {
        STARFISH_UNSUPPORTED("Media: unsupported codec");
        DEMUXERMP4_LOG("Unexpected structure of MP4");
        DEMUXERMP4_LOG("> HVC1/HEV1: OTHER -> MP4HEVC");
        return false;
    }
    stream->setCodec(MediaCodecVideoHEVC);
    return true;
}

static bool handleMetAV1(StreamInfoMP4* stream, MP4PacketGenerator* generator)
{
    if (!stream || !stream->isVideo()) {
        DEMUXERMP4_LOG("Unexpected structure of MP4");
        DEMUXERMP4_LOG("> AV01");
        return false;
    }
    if (generator->isCodec(MediaCodecUnknown)) {
        generator->setCodec(MediaCodecVideoAV1);
    } else if (!generator->isCodec(MediaCodecVideoAV1)) {
        STARFISH_UNSUPPORTED("Media: unsupported codec");
        DEMUXERMP4_LOG("Unexpected structure of MP4");
        DEMUXERMP4_LOG("> AV01: OTHER -> AV1");
        return false;
    }
    stream->setCodec(MediaCodecVideoAV1);
    return true;
}

static bool handleAVCC(MP4::AVCC* avcc, StreamInfoMP4* stream,
                       MP4PacketGenerator* generator)
{
    if (!stream || !stream->isVideo()) {
        DEMUXERMP4_LOG("Unexpected structure of MP4");
        DEMUXERMP4_LOG("> AVCC");
        return false;
    }
    if (!generator->isCodec(MediaCodecVideoH264)) {
        DEMUXERMP4_LOG("Unexpected structure of MP4");
        DEMUXERMP4_LOG("> AVCC: Invalid parent");
        return false;
    }

    // NOTE MediaPlayer does not need extradata for MP4 Video,
    //      because it'd be appended to every keyframes (annexB)
    STARFISH_ASSERT(stream->m_extraData.size() == 0);

    generator->setAVCNALSizeLength(avcc->nal_length);
    generator->setAVCExtraData(avcc->spsVector, avcc->ppsVector);
    return true;
}

static bool handleMP4A(MP4::MP4A* mp4a, StreamInfoMP4* stream)
{
    if (!stream || !stream->isAudio()) {
        DEMUXERMP4_LOG("Unexpected structure of MP4");
        DEMUXERMP4_LOG("> MP4A");
        return false;
    }
    stream->setAudioSampleRate(mp4a->sample_rate);
    stream->setAudioChannels(mp4a->channels);
    return true;
}

static bool handleESDS(MP4::ESDS* esds, StreamInfoMP4* stream)
{
    if (!stream || !stream->isAudio()) {
        DEMUXERMP4_LOG("Unexpected structure of MP4");
        DEMUXERMP4_LOG("> ESDS");
        return false;
    }
    if (stream->m_extraData.size() > 0) {
        std::vector<uint8_t>().swap(stream->m_extraData);
    }
    if (esds->decoder_config_size) {
        stream->m_extraData = std::move(esds->decoder_config);
    }
    // TODO Determine codec elaborately
    stream->setCodec(MediaCodecAudioAAC);
    return true;
}

bool DemuxerMP4::findStreamInfo(DemuxerSource* source, String* formatHint)
{
    Locker<Mutex> lock(*m_demuxingMutex);

    int64_t before = source->onSeek(0, DemuxerSource::SeekWhenceCurrent);
    std::unordered_set<size_t> detectedStreamIndice;
    StreamInfoMP4* currentStream = nullptr;
    MP4::TREX* trexData = nullptr;

    bool result =
        parseMP4(source, false, &m_readCache, [&](MP4::Atom* atom) -> bool {
            uint32_t type = atom->getType();
            switch (type) {
            case MP4_PARSER_DEFINE_TYPE_STRING("tkhd"): {
                // Found new stream
                currentStream = handleTKHD((MP4::TKHD*)atom, m_streamInfo);
                if (!currentStream) {
                    return false;
                }
                if (trexData) {
                    currentStream->setTrexData(trexData);
                    delete trexData;
                    trexData = nullptr;
                }
                detectedStreamIndice.insert(currentStream->streamIndex());
                return true;
            }
            case MP4_PARSER_DEFINE_TYPE_STRING("mdhd"): {
                return handleMDHD((MP4::MDHD*)atom, currentStream);
            }
            case MP4_PARSER_DEFINE_TYPE_STRING("elst"): {
                return handleELST((MP4::ELST*)atom, currentStream);
            }
            case MP4_PARSER_DEFINE_TYPE_STRING("avc1"):
            case MP4_PARSER_DEFINE_TYPE_STRING("avc3"): {
                return handleMetAVC(currentStream, m_packetGenerator);
            }
            case MP4_PARSER_DEFINE_TYPE_STRING("hev1"):
            case MP4_PARSER_DEFINE_TYPE_STRING("hvc1"): {
                return handleMetHVC(currentStream, m_packetGenerator);
            }
            case MP4_PARSER_DEFINE_TYPE_STRING("av01"): {
                return handleMetAV1(currentStream, m_packetGenerator);
            }
            case MP4_PARSER_DEFINE_TYPE_STRING("mp4a"): {
                return handleMP4A((MP4::MP4A*)atom, currentStream);
            }
            case MP4_PARSER_DEFINE_TYPE_STRING("esds"): {
                return handleESDS((MP4::ESDS*)atom, currentStream);
            }
            case MP4_PARSER_DEFINE_TYPE_STRING("avcC"): {
                return handleAVCC((MP4::AVCC*)atom, currentStream,
                                  m_packetGenerator);
            }
            case MP4_PARSER_DEFINE_TYPE_STRING("trex"): {
                if (trexData) {
                    DEMUXERMP4_LOG("Unexpected structure of MP4");
                    DEMUXERMP4_LOG("> TREX");
                    return false;
                }
                // Copy trex data
                trexData = new MP4::TREX(*((MP4::TREX*)atom));
                return true;
            }
            default:
                break;
            }
            return true;
        });
    if (trexData) {
        delete trexData;
    }
    if (result) {
        for (const size_t i : detectedStreamIndice) {
            StreamInfoMP4& item = m_streamInfo[i];
            for (size_t j = 0; j < m_demuxerClients.size(); j++) {
                m_demuxerClients[j]->onDetectStream(item);
            }
        }
    } else {
        source->onSeek(before, DemuxerSource::SeekWhenceSet);
    }
    return result;
}

struct SegmentParsingInfo {
    uint64_t m_DTS{ 0 };
    size_t m_streamIndex{ 0 };
    size_t m_defaultSampleSize{ 0 };
    size_t m_defaultSampleDuration{ 0 };
    bool m_hasDefaultSampleSize : 1;
    bool m_hasDefaultSampleDuration : 1;
    bool m_hasSampleSize : 1;
    bool m_hasSampleDuration : 1;
    bool m_hasSampleCTSOffset : 1;
    bool m_seenTFHD : 1;
    bool m_seenTFDT : 1;
    bool m_seenTRUN : 1;
    std::vector<MP4::TRUN::Sample> m_samples;
    SegmentParsingInfo()
        : m_hasDefaultSampleSize(false)
        , m_hasDefaultSampleDuration(false)
        , m_hasSampleSize(false)
        , m_hasSampleDuration(false)
        , m_hasSampleCTSOffset(false)
        , m_seenTFHD(false)
        , m_seenTFDT(false)
        , m_seenTRUN(false)
    {
    }
    void reset()
    {
        m_seenTFHD = m_seenTFDT = m_seenTRUN = false;
        std::vector<MP4::TRUN::Sample>().swap(m_samples);
    }
};

static bool handleTFHD(MP4::TFHD* tfhd, SegmentParsingInfo& parsingInfo)
{
    if (parsingInfo.m_seenTFHD) {
        DEMUXERMP4_LOG("Unexpected structure of MP4");
        DEMUXERMP4_LOG("> Duplicate TFHD");
        return false;
    }
    if (tfhd->track_id < 1) {
        DEMUXERMP4_LOG("Unexpected structure of MP4");
        DEMUXERMP4_LOG("> Wrong track number found in TFHD");
        return false;
    }
    parsingInfo.m_seenTFHD = true;
    parsingInfo.m_streamIndex = tfhd->track_id - 1;
    parsingInfo.m_hasDefaultSampleSize = tfhd->has_default_sample_size;
    parsingInfo.m_hasDefaultSampleDuration = tfhd->has_default_sample_duration;
    parsingInfo.m_defaultSampleSize = tfhd->default_sample_size;
    parsingInfo.m_defaultSampleDuration = tfhd->default_sample_duration;
    return true;
}

static bool handleTFDT(MP4::TFDT* tfdt, SegmentParsingInfo& parsingInfo)
{
    if (parsingInfo.m_seenTFDT) {
        DEMUXERMP4_LOG("Unexpected structure of MP4");
        DEMUXERMP4_LOG("> Duplicate TFDT");
        return false;
    }
    parsingInfo.m_seenTFDT = true;
    parsingInfo.m_DTS = tfdt->decodeTime;
    return true;
}

static bool handleTRUN(MP4::TRUN* trun, SegmentParsingInfo& parsingInfo)
{
    if (parsingInfo.m_seenTRUN) {
        DEMUXERMP4_LOG("Unexpected structure of MP4");
        DEMUXERMP4_LOG("> Duplicate TRUN");
        return false;
    }
    parsingInfo.m_seenTRUN = true;
    parsingInfo.m_hasSampleSize = trun->has_sample_size;
    parsingInfo.m_hasSampleDuration = trun->has_sample_duration;
    parsingInfo.m_hasSampleCTSOffset = trun->has_sample_composition_time_offset;
    parsingInfo.m_samples = std::move(trun->samples);
    return true;
}

#ifdef STARFISH_MEDIAPLAYER_DEBUG
static bool handleMFHD(MP4::MFHD* mfhd)
{
    DEMUXERMP4_LOG("Sequence %d data will follow", (int)mfhd->sequence_no);
    return true;
}
#endif

static size_t resolvePacketSize(StreamInfoMP4& stream,
                                SegmentParsingInfo& parsingInfo,
                                size_t sampleID)
{
    size_t packetSize = 0;
    if (parsingInfo.m_hasSampleSize) {
        packetSize = parsingInfo.m_samples[sampleID].size;
    } else if (parsingInfo.m_hasDefaultSampleSize) {
        packetSize = parsingInfo.m_defaultSampleSize;
    } else {
        packetSize = stream.trexSampleSize();
    }
    return packetSize;
}

static int64_t resolvePacketCTSOffset(StreamInfoMP4& stream,
                                      SegmentParsingInfo& parsingInfo,
                                      size_t sampleID)
{
    int64_t ctsOffset = 0;
    if (parsingInfo.m_hasSampleCTSOffset) {
        ctsOffset = parsingInfo.m_samples[sampleID].composition_time_offset;
    }
    return ctsOffset - stream.mediaTime();
}

static size_t resolvePacketDuration(StreamInfoMP4& stream,
                                    SegmentParsingInfo& parsingInfo,
                                    size_t sampleID)
{
    size_t duration = 0;
    if (parsingInfo.m_hasSampleDuration) {
        duration = parsingInfo.m_samples[sampleID].duration;
    } else if (parsingInfo.m_hasDefaultSampleDuration &&
               parsingInfo.m_defaultSampleDuration > 0) {
        duration = parsingInfo.m_defaultSampleDuration;
    } else {
        duration = stream.trexSampleDuration();
    }
    return duration;
}

bool DemuxerMP4::findStreamPacket(DemuxerSource* source)
{
    Locker<Mutex> lock(*m_demuxingMutex);
    SegmentParsingInfo parsingInfo;

    bool result =
        parseMP4(source, true, &m_readCache, [&](MP4::Atom* atom) -> bool {
            uint32_t type = atom->getType();
            switch (type) {
            case MP4_PARSER_DEFINE_TYPE_STRING("tfhd"): {
                return handleTFHD((MP4::TFHD*)atom, parsingInfo);
            }
            case MP4_PARSER_DEFINE_TYPE_STRING("tfdt"): {
                return handleTFDT((MP4::TFDT*)atom, parsingInfo);
            }
            case MP4_PARSER_DEFINE_TYPE_STRING("trun"): {
                return handleTRUN((MP4::TRUN*)atom, parsingInfo);
            }
#ifdef STARFISH_MEDIAPLAYER_DEBUG
            case MP4_PARSER_DEFINE_TYPE_STRING("mfhd"): {
                return handleMFHD((MP4::MFHD*)atom);
            }
#endif
            case MP4_PARSER_DEFINE_TYPE_STRING("mdat"): {
                if (!parsingInfo.m_seenTFHD || !parsingInfo.m_seenTFDT ||
                    !parsingInfo.m_seenTRUN) {
                    DEMUXERMP4_LOG("Unexpected structure of MP4");
                    DEMUXERMP4_LOG("> MDAT");
                    return false;
                }
                StreamInfoMP4& stream = m_streamInfo[parsingInfo.m_streamIndex];
                MP4::MDAT* mdat = (MP4::MDAT*)atom;
                int64_t before =
                    source->onSeek(0, DemuxerSource::SeekWhenceCurrent);
                int64_t start = (int64_t)mdat->dataPos;
                source->onSeek(start, DemuxerSource::SeekWhenceSet);

                size_t sizeSum = 0;
                uint64_t currentDTS = parsingInfo.m_DTS;
                for (size_t i = 0; i < parsingInfo.m_samples.size(); i++) {
                    size_t sampleSize =
                        resolvePacketSize(stream, parsingInfo, i);
                    if (sampleSize <= 0 || sizeSum + sampleSize > mdat->size) {
                        DEMUXERMP4_LOG("Unexpected structure of MP4");
                        DEMUXERMP4_LOG("> MDAT: Wrong sample size");
                        return false;
                    }
                    sizeSum += sampleSize;

                    // pts / dts
                    int64_t ctsOffset =
                        resolvePacketCTSOffset(stream, parsingInfo, i);
                    uint64_t pts =
                        stream.codedTimeToMilliseconds(currentDTS + ctsOffset);
                    uint64_t dts = stream.codedTimeToMilliseconds(currentDTS);

                    // duration
                    size_t duration =
                        resolvePacketDuration(stream, parsingInfo, i);
                    if (duration <= 0) {
                        DEMUXERMP4_LOG("Unexpected structure of MP4");
                        DEMUXERMP4_LOG("> MDAT: Wrong sample duration");
                        return false;
                    }
                    currentDTS += duration;
                    STARFISH_ASSERT(stream.codedTimeToMilliseconds(duration) <
                                    SIZE_MAX);
                    size_t durationMs =
                        (size_t)stream.codedTimeToMilliseconds(duration);

                    MediaPacket* packet = nullptr;
                    if (!m_packetGenerator->generate(source, sampleSize,
                                                     packet)) {
                        STARFISH_UNSUPPORTED("Media: unsupported codec");
                        DEMUXERMP4_LOG("Unexpected structure of MP4");
                        DEMUXERMP4_LOG("> MDAT");
                        return false;
                    }
                    packet->m_pts = pts;
                    packet->m_dts = dts;
                    packet->m_duration = durationMs;
                    for (size_t j = 0; j < m_demuxerClients.size(); j++) {
                        if (!m_demuxerClients[j]->onDetectPacket(
                                parsingInfo.m_streamIndex, packet)) {
                            DEMUXERMP4_LOG("Unexpected structure of MP4");
                            DEMUXERMP4_LOG(
                                "> Failed to append packet to groups");
                            DEMUXERMP4_LOG("> packet(pts: %d, dts:%d, dur:%d)",
                                           (int)packet->m_pts,
                                           (int)packet->m_dts,
                                           (int)packet->m_duration);
                            // Log first: destroy frees the whole combined
                            // block, packet fields die with it.
                            MediaPacket::destroy(packet);
                            return false;
                        }
                        // Ownership transferred to the client.
                        packet = nullptr;
                        break;
                    }
                    if (packet) {
                        MediaPacket::destroy(packet);
                    }
                }
                source->onSeek(before, DemuxerSource::SeekWhenceSet);
                parsingInfo.reset();
                break;
            }
            default:
                break;
            }
            return true;
        });
    return result;
}

Demuxer* Demuxer::createMP4Demuxer()
{
    return new DemuxerMP4();
}
} // namespace Starfish

#undef DEMUXERMP4_LOG
#endif /* STARFISH_ENABLE_MULTIMEDIA */
