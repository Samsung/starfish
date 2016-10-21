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
    DemuxerFFmpeg()
        : Demuxer()
        , m_isStreamFinded(false)
        , m_formatContext(nullptr)
        , m_ioContext(nullptr)
    {
        init();
    }

    void init()
    {
        if (!g_ffmpegInited) {
            av_register_all();
            avcodec_register_all();
            avformat_network_init();
            // av_log_set_level(AV_LOG_TRACE);

            STARFISH_LOG_INFO("avcodec_version->%d avformat_version->%d \n", avcodec_version(), avformat_version());
            g_ffmpegInited = true;
        }



        m_bufferForIO = (uint8_t*)av_malloc(g_ioBufferSize);

        m_ioContext = avio_alloc_context(m_bufferForIO, g_ioBufferSize, 0, this, [](void *opaque, uint8_t *buf, int buf_size) -> int
        {
            DemuxerFFmpeg* self = (DemuxerFFmpeg*)opaque;
            size_t sizeSuccessToRead = 0;
            int error = 0;
            self->m_demuxerSource->onRead(buf_size, sizeSuccessToRead, error, buf);
            if (error)
                return AVERROR(EIO);
            return sizeSuccessToRead;
        }, nullptr, [](void *opaque, int64_t offset, int whence) -> int64_t {
            DemuxerFFmpeg* self = (DemuxerFFmpeg*)opaque;
            if (whence == AVSEEK_SIZE) {
                return self->m_demuxerSource->onSeek(offset, DemuxerSource::SeekWhence::SeekWhenceLookSize);
            }
            return self->m_demuxerSource->onSeek(offset, (DemuxerSource::SeekWhence)whence);
        });

        GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
            STARFISH_LOG_INFO("DemuxerFFmpeg::~DemuxerFFmpeg\n");
            DemuxerFFmpeg* self = (DemuxerFFmpeg*)obj;
            av_free(self->m_ioContext);
            av_free(self->m_bufferForIO);
            if (self->m_formatContext)
                avformat_free_context(self->m_formatContext);
        }, NULL, NULL, NULL);
    }

    class DemuxerSourceController {
    public:
        DemuxerSourceController(DemuxerFFmpeg& src, DemuxerSource* source)
            : m_src(&src)
        {
            STARFISH_ASSERT(src.m_demuxerSource == nullptr);
            src.m_demuxerSource = source;
        }
        ~DemuxerSourceController()
        {
            m_src->m_demuxerSource = nullptr;
        }
        DemuxerFFmpeg* m_src;
    };

    virtual bool findStreamInfo(DemuxerSource* source, String* formatHint)
    {
        STARFISH_ASSERT(!m_isStreamFinded);
        DemuxerSourceController c(*this, source);

        if (!formatHint->equals(String::emptyString)) {
            if (formatHint->startsWith("video/", false) || formatHint->startsWith("audio/", false)) {
                formatHint = formatHint->substring(6, formatHint->length() - 6)->toLower();
                STARFISH_LOG_INFO("DemuxerFFmpeg::DemuxerFFmpeg -> av_find_input_format %s\n", formatHint->utf8Data());
            } else {
                STARFISH_LOG_INFO("DemuxerFFmpeg::DemuxerFFmpeg -> av_find_input_format X\n");
                formatHint = String::emptyString;
            }
        }

        int ret;
        if (!m_formatContext) {
            m_formatContext = avformat_alloc_context();

            if (formatHint->length())
                m_formatContext->iformat = av_find_input_format(formatHint->utf8Data());
            m_formatContext->flags = AVFMT_FLAG_CUSTOM_IO | AVFMT_FLAG_NOFILLIN | AVFMT_FLAG_NOBUFFER | AVFMT_FLAG_GENPTS;
            m_formatContext->pb = m_ioContext;
            av_dict_set(&m_formatContext->metadata, "skip_id3v1_tags", "", 0);

            if ((ret = avformat_open_input(&m_formatContext, NULL, NULL, NULL)) < 0) {
                m_formatContext = nullptr;
                char error[128];
                av_strerror(ret, error, 128);
                STARFISH_LOG_ERROR("DemuxerFFmpeg::findStreamInfo avformat_open_input: Error(%s).\n", error);
                return false;
            }

            STARFISH_LOG_ERROR("DemuxerFFmpeg::findStreamInfo avformat_open_input: ok.\n");
        }
/*
        if ((ret = avformat_find_stream_info(m_formatContext, NULL)) < 0) {
            char error[128];
            av_strerror(ret, error, 128);
            STARFISH_LOG_ERROR("DemuxerFFmpeg::findStreamInfo avformat_find_stream_info: Error(%s)\n", error);
            return false;
        }
*/
        STARFISH_LOG_ERROR("DemuxerFFmpeg::findStreamInfo avformat_find_stream_info: ok(duration %d).\n", (int)m_formatContext->duration / 1000);

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

        // avio_flush(m_formatContext->pb);
        // avformat_flush(m_formatContext);
        m_isStreamFinded = true;

        return true;
    }

    virtual bool findStreamPacket(DemuxerSource* source)
    {
        DemuxerSourceController c(*this, source);

        // avformat_flush(m_formatContext);
        // avio_flush(m_ioContext);
        av_seek_frame(m_formatContext, -1, 0, AVSEEK_FLAG_ANY);

        int ret;

        AVPacket avPacket;
        avPacket.size = 0;
        avPacket.data = NULL;
        av_init_packet(&avPacket);

        while ((ret = av_read_frame(m_formatContext, &avPacket)) == 0) {
            uint64_t pts = av_q2d(m_formatContext->streams[avPacket.stream_index]->time_base) * avPacket.pts * 1000;
            // STARFISH_LOG_INFO("DemuxerFFmpeg::process av_read_frame streamIndex(%d, %dbyte, %dms)\n", (int)avPacket.stream_index, (int)avPacket.size, (int)pts);
            for (size_t j = 0; j < m_demuxerClients.size(); j ++) {
                MediaPacket packet;
                packet.m_streamIndex = avPacket.stream_index;
                packet.m_data = avPacket.data;
                packet.m_dataSize = avPacket.size;
                packet.m_duration = 0; // TODO
                packet.m_pts = pts;
                m_demuxerClients[j]->onDetectPacket(packet);
            }
            av_free_packet(&avPacket);
        }
        
        av_free_packet(&avPacket);

        char error[128];
        av_strerror(ret, error, 128);
        STARFISH_LOG_ERROR("DemuxerFFmpeg::process av_read_frame: Error(%s)\n", error);

        return false;
    }

    virtual bool isFindedStreamInfo()
    {
        return m_isStreamFinded;
    }

    bool m_isStreamFinded;
    DemuxerSource* m_demuxerSource;
    AVFormatContext* m_formatContext;
    AVIOContext* m_ioContext;
    uint8_t* m_bufferForIO;
};

Demuxer* Demuxer::createFFmpegDemuxer()
{
    return new DemuxerFFmpeg();
}

}

#endif /* STARFISH_ENABLE_MULTIMEDIA */
