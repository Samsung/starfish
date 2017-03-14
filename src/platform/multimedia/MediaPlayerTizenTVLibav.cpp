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
#ifdef STARFISH_TIZEN_TV

extern "C" {
#include <libavformat/avformat.h>
}

#include "StarFishConfig.h"
#include "util/URL.h"
#include "dom/Document.h"
#include "dom/HTMLVideoElement.h"
#include "MediaPlayerTizenTV.h"
#include "platform/canvas/Canvas.h"
#include "platform/message_loop/MessageLoop.h"
#include "extra/MediaSource.h"
#include "extra/SourceBuffer.h"

#include <media/player.h>

namespace StarFish {

struct FFMpegIOContext {
    FFMpegIOContext(const GCVector<uint8_t>& buf)
        : m_readPos(0)
        , m_buffer(buf)
    {
    }
    size_t m_readPos;
    const GCVector<uint8_t>& m_buffer;
};

static int FFMpegIOContextReadCallback(void* opaque, uint8_t* buf, int buf_size)
{
    FFMpegIOContext* self = (FFMpegIOContext*)opaque;
    int sizeSuccessToRead = buf_size;

    if (sizeSuccessToRead + self->m_readPos > self->m_buffer.size()) {
        sizeSuccessToRead = self->m_buffer.size() - self->m_readPos;
    }

    memcpy(buf, self->m_buffer.data() + self->m_readPos, sizeSuccessToRead);
    self->m_readPos += sizeSuccessToRead;

    return sizeSuccessToRead;
}

static int64_t FFMpegIOContextSeekCallback(void* opaque, int64_t offset,
                                           int whence)
{
    FFMpegIOContext* self = (FFMpegIOContext*)opaque;
    if (whence == AVSEEK_SIZE) {
        return self->m_buffer.size();
    } else if (whence == SEEK_CUR) {
        self->m_readPos += offset;
    } else if (whence == SEEK_SET) {
        self->m_readPos = offset;
    } else if (whence == SEEK_END) {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return -1;
    }
    return self->m_readPos;
}

extern bool g_ffmpegInited;

void MediaPlayerTizenTV::setVideoStreamInfo(size_t initSegmentIndex)
{
    av_register_all();
    avcodec_register_all();
    avformat_network_init();

    // set video options
    player_video_stream_info_s videoInfo;
    if (m_activeMediaSource->activeVideoSourceBuffer()) {
        STARFISH_LOG_INFO("MediaPlayerTizenTV::setVideoStreamInfo\n");
        VideoStreamInfo* info =
            (VideoStreamInfo*)(m_activeMediaSource->activeVideoSourceBuffer()
                                   ->streamInfo(
                                       0, m_activeMediaSource
                                              ->activeVideoStreamIndex()));
        memset(&videoInfo, 0, sizeof(player_video_stream_info_s));

        const char* mediaFormat = "";
        if (strstr(info->m_codecName, "h264")) {
            videoInfo.mime = "video/x-h264";
            // TODO find container type
            mediaFormat = "mp4";
        } else if (strstr(info->m_codecName, "vp9")) {
            videoInfo.mime = "video/x-vp9";
            // TODO find container type
            mediaFormat = "webm";
        } else {
            // TODO
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

        m_videoWidth = videoInfo.width = info->m_width;
        m_videoHeight = videoInfo.height = info->m_height;
        videoInfo.framerate_den = info->m_timeBaseDen;
        videoInfo.framerate_num = info->m_timeBaseNum;

        uint8_t* bufferForIO = (uint8_t*)av_malloc(4096);
        FFMpegIOContext ctx(
            m_activeMediaSource->activeVideoSourceBuffer()->bufferHeader(0));
        AVIOContext* ioContext = avio_alloc_context(
            bufferForIO, 4096, 0, &ctx, FFMpegIOContextReadCallback, nullptr,
            FFMpegIOContextSeekCallback);
        AVFormatContext* fc = avformat_alloc_context();
        fc->flags = AVFMT_FLAG_CUSTOM_IO;
        fc->pb = ioContext;
        fc->iformat = av_find_input_format(mediaFormat);
        STARFISH_RELEASE_ASSERT(avformat_open_input(&fc, NULL, NULL, NULL) ==
                                0);
        videoInfo.codec_extradata =
            fc->streams[m_activeMediaSource->activeVideoStreamIndex()]
                ->codec->extradata;
        videoInfo.extradata_size =
            fc->streams[m_activeMediaSource->activeVideoStreamIndex()]
                ->codec->extradata_size;
        STARFISH_LOG_INFO(
            "ffmpegVideo Info[%d].. %d %d\n",
            (int)m_activeMediaSource->activeVideoStreamIndex(),
            (int)fc->streams[m_activeMediaSource->activeVideoStreamIndex()]
                ->codec->width,
            (int)fc->streams[m_activeMediaSource->activeVideoStreamIndex()]
                ->codec->height);
        STARFISH_LOG_INFO("tizen video Info.. %d %d %d %d\n", info->m_width,
                          info->m_height, (int)videoInfo.framerate_den,
                          (int)videoInfo.framerate_num);

        int ret = player_set_video_stream_info(m_nativePlayer, &videoInfo);
        STARFISH_RELEASE_ASSERT(ret == 0);

        avformat_close_input(&fc);
        av_free(bufferForIO);
        av_free(ioContext);

        m_videoInitSegmentIndex = initSegmentIndex;
    }
}

void MediaPlayerTizenTV::setAudioStreamInfo(size_t initSegmentIndex)
{
    av_register_all();
    avcodec_register_all();
    avformat_network_init();

    // set audio options
    player_audio_stream_info_s audioInfo;
    if (m_activeMediaSource->activeAudioSourceBuffer()) {
        STARFISH_LOG_INFO("MSE set Audio\n");
        memset(&audioInfo, 0, sizeof(player_audio_stream_info_s));

        StreamInfo* info =
            (m_activeMediaSource->activeAudioSourceBuffer()->streamInfo(
                initSegmentIndex,
                m_activeMediaSource->activeAudioStreamIndex()));

        const char* mediaFormat = "";
        if (strstr(info->m_codecName, "aac")) {
            audioInfo.mime = "audio/mpeg";
            // TODO find container type
            mediaFormat = "mp4";
        } else if (strstr(info->m_codecName, "vorbis")) {
            audioInfo.mime = "audio/x-vorbis";
            // TODO find container type
            mediaFormat = "webm";
        } else {
            // TODO
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

        uint8_t* bufferForIO = (uint8_t*)av_malloc(4096);
        FFMpegIOContext ctx(
            m_activeMediaSource->activeAudioSourceBuffer()->bufferHeader(
                initSegmentIndex));
        AVIOContext* ioContext = avio_alloc_context(
            bufferForIO, 4096, 0, &ctx, FFMpegIOContextReadCallback, nullptr,
            FFMpegIOContextSeekCallback);
        AVFormatContext* fc = avformat_alloc_context();
        fc->flags = AVFMT_FLAG_CUSTOM_IO;
        fc->pb = ioContext;
        fc->iformat = av_find_input_format(mediaFormat);
        STARFISH_RELEASE_ASSERT(avformat_open_input(&fc, NULL, NULL, NULL) ==
                                0);

        AVStream* audioStream =
            fc->streams[m_activeMediaSource->activeAudioStreamIndex()];
        AVCodecContext* audioCodecCtx =
            fc->streams[m_activeMediaSource->activeAudioStreamIndex()]->codec;
        audioInfo.channels = audioCodecCtx->channels;
        audioInfo.sample_rate = audioCodecCtx->sample_rate;
        audioInfo.bit_rate = audioCodecCtx->bit_rate;
        audioInfo.version = 2;
        audioInfo.user_info = 0;
        audioInfo.codec_extradata = audioCodecCtx->extradata;
        audioInfo.extradata_size = audioCodecCtx->extradata_size;

        int ret = player_set_audio_stream_info(m_nativePlayer, &audioInfo);
        STARFISH_RELEASE_ASSERT(ret == 0);

        avformat_close_input(&fc);
        av_free(bufferForIO);
        av_free(ioContext);

        m_audioInitSegmentIndex = initSegmentIndex;
    }
}
}

#endif
#endif
