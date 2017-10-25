/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "StarFishConfig.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/threading/Mutex.h"
#include "platform/multimedia/Demuxer.h"
#include "platform/multimedia/DemuxerSource.h"
#include "platform/multimedia/PacketGenerator.h"

#include "mp4.h"
#include "atoms.h"
#include "MP4.BinaryStream.h"
#include "MP4.Parser.h"

#define DEMUXERMP4_DEBUG
#ifdef DEMUXERMP4_DEBUG
#define DEMUXERMP4_LOG(...)             \
    STARFISH_LOG_INFO("[DemuxerMP4] "); \
    STARFISH_LOG_INFO(__VA_ARGS__);
#else
#define DEMUXERMP4_LOG(...)
#endif

class MP4BinaryStreamAdapter : public MP4::BinaryStream {
public:
    MP4BinaryStreamAdapter(StarFish::DemuxerSource* source)
        : m_source(source)
    {
    }
    virtual void ignore(std::streamsize n = 1)
    {
        m_source->onSeek(n, StarFish::DemuxerSource::SeekWhenceCurrent);
    }
    virtual void read(char* s, std::streamsize n)
    {
        size_t sizeWantToRead = n;
        size_t out;
        int error = 0;
        m_source->onRead(n, out, error, (uint8_t*)s);
        STARFISH_ASSERT(error == 0);
    }

    virtual bool eof() const
    {
        return m_source->onSeek(0,
                                StarFish::DemuxerSource::SeekWhenceLookSize) !=
               m_source->onSeek(0, StarFish::DemuxerSource::SeekWhenceCurrent);
    }

    virtual size_t pos()
    {
        return m_source->onSeek(0, StarFish::DemuxerSource::SeekWhenceCurrent);
    }

private:
    virtual void get(char& c)
    {
        read(&c, 1);
    }
    virtual void get(char* s, std::streamsize n)
    {
        read(s, n);
    }
    StarFish::DemuxerSource* m_source;
};

namespace StarFish {

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
                     const std::function<bool(MP4::Atom* atom)>& fn)
{
    uint32_t length;
    uint64_t dataLength;
    size_t lastUnpairedMoofPos = SIZE_MAX;
    char type[5];
    MP4BinaryStreamAdapter src(source);
    memset(type, 0, 5);

#ifdef DEMUXERMP4_DEBUG
#define PRINT_STOP_REASON(...)                                                 \
    STARFISH_LOG_INFO(                                                         \
        "[DemuxerMP4] Stop parsing at %d and rewind to %d (end:%d)\n",         \
        (int)source->onSeek(0, DemuxerSource::SeekWhenceCurrent), (int)orgPos, \
        (int)maxPos);                                                          \
    STARFISH_LOG_INFO("[DemuxerMP4] Reason: ");                                \
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
            PRINT_STOP_REASON("4byte requried to read \"DataLength\"\n");
            source->onSeek(orgPos, DemuxerSource::SeekWhenceSet);
            break;
        }
        dataLength = 0;

        size_t t1;
        source->onRead(4, t1, err, (uint8_t*)type);
        if (err < 0) {
            PRINT_STOP_REASON("4byte requried to read \"DataType\"\n");
            source->onSeek(orgPos, DemuxerSource::SeekWhenceSet);
            break;
        }

        if (length == 1) {
            dataLength = readBigEndianUnsignedInteger(source, err) - 16;
            if (err < 0) {
                PRINT_STOP_REASON(
                    "4byte requried to read \"DataLength\" (case 2)\n");
                source->onSeek(orgPos, DemuxerSource::SeekWhenceSet);
                break;
            }
        } else {
            dataLength = length - 8;
        }

        if (source->onSeek(0, DemuxerSource::SeekWhenceCurrent) +
                (int64_t)dataLength >
            maxPos) {
            PRINT_STOP_REASON("%dbyte requried to read \"Data\"\n",
                              (int)dataLength);
            source->onSeek(orgPos, DemuxerSource::SeekWhenceSet);
            break;
        }

        // STARFISH_LOG_INFO("found %x %s\n", (int)orgPos, type);
        uint32_t typeInt = MP4_PARSER_DEFINE_TYPE_STRING(type);
        if (typeInt == MP4_PARSER_DEFINE_TYPE_STRING("meta")) {
            source->onSeek(orgLength, DemuxerSource::SeekWhenceCurrent);
            continue;
        }

        if (typeInt == MP4_PARSER_DEFINE_TYPE_STRING("moov")) {
            if (!findPacket) {
                maxPos = orgPos + orgLength;
                // DEMUXERMP4_LOG("Found movv. (endpos %d)\n", (int)maxPos);
            }
        }

        if (typeInt == MP4_PARSER_DEFINE_TYPE_STRING("moof")) {
            if (!findPacket) {
                // PRINT_STOP_REASON("Found MOOF while parsing stream info\n");
                source->onSeek(orgPos, DemuxerSource::SeekWhenceSet);
                break;
            }
            lastUnpairedMoofPos = orgPos;
        }

        if (typeInt == MP4_PARSER_DEFINE_TYPE_STRING("mdat")) {
            if (!findPacket) {
                // PRINT_STOP_REASON("Found MDAT while parsing stream info\n");
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
        DEMUXERMP4_LOG("Rewind to last unpaired MOOF position %d (end:%d)\n",
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
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           STARFISH_LOG_INFO(
                                               "DemuxerMP4::~DemuxerMP4\n");
                                           DemuxerMP4* self = (DemuxerMP4*)obj;
                                           self->m_streamInfo.clear();
                                       },
                                       NULL, NULL, NULL);
    }

    virtual bool findStreamInfo(DemuxerSource* source, String* formatHint);
    virtual bool findStreamPacket(DemuxerSource* source);

    std::unordered_map<size_t, StreamInfoMP4> m_streamInfo;
    Mutex* m_demuxingMutex;
    MP4PacketGenerator* m_packetGenerator;
};

static StreamInfoMP4* handleTKHD(MP4::TKHD* tkhd,
                                 std::unordered_map<size_t, StreamInfoMP4>& map)
{
    if (tkhd->track_id < 1) {
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> TRAK(TKHD): Found wrong track number\n");
        return nullptr;
    }
    size_t streamIndex = tkhd->track_id - 1;
#ifdef DEMUXERMP4_DEBUG
    auto exist = map.find(streamIndex);
    if (exist != map.end()) {
        DEMUXERMP4_LOG("TRAK(TKHD): Replace previous stream at index %d\n",
                       (int)streamIndex);
    }
#endif
    auto& info = map[streamIndex];
    if (tkhd->track_width && tkhd->track_height) {
        DEMUXERMP4_LOG("TRAK(TKHD): Found new VIDEO stream\n");
        info.setType(StreamTypeVideo);
        info.setVideoWidth(tkhd->track_width);
        info.setVideoHeight(tkhd->track_height);
        info.setVideoHasFramerate(false);
    } else {
        DEMUXERMP4_LOG("TRAK(TKHD): Found new AUDIO stream\n");
        info.setType(StreamTypeAudio);
    }
    info.setRawDuration(tkhd->duration);
    info.setStreamIndex(tkhd->track_id - 1);
    return &info;
}

static bool handleMDHD(MP4::MDHD* mdhd, StreamInfoMP4* stream)
{
    if (!stream) {
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> MDHD: Need TRAK before MDHD\n");
        return false;
    }
    stream->setTimescale(mdhd->_timeScale);
    return true;
}

static bool handleELST(MP4::ELST* elst, StreamInfoMP4* stream)
{
    if (!stream) {
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> ELST: Need TRAK before ELST\n");
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
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> AVC1/AVC3\n");
        return false;
    }
    if (!generator->isCodec(MediaCodecUnknown) &&
        !generator->isCodec(MediaCodecVideoH264)) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> AVC1/AVC3: OTHER -> MP4H264\n");
        return false;
    }
    generator->setCodec(MediaCodecVideoH264);
    stream->setCodec(MediaCodecVideoH264);
    return true;
}

static bool handleMetHVC(StreamInfoMP4* stream, MP4PacketGenerator* generator)
{
    if (!stream || !stream->isVideo()) {
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> HVC1/HEV1\n");
        return false;
    }
    if (generator->isCodec(MediaCodecUnknown)) {
        generator->setCodec(MediaCodecVideoHEVC);
    } else if (!generator->isCodec(MediaCodecVideoHEVC)) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> HVC1/HEV1: OTHER -> MP4HEVC\n");
        return false;
    }
    stream->setCodec(MediaCodecVideoHEVC);
    return true;
}

static bool handleAVCC(MP4::AVCC* avcc, StreamInfoMP4* stream,
                       MP4PacketGenerator* generator)
{
    if (!stream || !stream->isVideo()) {
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> AVCC\n");
        return false;
    }
    if (!generator->isCodec(MediaCodecVideoH264)) {
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> AVCC: Invalid parent\n");
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
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> MP4A\n");
        return false;
    }
    stream->setAudioSampleRate(mp4a->sample_rate);
    stream->setAudioChannels(mp4a->channels);
    return true;
}

static bool handleESDS(MP4::ESDS* esds, StreamInfoMP4* stream)
{
    if (!stream || !stream->isAudio()) {
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> ESDS\n");
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

    bool result = parseMP4(source, false, [&](MP4::Atom* atom) -> bool {
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
                DEMUXERMP4_LOG("Unexpected structure of MP4\n");
                DEMUXERMP4_LOG("> TREX\n");
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
    uint64_t m_DTS;
    size_t m_streamIndex;
    size_t m_defaultSampleSize;
    size_t m_defaultSampleDuration;
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
        : m_seenTFHD(false)
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
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> Duplicate TFHD\n");
        return false;
    }
    if (tfhd->track_id < 1) {
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> Wrong track number found in TFHD\n");
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
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> Duplicate TFDT\n");
        return false;
    }
    parsingInfo.m_seenTFDT = true;
    parsingInfo.m_DTS = tfdt->decodeTime;
    return true;
}

static bool handleTRUN(MP4::TRUN* trun, SegmentParsingInfo& parsingInfo)
{
    if (parsingInfo.m_seenTRUN) {
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> Duplicate TRUN\n");
        return false;
    }
    parsingInfo.m_seenTRUN = true;
    parsingInfo.m_hasSampleSize = trun->has_sample_size;
    parsingInfo.m_hasSampleDuration = trun->has_sample_duration;
    parsingInfo.m_hasSampleCTSOffset = trun->has_sample_composition_time_offset;
    parsingInfo.m_samples = std::move(trun->samples);
    return true;
}

#ifdef DEMUXERMP4_DEBUG
static bool handleMFHD(MP4::MFHD* mfhd)
{
    DEMUXERMP4_LOG("Sequence %d data will follow\n", (int)mfhd->sequence_no);
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

    bool result = parseMP4(source, true, [&](MP4::Atom* atom) -> bool {
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
#ifdef DEMUXERMP4_DEBUG
        case MP4_PARSER_DEFINE_TYPE_STRING("mfhd"): {
            return handleMFHD((MP4::MFHD*)atom);
        }
#endif
        case MP4_PARSER_DEFINE_TYPE_STRING("mdat"): {
            if (!parsingInfo.m_seenTFHD || !parsingInfo.m_seenTFDT ||
                !parsingInfo.m_seenTRUN) {
                DEMUXERMP4_LOG("Unexpected structure of MP4\n");
                DEMUXERMP4_LOG("> MDAT\n");
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
                MediaPacket packet;
                packet.m_streamIndex = parsingInfo.m_streamIndex;
                size_t sampleSize = resolvePacketSize(stream, parsingInfo, i);
                if (sampleSize <= 0 || sizeSum + sampleSize > mdat->size) {
                    DEMUXERMP4_LOG("Unexpected structure of MP4\n");
                    DEMUXERMP4_LOG("> MDAT: Wrong sample size\n");
                    return false;
                }
                sizeSum += sampleSize;

                // packet.m_pts
                // packet.m_dts
                int64_t ctsOffset =
                    resolvePacketCTSOffset(stream, parsingInfo, i);
                packet.m_pts =
                    stream.codedTimeToMilliseconds(currentDTS + ctsOffset);
                packet.m_dts = stream.codedTimeToMilliseconds(currentDTS);

                // packet.m_duration
                size_t duration = resolvePacketDuration(stream, parsingInfo, i);
                if (duration <= 0) {
                    DEMUXERMP4_LOG("Unexpected structure of MP4\n");
                    DEMUXERMP4_LOG("> MDAT: Wrong sample duration\n");
                    return false;
                }
                currentDTS += duration;
                packet.m_duration = stream.codedTimeToMilliseconds(duration);

                if (!m_packetGenerator->generate(source, sampleSize, packet)) {
                    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
                    DEMUXERMP4_LOG("Unexpected structure of MP4\n");
                    DEMUXERMP4_LOG("> MDAT\n");
                    return false;
                }
                for (size_t j = 0; j < m_demuxerClients.size(); j++) {
                    if (!m_demuxerClients[j]->onDetectPacket(packet)) {
                        delete[] packet.m_data;
                        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
                        DEMUXERMP4_LOG("> Failed to append packet to groups\n");
                        DEMUXERMP4_LOG("> packet(pts: %d, dts:%d, dur:%d)\n",
                                       (int)packet.m_pts, (int)packet.m_dts,
                                       (int)packet.m_duration);
                        return false;
                    }
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
}

#undef DEMUXERMP4_LOG
#endif /* STARFISH_ENABLE_MULTIMEDIA */
