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

extern "C" {
#include <libavformat/avformat.h>
#include "libavcodec/avcodec.h"
}

namespace StarFish {

MediaSourceClient::MediaSourceClient()
    : m_isWebm(false)
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

void VideoPlayer::onBufferNeedVideoData(MediaSource* ms)
{
    // NOTE : This function running on non-main thread. Use Mutex to protect variables.

    // HINT
    // void pushVideoPacket(uint8_t *buf, uint32_t len, uint64_t pts)
}

void VideoPlayer::onBufferNeedAudioData(MediaSource* ms)
{
    // NOTE : This function running on non-main thread. Use Mutex to protect variables.

    // HINT
    // void pushAudioPacket(uint8_t *buf, uint32_t len, uint64_t pts)
}

}

#endif
