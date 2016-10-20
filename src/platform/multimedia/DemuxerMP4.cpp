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

static MP4::File* parseMP4(DemuxerSource* source, bool findStream)
{
    bool container = false;
    uint32_t length;
    uint64_t dataLength;
    char type[ 5 ];
    char* data = NULL;
    MP4::File* file = new MP4::File();
    MP4::Atom* atom = NULL;
    MP4::ContainerAtom* containerAtom = NULL;
    MP4::ContainerAtom* parentAtom = file;
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

        if (strcmp(type, "moov") == 0) {
            if (!findStream) {
                maxPos = orgPos + orgLength;
                printf("found movv. (endpos %d)\n", (int)maxPos);
            }
        }

        // printf("found %x %s\n", (int)orgPos, type);

        /* Container atoms */
        if (strcmp(type, "dinf") == 0
            || strcmp(type, "edts") == 0
            || strcmp(type, "ipro") == 0
            || strcmp(type, "mdia") == 0
            || strcmp(type, "meta") == 0
            || strcmp(type, "mfra") == 0
            || strcmp(type, "minf") == 0
            || strcmp(type, "moof") == 0
            || strcmp(type, "moov") == 0
            || strcmp(type, "mvex") == 0
            || strcmp(type, "sinf") == 0
            || strcmp(type, "skip") == 0
            || strcmp(type, "stbl") == 0
            || strcmp(type, "traf") == 0
            || strcmp(type, "trak") == 0) {
            containerAtom = new MP4::ContainerAtom( type );

            parentAtom->addChild(containerAtom);
            parentAtom = containerAtom;
            continue;
        }

        /* Data atoms */
        if (strcmp(type, "bxml") == 0) {
            atom = (MP4::Atom *) (new MP4::BXML());
        } else if (strcmp(type, "co64") == 0) {
            atom = (MP4::Atom *) (new MP4::CO64());
        } else if (strcmp(type, "cprt") == 0) {
            atom = (MP4::Atom *) (new MP4::CPRT());
        } else if (strcmp(type, "ctts") == 0) {
            atom = (MP4::Atom *) (new MP4::CTTS());
        } else if (strcmp(type, "dref") == 0) {
            atom = (MP4::Atom *) (new MP4::DREF());
        } else if (strcmp(type, "elst") == 0) {
            atom = (MP4::Atom *) (new MP4::ELST());
        } else if (strcmp(type, "free") == 0) {
            atom = (MP4::Atom *) (new MP4::FREE());
        } else if (strcmp(type, "frma") == 0) {
            atom = (MP4::Atom *) (new MP4::FRMA());
        } else if (strcmp(type, "ftyp") == 0) {
            atom = (MP4::Atom *) (new MP4::FTYP());
        } else if (strcmp(type, "hdlr") == 0) {
            atom = (MP4::Atom *) (new MP4::HDLR());
        } else if (strcmp(type, "hmhd") == 0) {
            atom = (MP4::Atom *) (new MP4::HMHD());
        } else if (strcmp(type, "iinf") == 0) {
            atom = (MP4::Atom *) (new MP4::IINF());
        } else if (strcmp(type, "iloc") == 0) {
            atom = (MP4::Atom *) (new MP4::ILOC());
        } else if (strcmp(type, "imif") == 0) {
            atom = (MP4::Atom *) (new MP4::IMIF());
        } else if (strcmp(type, "ipmc") == 0) {
            atom = (MP4::Atom *) (new MP4::IPMC());
        } else if (strcmp(type, "mdat") == 0) {
            atom = (MP4::Atom *) (new MP4::MDAT());
        } else if (strcmp(type, "mdhd") == 0) {
            atom = (MP4::Atom *) (new MP4::MDHD());
        } else if (strcmp(type, "mehd") == 0) {
            atom = (MP4::Atom *) (new MP4::MEHD());
        } else if (strcmp(type, "mfhd") == 0) {
            atom = (MP4::Atom *) (new MP4::MFHD());
        } else if (strcmp(type, "mfro") == 0) {
            atom = (MP4::Atom *) (new MP4::MFRO());
        } else if (strcmp(type, "mvhd") == 0) {
            atom = (MP4::Atom *) (new MP4::MVHD());
        } else if (strcmp(type, "padb") == 0) {
            atom = (MP4::Atom *) (new MP4::PADB());
        } else if (strcmp(type, "pdin") == 0) {
            atom = (MP4::Atom *) (new MP4::PDIN());
        } else if (strcmp(type, "pitm") == 0) {
            atom = (MP4::Atom *) (new MP4::PITM());
        } else if (strcmp(type, "sbgp") == 0) {
            atom = (MP4::Atom *) (new MP4::SBGP());
        } else if (strcmp(type, "schi") == 0) {
            atom = (MP4::Atom *) (new MP4::SCHI());
        } else if (strcmp(type, "schm") == 0) {
            atom = (MP4::Atom *) (new MP4::SCHM());
        } else if (strcmp(type, "sdtp") == 0) {
            atom = (MP4::Atom *) (new MP4::SDTP());
        } else if (strcmp(type, "sgpd") == 0) {
            atom = (MP4::Atom *) (new MP4::SGPD());
        } else if (strcmp(type, "smhd") == 0) {
            atom = (MP4::Atom *) (new MP4::SMHD());
        } else if (strcmp(type, "subs") == 0) {
            atom = (MP4::Atom *) (new MP4::SUBS());
        } else if (strcmp(type, "stsd") == 0) {
            atom = (MP4::Atom *) (new MP4::STSD());
        } else if (strcmp(type, "stco") == 0) {
            atom = (MP4::Atom *) (new MP4::STCO());
        } else if (strcmp(type, "stdp") == 0) {
            atom = (MP4::Atom *) (new MP4::STDP());
        } else if (strcmp(type, "stsc") == 0) {
            atom = (MP4::Atom *) (new MP4::STSC());
        } else if (strcmp(type, "stsh") == 0) {
            atom = (MP4::Atom *) (new MP4::STSH());
        } else if (strcmp(type, "stss") == 0) {
            atom = (MP4::Atom *) (new MP4::STSS());
        } else if (strcmp(type, "stsz") == 0) {
            atom = (MP4::Atom *) (new MP4::STSZ());
        } else if (strcmp(type, "stts") == 0) {
            atom = (MP4::Atom *) (new MP4::STTS());
        } else if (strcmp(type, "stz2") == 0) {
            atom = (MP4::Atom *) (new MP4::STZ2());
        } else if (strcmp(type, "tfdt") == 0) {
            atom = (MP4::Atom *) (new MP4::TFDT());
        } else if (strcmp(type, "tfhd") == 0) {
            atom = (MP4::Atom *) (new MP4::TFHD());
        } else if (strcmp(type, "tfra") == 0) {
            atom = (MP4::Atom *) (new MP4::TFRA());
        } else if (strcmp(type, "tkhd") == 0) {
            atom = (MP4::Atom *) (new MP4::TKHD());
        } else if (strcmp(type, "tref") == 0) {
            atom = (MP4::Atom *) (new MP4::TREF());
        } else if (strcmp(type, "trex") == 0) {
            atom = (MP4::Atom *) (new MP4::TREX());
        } else if (strcmp(type, "trun") == 0) {
            atom = (MP4::Atom *) (new MP4::TRUN());
        } else if (strcmp(type, "udta") == 0) {
            atom = (MP4::Atom *) (new MP4::UDTA());
        } else if (strcmp(type, "vmhd") == 0) {
            atom = (MP4::Atom *) (new MP4::VMHD());
        } else if (strcmp(type, "xml ") == 0) {
            atom = (MP4::Atom *) (new MP4::XML());
        } else {
            atom = new MP4::UnknownAtom(type);
        }

        parentAtom->addChild(atom);

        ((MP4::DataAtom*)atom)->processData(&src, dataLength);
    }

    return file;
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
        STARFISH_ASSERT(!m_isStreamFinded);
        bool seenTrack = false;
        std::unique_ptr<MP4::File> pFile(parseMP4(source, false));
        pFile->traverse([&](MP4::Atom* atom, MP4::ContainerAtom* container) {
            if (atom->getType() == "TKHD") {
                MP4::TKHD* tkhd = (MP4::TKHD*)atom;
                MP4::MDHD* mdhd = (MP4::MDHD*)container->findChild("MDHD");
                if (mdhd) {
                    seenTrack = true;
                    m_defaultTimescale[tkhd->track_id - 1] = mdhd->_timeScale;
                    if (tkhd->track_width && tkhd->track_height) {
                        // video
                        VideoStreamInfo info;
                        info.m_streamIndex = tkhd->track_id - 1;
                        info.m_duration = tkhd->duration;
                        // TODO read avg_frame_rate
                        info.m_timeBaseNum = 0;
                        info.m_timeBaseDen = 1;
                        // TODO read codec name
                        info.m_codecName = "h264";
                        // info.m_bitRate = m_formatContext->streams[i]->codec->bit_rate;
                        info.m_width = tkhd->track_width;
                        info.m_height = tkhd->track_height;
                        // STARFISH_LOG_INFO("DemuxerMP4::findStreamInfo finded video. %d %d %d\n", (int)info.m_streamIndex, (int)info.m_width, (int)info.m_height);
                        for (size_t j = 0; j < m_demuxerClients.size(); j ++) {
                            m_demuxerClients[j]->onDetectVideoStream(info);
                        }
                    } else {
                        AudioStreamInfo info;
                        info.m_streamIndex = tkhd->track_id - 1;
                        info.m_duration = tkhd->duration;
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
            }
        });

        m_isStreamFinded = seenTrack;
        return seenTrack;
    }

    virtual bool findStreamPacket(DemuxerSource* source)
    {
        STARFISH_ASSERT(m_isStreamFinded);
        std::unique_ptr<MP4::File> pFile(parseMP4(source, true));

        size_t trackID = SIZE_MAX;
        uint64_t dts = SIZE_MAX;
        MP4::TRUN* trun = nullptr;
        pFile->traverse([&](MP4::Atom* atom, MP4::ContainerAtom* container) {
            if (atom->getType() == "TFHD") {
                MP4::TFHD* tfhd = (MP4::TFHD*)atom;
                trackID = tfhd->track_id - 1;
            } else if (atom->getType() == "TFDT") {
                MP4::TFDT* tfdt = (MP4::TFDT*)atom;
                dts = tfdt->decodeTime;
            } else if (atom->getType() == "TRUN") {
                trun = (MP4::TRUN*)atom;
            } else if (atom->getType() == "MDAT") {
                MP4::MDAT* mdat = (MP4::MDAT*)atom;
                if (trackID != SIZE_MAX && dts != SIZE_MAX && trun != nullptr) {
                    MediaPacket packet;
                    packet.m_streamIndex = trackID;
                    size_t scale = m_defaultTimescale[trackID];
                    uint8_t* dataPtr = mdat->data;
                    uint64_t pts = dts * 1000 / scale;
                    STARFISH_ASSERT(trun->has_sample_size);
                    for (size_t i = 0; i < trun->samples.size(); i ++) {
                        packet.m_data = dataPtr;
                        packet.m_dataSize = trun->samples[i].size;
                        packet.m_pts = pts;

                        if (trun->has_sample_duration) {
                            packet.m_duration = trun->samples[i].duration * 1000.0 / (double)scale;
                        } else if (trun->has_sample_composition_time_offset) {
                            packet.m_duration = trun->samples[i].composition_time_offset * 1000.0 / (double)scale;
                        } else {
                            STARFISH_RELEASE_ASSERT_NOT_REACHED();
                        }
                        dataPtr += trun->samples[i].size;

                        STARFISH_ASSERT(dataPtr <= (mdat->data + mdat->size));
                        // STARFISH_LOG_INFO("DemuxerMP4::findStreamPacket streamIndex(%d, %dbyte, %dms)\n", (int)packet.m_streamIndex, (int)packet.m_dataSize, (int)packet.m_pts);
                        for (size_t j = 0; j < m_demuxerClients.size(); j ++) {
                            m_demuxerClients[j]->onDetectPacket(packet);
                        }

                        pts += packet.m_duration;
                    }
                }

                trackID = SIZE_MAX;
                dts = SIZE_MAX;
                trun = nullptr;
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
