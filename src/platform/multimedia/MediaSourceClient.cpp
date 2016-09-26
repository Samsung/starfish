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
#include <libavcodec/avcodec.h>
}

#define PUSH_DURATION_PER_CB 2 // second

namespace StarFish {

AVIOContextWrapper::AVIOContextWrapper(MediaRawData data)
{
    STARFISH_LOG_ERROR("construct AVIOMemContext %d\n", (int)data.size);
    // Output buffer
    m_bufferSize = 4096;
    m_buffer = static_cast<char*>(av_malloc(m_bufferSize));
    if (!m_buffer) {
        STARFISH_LOG_ERROR("av_malloc is failed\n");
        return;
    }

    // Internal buffer
    m_pos = 0;
    m_rawDataList.clear();
    m_rawDataList.push_back(data);
    m_totalSize = data.size;
    m_curListIdx = 0;

    m_avioctx = avio_alloc_context((unsigned char*) m_buffer, m_bufferSize, 0, this, &AVIOContextWrapper::read, &AVIOContextWrapper::write, &AVIOContextWrapper::seek);
}

AVIOContextWrapper::~AVIOContextWrapper()
{
    av_free(m_buffer);
    av_free(m_avioctx);
}

void AVIOContextWrapper::pushMediaData(MediaRawData data)
{
    m_rawDataList.push_back(data);
    m_totalSize += data.size;
}

int AVIOContextWrapper::read(void *opaque, unsigned char *buf, int buf_size)
{
    AVIOContextWrapper* ctx = static_cast<AVIOContextWrapper*>(opaque);
    printf("[AVIOContextWrapper::read] buf %d pos %d of dataSize %d (%p)\n", buf_size, ctx->pos(), ctx->dataSize(), ctx->rawData());
    // Read from pos to pos + buf_size
    if (ctx->pos() + buf_size > ctx->dataSize()) {
        int len = ctx->dataSize() - ctx->pos();
        memcpy(buf, ctx->rawData() + ctx->pos(), len);
        ctx->increasePosition(len);
        if (ctx->moveToNextDataIfPossible()) {
            STARFISH_LOG_ERROR("----[AVIOContextWrapper::read] move next buffer\n");
        }
        return len;
    } else {
        memcpy(buf, ctx->rawData() + ctx->pos(), buf_size);
        ctx->increasePosition(buf_size);
        return buf_size;
    }
    return -1;
}

MediaSourceClient::MediaSourceClient()
    : m_formatContext(nullptr)
    , m_ioContext(nullptr)
    , m_videoStreamIdx(0)
    , m_audioStreamIdx(1)
    , m_subtitleStreamIdx(0)
    , m_isWebm(false)
    , m_isReady(false)
{
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

    if (!m_formatContext) {
        STARFISH_LOG_ERROR("[MediaSourceClient::setFormat] avformat context is not found\n");
        return;
    }

    if (type->startsWith("video/webm") || type->startsWith("audio/webm")) {
        m_formatContext->iformat = av_find_input_format("webm");
        m_isWebm = true;
    }
    m_formatContext->flags = AVFMT_FLAG_CUSTOM_IO;

#if STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE)
    String* typestr = String::emptyString;
    if (m_isWebm)
        typestr = String::fromUTF8("video/x-vp9");
    if (m_player)
        m_player->setVideoStreamInfo(typestr, 512, 288, 100, 2997);
#endif
}

void MediaSourceClient::appendBuffer(MediaRawData buffer)
{
    if (m_ioContext) {
        m_ioContext->pushMediaData(buffer);
    } else {
        m_ioContext = new AVIOContextWrapper(buffer);
        if (!m_ioContext || !m_ioContext->get_avio()) {
            STARFISH_LOG_ERROR("[appendBuffer] AVIOContextWrapper is not found\n");
            return;
        }
    }

    if (!m_formatContext) {
        STARFISH_LOG_ERROR("[appendBuffer] avformat context is not found\n");
        return;
    }

    // FIXME: wait until real video data is received (to support shaka)
    if (m_isReady || m_ioContext->totalSize() < 2048)
        return;

    m_formatContext->pb = m_ioContext->get_avio();
    int ret;
    if ((ret = avformat_open_input(&m_formatContext, "", NULL, NULL)) < 0) {
        STARFISH_LOG_ERROR("[appendBuffer] avformat_open_input: Error(%u)\n", ret);
        return;
    }

    if ((ret = avformat_find_stream_info(m_formatContext, NULL)) < 0) {
        STARFISH_LOG_ERROR("[appendBuffer] avformat_find_stream_info: Error(%u)\n", ret);
        return;
    }

    for (int i = 0; i < (int)m_formatContext->nb_streams; i++) {
        STARFISH_ASSERT(m_formatContext->streams[i]->codec);
        if (m_formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoStreamIdx = i;
        } else if (m_formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_audioStreamIdx = i;
        } else if (m_formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE) {
            m_subtitleStreamIdx = i;
        }
    }
    setReady();
}

void VideoPlayer::onBufferNeedVideoData(MediaSource* ms)
{
    // NOTE : This function running on non-main thread. Use Mutex to protect variables.
    STARFISH_LOG_ERROR("onBufferNeedVideoData()\n");
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
    int video_stream_idx = ms->mseClient()->videoStreamIdx();
    STARFISH_LOG_ERROR("onBufferNeedVideoData(%d)\n", video_stream_idx);
    // FIXME : stream's time_base returns wrong value (2016.09.08),
    // AVRational timeBase = ms->mseClient()->formatContext()->streams[video_stream_idx]->time_base;
    AVRational timeBase = { 1, 30 };
    int64_t threshold = timeBase.den * PUSH_DURATION_PER_CB;
    int64_t initialPts = AV_NOPTS_VALUE;
    while ((ret = av_read_frame(ms->mseClient()->formatContext(), &avpacket)) >= 0) {
        if (avpacket.stream_index == video_stream_idx) {
            pushVideoPacket(avpacket.data, avpacket.size, avpacket.pts);
            if (initialPts == (int64_t)AV_NOPTS_VALUE)
                initialPts = avpacket.pts;
            if ((avpacket.pts - initialPts) * timeBase.num >= threshold) {
                break;
            }
            avpacket.size = 0;
            avpacket.data = NULL;
        }
    }
    av_free_packet(&avpacket);
    // STARFISH_LOG_ERROR("onBufferNeedVideoData()-end\n");
}

void VideoPlayer::onBufferNeedAudioData(MediaSource* ms)
{
    // NOTE : This function running on non-main thread. Use Mutex to protect variables.
    STARFISH_LOG_ERROR("onBufferNeedAudioData()\n");
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
    // TODO : push packets for 2 sec
    int audio_stream_idx = ms->mseClient()->audioStreamIdx();
    // FIXME : stream's time_base returns wrong value (2016.09.08),
    // AVRational timeBase = ms->mseClient()->formatContext()->streams[audio_stream_idx]->time_base;
    AVRational timeBase = { 1, 30 };
    int64_t threshold = timeBase.den * PUSH_DURATION_PER_CB;
    int64_t initialPts = AV_NOPTS_VALUE;
    while ((ret = av_read_frame(ms->mseClient()->formatContext(), &avpacket)) >= 0) {
        if (avpacket.stream_index == audio_stream_idx) {
            pushAudioPacket(avpacket.data, avpacket.size, avpacket.pts);
            if (initialPts == (int64_t)AV_NOPTS_VALUE)
                initialPts = avpacket.pts;
            if ((avpacket.pts - initialPts) * timeBase.num >= threshold) {
                break;
            }
            avpacket.size = 0;
            avpacket.data = NULL;
        }
    }
    av_free_packet(&avpacket);
    // STARFISH_LOG_ERROR("onBufferNeedAudioData()-end\n");
}

}

#endif
