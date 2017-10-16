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

static void parseMP4(DemuxerSource* source, bool findPacket,
                     const std::function<void(MP4::Atom* atom)>& fn)
{
    bool container = false;
    uint32_t length;
    uint64_t dataLength;
    size_t lastUnpairedMoofPos = SIZE_MAX;
    char type[5];
    char* data = NULL;
    MP4::Atom* atom = NULL;
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
                DEMUXERMP4_LOG("Found movv. (endpos %d)\n", (int)maxPos);
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
        fn(atom);
        delete atom;
    }
    if (lastUnpairedMoofPos != SIZE_MAX) {
        DEMUXERMP4_LOG("Rewind to last unpaired MOOF position %d (end:%d)\n",
                       (int)lastUnpairedMoofPos, (int)maxPos);
        source->onSeek(lastUnpairedMoofPos, DemuxerSource::SeekWhenceSet);
    }
#undef PRINT_STOP_REASON
}

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

    std::unordered_map<size_t, StreamInfo> m_streamInfo;
    Mutex* m_demuxingMutex;
    MP4PacketGenerator* m_packetGenerator;
};

static StreamInfo* handleTKHD(MP4::TKHD* tkhd,
                              std::unordered_map<size_t, StreamInfo>& map)
{
    if (tkhd->track_id < 1) {
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> Wrong track number found in TKHD\n");
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
    info.setDuration(tkhd->duration * 1000LL);
    info.setStreamIndex(tkhd->track_id - 1);
    return &info;
}

static bool handleMDHD(MP4::MDHD* mdhd, StreamInfo* stream)
{
    if (!stream) {
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> no TKHD found before MDHD\n");
        return false;
    }
    stream->setTimescale(mdhd->_timeScale);
    if (mdhd->_timeScale && stream->duration()) {
        // Note Set duration
        // https://www.w3.org/2008/WebVideo/Annotations/drafts/
        // ontology10/CR/mappings_tested/container-MPEG4.htm
        stream->setDuration(stream->duration() / mdhd->_timeScale);
    }
    return true;
}

static bool handleELST(MP4::ELST* elst, StreamInfo* stream)
{
    if (!stream) {
        return false;
    }
    uint64_t mediaTime = 0;
    if (elst->edit_entries.size() > 0 && elst->edit_entries[0].media_time > 0) {
        mediaTime = (uint64_t)elst->edit_entries[0].media_time;
    }
    stream->setMediaTime(mediaTime);
    return true;
}

static bool handleMetAVC(StreamInfo* stream, MP4PacketGenerator* generator)
{
    if (!stream || !stream->isVideo()) {
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> AVC1/AVC3\n");
        return false;
    }
    if (generator->isCodec(MediaCodecUnknown)) {
        generator->setCodec(MediaCodecVideoH264);
    } else if (!generator->isCodec(MediaCodecVideoH264)) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> OTHER -> MP4H264\n");
        return false;
    }
    stream->setCodec(MediaCodecVideoH264);
    return true;
}

static bool handleMetHVC(StreamInfo* stream, MP4PacketGenerator* generator)
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
        DEMUXERMP4_LOG("> OTHER -> MP4HEVC\n");
        return false;
    }
    stream->setCodec(MediaCodecVideoHEVC);
    return true;
}

static bool handleAVCC(MP4::AVCC* avcc, StreamInfo* stream,
                       MP4PacketGenerator* generator)
{
    if (!stream || !stream->isVideo()) {
        return false;
    }
    if (!generator->isCodec(MediaCodecVideoH264)) {
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> AVCC\n");
        return false;
    }

    // NOTE MediaPlayer does not need extradata for MP4 Video,
    //      because it'd be appended to every keyframes (annexB)
    STARFISH_ASSERT(stream->m_extraData.size() == 0);

    generator->setAVCNALSizeLength(avcc->nal_length);
    generator->setAVCExtraData(avcc->spsVector, avcc->ppsVector);
    return true;
}

static bool handleMP4A(MP4::MP4A* mp4a, StreamInfo* stream)
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

static bool handleESDS(MP4::ESDS* esds, StreamInfo* stream)
{
    if (!stream || !stream->isAudio()) {
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> ESDS\n");
        return false;
    }
    if (stream->m_extraData.size() > 0) {
        DEMUXERMP4_LOG("Unexpected structure of MP4\n");
        DEMUXERMP4_LOG("> Current stream have met ESDS before\n");
        return false;
    }
    if (esds->decoder_config_size) {
        stream->m_extraData = std::move(esds->decoder_config);
    }
    // TODO
    stream->setCodec(MediaCodecAudioAAC);
    return true;
}

bool DemuxerMP4::findStreamInfo(DemuxerSource* source, String* formatHint)
{
    Locker<Mutex> lock(*m_demuxingMutex);

    int64_t before = source->onSeek(0, DemuxerSource::SeekWhenceCurrent);
    std::vector<size_t> detectedStreamIndice;
    StreamInfo* currentStream = nullptr;

    parseMP4(source, false, [&](MP4::Atom* atom) {
        uint32_t type = atom->getType();
        switch (type) {
        case MP4_PARSER_DEFINE_TYPE_STRING("tkhd"): {
            // Found new stream
            currentStream = handleTKHD((MP4::TKHD*)atom, m_streamInfo);
            if (currentStream) {
                detectedStreamIndice.push_back(currentStream->streamIndex());
            }
            break;
        }
        case MP4_PARSER_DEFINE_TYPE_STRING("mdhd"): {
            handleMDHD((MP4::MDHD*)atom, currentStream);
            break;
        }
        case MP4_PARSER_DEFINE_TYPE_STRING("elst"): {
            handleELST((MP4::ELST*)atom, currentStream);
            break;
        }
        case MP4_PARSER_DEFINE_TYPE_STRING("avc1"):
        case MP4_PARSER_DEFINE_TYPE_STRING("avc3"): {
            handleMetAVC(currentStream, m_packetGenerator);
            break;
        }
        case MP4_PARSER_DEFINE_TYPE_STRING("hev1"):
        case MP4_PARSER_DEFINE_TYPE_STRING("hvc1"): {
            handleMetHVC(currentStream, m_packetGenerator);
            break;
        }
        case MP4_PARSER_DEFINE_TYPE_STRING("mp4a"): {
            handleMP4A((MP4::MP4A*)atom, currentStream);
            break;
        }
        case MP4_PARSER_DEFINE_TYPE_STRING("esds"): {
            handleESDS((MP4::ESDS*)atom, currentStream);
            break;
        }
        case MP4_PARSER_DEFINE_TYPE_STRING("avcC"): {
            handleAVCC((MP4::AVCC*)atom, currentStream, m_packetGenerator);
            break;
        }
        default:
            break;
        }
    });
    if (detectedStreamIndice.size() == 0) {
        source->onSeek(before, DemuxerSource::SeekWhenceSet);
        return false;
    }
    size_t submitCount = 0;
    for (size_t i = 0; i < detectedStreamIndice.size(); i++) {
        StreamInfo& item = m_streamInfo[detectedStreamIndice[i]];
        for (size_t j = 0; j < m_demuxerClients.size(); j++) {
            if (item.isAudio()) {
                m_demuxerClients[j]->onDetectAudioStream(item);
                submitCount++;
            } else if (item.isVideo()) {
                m_demuxerClients[j]->onDetectVideoStream(item);
                submitCount++;
            } else {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            }
        }
    }
    return submitCount > 0;
}

bool DemuxerMP4::findStreamPacket(DemuxerSource* source)
{
    Locker<Mutex> lock(*m_demuxingMutex);
    bool seenTrun = false;
    bool seenTfhd = false;
    size_t streamIndex = SIZE_MAX;
    uint64_t dts = SIZE_MAX;
    std::vector<MP4::TRUN::Sample> samples;
    // Sample size
    bool has_sample_size = false;
    bool has_default_sample_size = false;
    size_t default_sample_size = 0;
    // Sample duration
    bool has_sample_duration = false;
    bool has_default_sample_duration = false;
    size_t default_sample_duration = 0;
    // SampleComposition time offset
    bool has_sample_composition_time_offset = false;

    parseMP4(source, true, [&](MP4::Atom* atom) {
        uint32_t type = atom->getType();
        switch (type) {
        case MP4_PARSER_DEFINE_TYPE_STRING("tfhd"): {
            MP4::TFHD* tfhd = (MP4::TFHD*)atom;
            seenTfhd = true;
            streamIndex = tfhd->track_id - 1;
            has_default_sample_duration = tfhd->has_default_sample_duration;
            has_default_sample_size = tfhd->has_default_sample_size;
            default_sample_size = tfhd->default_sample_size;
            default_sample_duration = tfhd->default_sample_duration;
            break;
        }
        case MP4_PARSER_DEFINE_TYPE_STRING("tfdt"): {
            MP4::TFDT* tfdt = (MP4::TFDT*)atom;
            dts = tfdt->decodeTime;
            break;
        }
        case MP4_PARSER_DEFINE_TYPE_STRING("trun"): {
            MP4::TRUN* trun = (MP4::TRUN*)atom;
            has_sample_size = trun->has_sample_size;
            has_sample_duration = trun->has_sample_duration;
            has_sample_composition_time_offset =
                trun->has_sample_composition_time_offset;
            samples = std::move(trun->samples);
            seenTrun = true;
            break;
        }
#ifdef DEMUXERMP4_DEBUG
        case MP4_PARSER_DEFINE_TYPE_STRING("mfhd"): {
            MP4::MFHD* mfhd = (MP4::MFHD*)atom;
            DEMUXERMP4_LOG("Sequence %d data will follow\n",
                           (int)mfhd->sequence_no);
            break;
        }
#endif
        case MP4_PARSER_DEFINE_TYPE_STRING("mdat"): {
            if (streamIndex == SIZE_MAX || dts == SIZE_MAX || !seenTrun ||
                !seenTfhd) {
                DEMUXERMP4_LOG("Unexpected structure of MP4\n");
                return;
            }
            StreamInfo& stream = m_streamInfo[streamIndex];
            MP4::MDAT* mdat = (MP4::MDAT*)atom;
            int64_t before =
                source->onSeek(0, DemuxerSource::SeekWhenceCurrent);
            int64_t start = (int64_t)mdat->dataPos;
            source->onSeek(start, DemuxerSource::SeekWhenceSet);

            size_t sizeSum = 0;
            uint64_t currentDTS = dts;
            for (size_t i = 0; i < samples.size(); i++) {
                MediaPacket packet;
                packet.m_streamIndex = streamIndex;
                size_t sampleSize = 0;
                if (has_sample_size) {
                    sampleSize = samples[i].size;
                } else if (has_default_sample_size) {
                    sampleSize = default_sample_size;
                } else {
                    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
                    DEMUXERMP4_LOG("Unexpected structure of MP4\n");
                    DEMUXERMP4_LOG("> Could not determine sample size\n");
                    return;
                }
                if (sizeSum + sampleSize > mdat->size) {
                    DEMUXERMP4_LOG("Unexpected structure of MP4\n");
                    DEMUXERMP4_LOG("> Wrong sample size\n");
                    return;
                }
                sizeSum += sampleSize;

                // packet.m_pts
                // TODO divide by scale later
                int64_t ctsOffset = 0;
                if (has_sample_composition_time_offset) {
                    ctsOffset = samples[i].composition_time_offset;
                }
                ctsOffset -= stream.mediaTime();
                packet.m_pts =
                    (currentDTS + ctsOffset) * 1000LL / stream.timescale();
                packet.m_dts = currentDTS * 1000LL / stream.timescale();

                // packet.m_duration
                // TODO divide by scale later
                uint32_t duration = 0;
                if (has_sample_duration) {
                    duration = samples[i].duration;
                } else if (has_default_sample_duration) {
                    duration = default_sample_duration;
                } else {
                    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
                    DEMUXERMP4_LOG("Unexpected structure of MP4\n");
                    DEMUXERMP4_LOG("> Could not determine duration\n");
                    return;
                }
                currentDTS += duration;
                packet.m_duration = duration * 1000LL / stream.timescale();

                if (!m_packetGenerator->generate(source, sampleSize, packet)) {
                    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
                    DEMUXERMP4_LOG("Unexpected structure of MP4\n");
                    DEMUXERMP4_LOG("> Fail to parse MDAT\n");
                    return;
                }
                for (size_t j = 0; j < m_demuxerClients.size(); j++) {
                    if (m_demuxerClients[j]->onDetectPacket(packet)) {
                        break;
                    }
                }
            }
            source->onSeek(before, DemuxerSource::SeekWhenceSet);
            streamIndex = SIZE_MAX;
            dts = SIZE_MAX;
            seenTrun = false;
            seenTfhd = false;
            break;
        }
        default:
            break;
        }
    });

    return true;
}

Demuxer* Demuxer::createMP4Demuxer()
{
    return new DemuxerMP4();
}
}

#undef DEMUXERMP4_LOG
#endif /* STARFISH_ENABLE_MULTIMEDIA */
