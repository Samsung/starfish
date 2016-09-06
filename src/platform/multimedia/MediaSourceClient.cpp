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
#include "MediaSourceClient.h"
#include "MediaPlayer.h"
#include "extra/MediaSource.h"

extern "C" {
#include <libavformat/avformat.h>
#include "libavcodec/avcodec.h"
}

namespace StarFish {

AVIOContextWrapper::AVIOContextWrapper(char* videoData, const int videoLen)
{
    printf("construct AVIOMemContext %d\n", videoLen);
    // Output buffer
    m_bufferSize = 4096;
    m_buffer = static_cast<char*>(av_malloc(m_bufferSize));

    // Internal buffer
    m_pos = 0;
    this->m_rawData = videoData;
    this->m_dataSize = videoLen;

    m_avioctx = avio_alloc_context((unsigned char*) m_buffer, m_bufferSize, 0, this, &AVIOContextWrapper::read, &AVIOContextWrapper::write, &AVIOContextWrapper::seek);
}

AVIOContextWrapper::~AVIOContextWrapper()
{
    av_free(m_buffer);
}

int AVIOContextWrapper::read(void *opaque, unsigned char *buf, int buf_size)
{
    printf("avio read %p %p %d\n", opaque, buf, buf_size);
    AVIOContextWrapper* ctx = static_cast<AVIOContextWrapper*>(opaque);
    // Read from pos to pos + buf_size
    if (ctx->pos() + buf_size > ctx->dataSize()) {
        int len = ctx->dataSize() - ctx->pos();
        memcpy(buf, ctx->rawData() + ctx->pos(), len);
        ctx->increasePosition(len);
        return len;
    } else {
        memcpy(buf, ctx->rawData() + ctx->pos(), buf_size);
        ctx->increasePosition(buf_size);
        return buf_size;
    }
    return -1;
}

MediaSourceClient::MediaSourceClient()
    : m_isWebm(false)
{
    m_formatContext = nullptr;
    av_register_all();
    avcodec_register_all();
    avformat_network_init();
}

void MediaSourceClient::registerMediaPlayer(VideoPlayer* player)
{
    m_player = player;
}

void MediaSourceClient::setFormat(String* type)
{
    if (!m_formatContext)
        m_formatContext = avformat_alloc_context();

    if (type->startsWith("video/webm")) {
        m_formatContext->iformat = av_find_input_format("webm");
        m_isWebm = true;
    }
    m_formatContext->flags = AVFMT_FLAG_CUSTOM_IO;

#if STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE)
    String* typestr = String::emptyString;
    if (m_isWebm)
        typestr = String::fromUTF8("video/x-vp9");
    if (m_player)
        m_player->setVideoStreamInfo(typestr, 512, 288, 1, 1000);
#endif
}

void MediaSourceClient::appendBuffer(MediaRawBuffer buffer)
{
    AVIOContextWrapper mem_ctx = AVIOContextWrapper(buffer.memory, buffer.size);
    m_formatContext->pb = mem_ctx.get_avio();

    int ret;
    if ((ret = avformat_open_input(&m_formatContext, "", NULL, NULL)) < 0) {
        STARFISH_LOG_ERROR("avformat_open_input: Error(%u)\n", ret);
    }

    if ((ret = avformat_find_stream_info(m_formatContext, NULL)) < 0) {
        STARFISH_LOG_ERROR("avformat_find_stream_info: Error(%u)\n", ret);
    }
    // FIXME x64 build crash
    // m_player->notifyInitialPacketReady();
}

void VideoPlayer::onBufferNeedVideoData(MediaSource* ms)
{
    // NOTE : This function running on non-main thread. Use Mutex to protect variables.
    STARFISH_LOG_ERROR("onBufferNeedVideoData()\n");
    int video_stream_idx = 0;
    int ret;
    AVPacket avpacket;
    avpacket.size = 0;
    avpacket.data = NULL;
    av_init_packet(&avpacket);
    if (!ms->mseClient()) {
        STARFISH_LOG_ERROR("onBufferNeedVideoData() : MSEClient not ready\n");
        return;
    }
    if (!ms->mseClient()->formatContext()) {
        STARFISH_LOG_ERROR("onBufferNeedVideoData() : MSEClient's formatContext not ready\n");
        return;
    }
    while ((ret = av_read_frame(ms->mseClient()->formatContext(), &avpacket)) >= 0) {
        if (avpacket.stream_index == video_stream_idx) {
            pushVideoPacket(avpacket.data, avpacket.size, avpacket.pts);
            av_free_packet(&avpacket);
            av_init_packet(&avpacket);
            avpacket.size = 0;
            avpacket.data = NULL;
        }
    }
    STARFISH_LOG_ERROR("onBufferNeedVideoData()-end\n");
}

void VideoPlayer::onBufferNeedAudioData(MediaSource* ms)
{
    // NOTE : This function running on non-main thread. Use Mutex to protect variables.
    STARFISH_LOG_ERROR("onBufferNeedAudioData()\n");
    int audio_stream_idx = 1;
    int ret;
    AVPacket avpacket;
    avpacket.size = 0;
    avpacket.data = NULL;
    av_init_packet(&avpacket);
    if (!ms->mseClient()) {
        STARFISH_LOG_ERROR("onBufferNeedVideoData() : MSEClient not ready\n");
        return;
    }
    if (!ms->mseClient()->formatContext()) {
        STARFISH_LOG_ERROR("onBufferNeedAudioData() : MSEClient's formatContext not ready\n");
        return;
    }
    while ((ret = av_read_frame(ms->mseClient()->formatContext(), &avpacket)) >= 0) {
        if (avpacket.stream_index == audio_stream_idx) {
            pushAudioPacket(avpacket.data, avpacket.size, avpacket.pts);
            av_free_packet(&avpacket);
            av_init_packet(&avpacket);
            avpacket.size = 0;
            avpacket.data = NULL;
        }
    }
    STARFISH_LOG_ERROR("onBufferNeedAudioData()-end\n");
}

}

#endif
