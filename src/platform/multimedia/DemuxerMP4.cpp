/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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
#include "Demuxer.h"

#include "mp4.h"
#include "atoms.h"
#include "MP4.BinaryStream.h"
#include "MP4.Parser.h"

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
        // virtual void onRead(size_t sizeWantToRead, size_t& sizeSuccessToRead, int& errorCode, uint8_t* buffer) = 0;
        m_source->onRead(n, out, error, (uint8_t*)s);
        STARFISH_ASSERT(error == 0);
    }

    virtual bool eof() const
    {
        return m_source->onSeek(0, StarFish::DemuxerSource::SeekWhenceLookSize) != m_source->onSeek(0, StarFish::DemuxerSource::SeekWhenceCurrent);
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

static uint32_t readBigEndianUnsignedInteger(DemuxerSource* source)
{
    uint8_t c[4];
    uint32_t n;

    size_t t1;
    int err;
    source->onRead(4, t1, err, c);

    n = (uint32_t) c[0] << 24 | (uint32_t) c[1] << 16 | (uint32_t) c[2] << 8 | (uint32_t) c[3];

    return n;
}

static void parseMP4(DemuxerSource* source, bool findStream, const std::function<void(MP4::Atom* atom)>& fn)
{
    bool container = false;
    uint32_t length;
    uint64_t dataLength;
    char type[ 5 ];
    char* data = NULL;
    MP4::Atom* atom = NULL;
    MP4BinaryStreamAdapter src(source);
    memset(type, 0, 5);

    int64_t maxPos = source->onSeek(0, DemuxerSource::SeekWhenceLookSize);
    while (maxPos != source->onSeek(0, DemuxerSource::SeekWhenceCurrent)) {
        size_t orgPos = source->onSeek(0, DemuxerSource::SeekWhenceCurrent);
        size_t orgLength = length = readBigEndianUnsignedInteger(source);
        dataLength = 0;

        size_t t1;
        int err;
        source->onRead(4, t1, err, (uint8_t*)type);

        if (length == 1) {
            dataLength = readBigEndianUnsignedInteger(source) - 16;
        } else {
            dataLength = length - 8;
        }

        // printf("found %x %s\n", (int)orgPos, type);
        uint32_t typeInt = MP4_PARSER_DEFINE_TYPE_STRING(type);

        if (typeInt == MP4_PARSER_DEFINE_TYPE_STRING("meta")) {
            source->onSeek(orgLength, DemuxerSource::SeekWhenceCurrent);
            continue;
        }

        if (typeInt == MP4_PARSER_DEFINE_TYPE_STRING("moov")) {
            if (!findStream) {
                maxPos = orgPos + orgLength;
                printf("found movv. (endpos %d)\n", (int)maxPos);
            }
        }

        if (!findStream) {
            if (typeInt == MP4_PARSER_DEFINE_TYPE_STRING("mdat") || typeInt == MP4_PARSER_DEFINE_TYPE_STRING("moof")) {
                source->onSeek(orgPos, DemuxerSource::SeekWhenceSet);
                break;
            }
        }

        MP4::Atom* atom = MP4::atomFactory(typeInt);

        if (atom->isContainerAtom()) {
            fn(atom);
            delete atom;
            continue;
        }
        ((MP4::DataAtom*)atom)->processData(&src, dataLength);
        fn(atom);
        delete atom;
    }
}

class DemuxerMP4 : public Demuxer {
public:
    DemuxerMP4()
        : Demuxer()
    {
        m_isStreamFinded = false;
        GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
            STARFISH_LOG_INFO("DemuxerMP4::~DemuxerMP4\n");
            DemuxerMP4* self = (DemuxerMP4*)obj;
            self->m_defaultTimescale.clear();
        }, NULL, NULL, NULL);
    }

    virtual bool findStreamInfo(DemuxerSource* source, String* formatHint)
    {
        bool seenTrack = false;
        int64_t before = source->onSeek(0, DemuxerSource::SeekWhenceCurrent);

        bool seenTkhd = false;
        size_t track_id;
        size_t track_width;
        size_t track_height;
        size_t duration;

        parseMP4(source, false, [&](MP4::Atom* atom) {
            if (atom->getType() == MP4_PARSER_DEFINE_TYPE_STRING("tkhd")) {
                MP4::TKHD* tkhd = (MP4::TKHD*)atom;
                track_id = tkhd->track_id;
                track_width = tkhd->track_width;
                track_height = tkhd->track_height;
                duration = tkhd->duration;
                seenTkhd = true;
            } else if (atom->getType() == MP4_PARSER_DEFINE_TYPE_STRING("mdhd")) {
                MP4::MDHD* mdhd = (MP4::MDHD*)atom;

                if (seenTkhd && mdhd) {
                    m_defaultTimescale[track_id - 1] = mdhd->_timeScale;
                    if (track_width && track_height) {
                        // video
                        VideoStreamInfo info;
                        info.m_streamIndex = track_id - 1;
                        info.m_duration = duration;
                        // TODO read avg_frame_rate
                        info.m_timeBaseNum = 0;
                        info.m_timeBaseDen = 1;
                        // TODO read codec name
                        info.m_codecName = "h264";
                        // info.m_bitRate = m_formatContext->streams[i]->codec->bit_rate;
                        info.m_width = track_width;
                        info.m_height = track_height;
                        STARFISH_LOG_INFO("DemuxerMP4::findStreamInfo finded video. %d %d %d\n", (int)info.m_streamIndex, (int)info.m_width, (int)info.m_height);
                        for (size_t j = 0; j < m_demuxerClients.size(); j ++) {
                            m_demuxerClients[j]->onDetectVideoStream(info);
                        }
                    } else {
                        AudioStreamInfo info;
                        info.m_streamIndex = track_id - 1;
                        info.m_duration = duration;
                        // TODO read codec name
                        info.m_codecName = "aac";
                        // info.m_bitRate = m_formatContext->streams[i]->codec->bit_rate;
                        // info.m_sampleFormat = (AudioSampleFormat)m_formatContext->streams[i]->codec->sample_fmt;
                        // info.m_channels = m_formatContext->streams[i]->codec->channels;
                        info.m_sampleRate = mdhd->_timeScale;
                        // STARFISH_LOG_INFO("DemuxerMP4::findStreamInfo finded audio. %d\n", (int)info.m_streamIndex);
                        for (size_t j = 0; j < m_demuxerClients.size(); j ++) {
                            m_demuxerClients[j]->onDetectAudioStream(info);
                        }
                    }
                }
                seenTkhd = false;
                mdhd = nullptr;
            } else if (atom->getType() == MP4_PARSER_DEFINE_TYPE_STRING("moov")) {
                seenTrack = true;
            }
        });

        if (seenTrack) {
            if (!m_isStreamFinded)
                m_isStreamFinded = seenTrack;
        } else {
            source->onSeek(before, DemuxerSource::SeekWhenceSet);
        }

        return seenTrack;
    }

    virtual bool findStreamPacket(DemuxerSource* source)
    {
        size_t trackID = SIZE_MAX;
        bool has_default_sample_size;
        bool has_default_sample_duration;
        size_t default_sample_size;
        size_t default_sample_duration;
        uint64_t dts = SIZE_MAX;
        bool seenTrun = false;
        bool seenTfhd = false;
        std::vector<MP4::TRUN::Sample> samples;
        bool has_sample_size;
        bool has_sample_duration;
        parseMP4(source, true, [&](MP4::Atom* atom) {
            if (atom->getType() == MP4_PARSER_DEFINE_TYPE_STRING("tfhd")) {
                MP4::TFHD* tfhd = (MP4::TFHD*)atom;
                seenTfhd = true;
                trackID = tfhd->track_id - 1;
                has_default_sample_duration = tfhd->has_default_sample_duration;
                has_default_sample_size = tfhd->has_default_sample_size;
                default_sample_size = tfhd->default_sample_size;
                default_sample_duration = tfhd->default_sample_duration;
            } else if (atom->getType() == MP4_PARSER_DEFINE_TYPE_STRING("tfdt")) {
                MP4::TFDT* tfdt = (MP4::TFDT*)atom;
                dts = tfdt->decodeTime;
            } else if (atom->getType() == MP4_PARSER_DEFINE_TYPE_STRING("trun")) {
                MP4::TRUN* trun = (MP4::TRUN*)atom;
                has_sample_size = trun->has_sample_size;
                has_sample_duration = trun->has_sample_duration;
                seenTrun = true;
                samples = std::move(trun->samples);
            } else if (atom->getType() == MP4_PARSER_DEFINE_TYPE_STRING("mdat")) {
                MP4::MDAT* mdat = (MP4::MDAT*)atom;
                if (trackID != SIZE_MAX && dts != SIZE_MAX && seenTrun && seenTfhd) {
                    int64_t before = source->onSeek(0, DemuxerSource::SeekWhenceCurrent);
                    source->onSeek(mdat->dataPos, DemuxerSource::SeekWhenceSet);
                    MediaPacket packet;
                    packet.m_streamIndex = trackID;
                    size_t scale = m_defaultTimescale[trackID];
                    size_t dataPtr = 0;
                    uint64_t ptsInMP4 = dts;

                    STARFISH_RELEASE_ASSERT(has_sample_size || has_default_sample_size);
                    for (size_t i = 0; i < samples.size(); i ++) {
                        if (has_sample_size)
                            packet.m_dataSize = samples[i].size;
                        else
                            packet.m_dataSize = default_sample_size;
                        packet.m_pts = ptsInMP4 * 1000LL / scale;

                        if (has_sample_duration) {
                            ptsInMP4 += samples[i].duration;
                            packet.m_duration = samples[i].duration * 1000LL / scale;
                        } else if (has_default_sample_duration) {
                            ptsInMP4 += default_sample_duration;
                            packet.m_duration = default_sample_duration * 1000LL / scale;
                        } else {
                            STARFISH_RELEASE_ASSERT_NOT_REACHED();
                        }

                        uint8_t* data = new uint8_t[packet.m_dataSize];
                        size_t s;
                        int err;
                        source->onRead(packet.m_dataSize, s, err, data);
                        STARFISH_ASSERT(packet.m_dataSize == s);
                        packet.m_data = data;
                        /*
                        STARFISH_LOG_INFO("DemuxerMP4::findStreamPacket streamIndex(%d, %dbyte, %dms)\n", (int)packet.m_streamIndex, (int)packet.m_dataSize, (int)packet.m_pts);
                        */
                        for (size_t j = 0; j < m_demuxerClients.size(); j ++) {
                            if (m_demuxerClients[j]->onDetectPacket(packet)) {
                                data = nullptr;
                                break;
                            }
                        }
                        delete[] data;
                        dataPtr += packet.m_dataSize;
                        STARFISH_ASSERT(dataPtr <= mdat->size);
                    }

                    source->onSeek(before, DemuxerSource::SeekWhenceSet);
                }

                trackID = SIZE_MAX;
                dts = SIZE_MAX;
                seenTrun = false;
                seenTfhd = false;
            }
        });

        return true;
    }

    virtual bool isFindedStreamInfo()
    {
        return m_isStreamFinded;
    }

    bool m_isStreamFinded;
    std::unordered_map<size_t, size_t> m_defaultTimescale;
};

Demuxer* Demuxer::createMP4Demuxer()
{
    return new DemuxerMP4();
}

}

#endif /* STARFISH_ENABLE_MULTIMEDIA */
