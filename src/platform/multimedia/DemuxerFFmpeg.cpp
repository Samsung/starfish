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

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

namespace StarFish {

bool g_ffmpegInited = false;
static const int g_ioBufferSize = 4096;
class DemuxerFFmpeg : public Demuxer {
public:
    DemuxerFFmpeg(DemuxerSource* source, String* formatHint)
        : Demuxer(source)
        , m_isStreamFinded(false)
    {
        if (!g_ffmpegInited) {
            av_register_all();
            avcodec_register_all();
            avformat_network_init();

            STARFISH_LOG_INFO("avcodec_version->%d avformat_version->%d \n", avcodec_version(), avformat_version());
            g_ffmpegInited = true;
        }


        // TODO delete this buffer
        m_formatHint = String::emptyString;
        m_bufferForIO = (uint8_t*)av_malloc(g_ioBufferSize);

        if (!formatHint->equals(String::emptyString)) {
            if (formatHint->startsWith("video/", false) || formatHint->startsWith("audio/", false)) {
                m_formatHint = formatHint->substring(6, formatHint->length() - 6)->toLower();
                const char* f = m_formatHint->utf8Data();
                STARFISH_LOG_INFO("DemuxerFFmpeg::DemuxerFFmpeg -> av_find_input_format %s\n", f);
            } else {
                STARFISH_LOG_INFO("DemuxerFFmpeg::DemuxerFFmpeg -> av_find_input_format X\n");
            }
        }
    }

    virtual bool findStreamInfo()
    {
        STARFISH_ASSERT(!m_isStreamFinded);

        // TODO delete this context
        m_formatContext = avformat_alloc_context();
        m_formatContext->iformat = av_find_input_format(m_formatHint->utf8Data());
        m_formatContext->flags = AVFMT_FLAG_CUSTOM_IO;

        // TODO delete this context
        m_formatContext->pb = avio_alloc_context(m_bufferForIO, g_ioBufferSize, 0, this, [](void *opaque, uint8_t *buf, int buf_size) -> int
        {
            DemuxerFFmpeg* self = (DemuxerFFmpeg*)opaque;
            size_t sizeSuccessToRead = 0;
            self->m_demuxerSource->onRead(buf_size, sizeSuccessToRead, buf);
            return sizeSuccessToRead;
        }, nullptr, [](void *opaque, int64_t offset, int whence) -> int64_t {
            DemuxerFFmpeg* self = (DemuxerFFmpeg*)opaque;
            if (whence == AVSEEK_SIZE) {
                return self->m_demuxerSource->onSeek(offset, DemuxerSource::SeekWhence::SeekWhenceLookSize);
            }
            return self->m_demuxerSource->onSeek(offset, (DemuxerSource::SeekWhence)whence);
        });

        int ret;
        if ((ret = avformat_open_input(&m_formatContext, "", NULL, NULL)) < 0) {
            av_free(m_formatContext);
            char error[128];
            av_strerror(ret, error, 128);
            STARFISH_LOG_ERROR("DemuxerFFmpeg::findStreamInfo avformat_open_input: Error(%s).\n", error);
            return false;
        }

        if ((ret = avformat_find_stream_info(m_formatContext, NULL)) < 0) {
            av_free(m_formatContext);
            char error[128];
            av_strerror(ret, error, 128);
            STARFISH_LOG_ERROR("DemuxerFFmpeg::findStreamInfo avformat_find_stream_info: Error(%s)\n", error);
            return false;
        }

        for (size_t i = 0; i < m_formatContext->nb_streams; i++) {
            STARFISH_ASSERT(m_formatContext->streams[i]->codec);
            STARFISH_LOG_INFO("DemuxerFFmpeg::findStreamInfo finded stream info. codec id %d\n", (int)m_formatContext->streams[i]->codec->codec_id);
            if (m_formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
                VideoStreamInfo info;
                info.m_streamIndex = i;
                info.m_codecName = m_formatContext->streams[i]->codec->codec_name;
                info.m_bitRate = m_formatContext->streams[i]->codec->bit_rate;
                info.m_timeBaseNum = m_formatContext->streams[i]->codec->time_base.num;
                info.m_timeBaseDen = m_formatContext->streams[i]->codec->time_base.den;
                info.m_width = info.m_timeBaseDen = m_formatContext->streams[i]->codec->width;
                info.m_height = info.m_timeBaseDen = m_formatContext->streams[i]->codec->height;
                for (size_t j = 0; j < m_demuxerClients.size(); j ++) {
                    m_demuxerClients[j]->onDetectVideoStream(info);
                }
            } else if (m_formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
                AudioStreamInfo info;
                info.m_streamIndex = i;
                info.m_codecName = m_formatContext->streams[i]->codec->codec_name;
                info.m_bitRate = m_formatContext->streams[i]->codec->bit_rate;
                info.m_sampleFormat = (AudioSampleFormat)m_formatContext->streams[i]->codec->sample_fmt;
                info.m_channels = m_formatContext->streams[i]->codec->channels;
                info.m_sampleRate = m_formatContext->streams[i]->codec->sample_rate;
                for (size_t j = 0; j < m_demuxerClients.size(); j ++) {
                    m_demuxerClients[j]->onDetectAudioStream(info);
                }
            } else if (m_formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE) {
                // TODO
            }
        }
        m_isStreamFinded = true;
        return true;
    }

    virtual bool findStreamPacket()
    {
        AVPacket avPacket;
        avPacket.size = 0;
        avPacket.data = NULL;
        av_init_packet(&avPacket);
        
        int ret;
        if ((ret = av_read_frame(m_formatContext, &avPacket)) >= 0) {
            STARFISH_LOG_ERROR("DemuxerFFmpeg::process av_read_frame streamIndex(%d, %p, %d)\n", (int)avPacket.stream_index, avPacket.data, (int)avPacket.size);
            for (size_t j = 0; j < m_demuxerClients.size(); j ++) {
                MediaPacket packet;
                packet.m_streamIndex = avPacket.stream_index;
                packet.m_data = avPacket.data;
                packet.m_dataSize = avPacket.size;
                packet.m_pts = avPacket.pts;
                m_demuxerClients[j]->onDetectPacket(packet);
            }
            av_free_packet(&avPacket);
            return true;
        }
        
        av_free_packet(&avPacket);

        char error[128];
        av_strerror(ret, error, 128);
        STARFISH_LOG_ERROR("DemuxerFFmpeg::process av_read_frame: Error(%s)\n", error);

        return false;
    }

    AVFormatContext* m_formatContext;
    uint8_t* m_bufferForIO;
    bool m_isStreamFinded;
    String* m_formatHint;
};

Demuxer* Demuxer::create(DemuxerSource* source, String* formatHint)
{
    return new DemuxerFFmpeg(source, formatHint);
}

}

#endif /* STARFISH_ENABLE_MULTIMEDIA */
