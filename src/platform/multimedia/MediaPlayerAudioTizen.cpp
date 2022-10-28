/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#if defined(STARFISH_ENABLE_MULTIMEDIA) && defined(STARFISH_ENABLE_WEBAUDIO)
#if !defined(STARFISH_USE_MOCK_MEDIAPLAYER) && defined(STARFISH_TIZEN)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "MediaPlayerAudioTizen.h"

#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/message_loop/MessageLoop.h"

#include "core/dom/HTMLMediaElement.h"
#include "core/dom/HTMLAudioElement.h"
#include "core/modules/webaudio/AudioBufferSourceNode.h"

namespace Starfish {
MediaPlayerAudioTizen::MediaPlayerAudioTizen(AudioNode* element)
    : MediaPlayerAudio(element)
{
    PLAYER_LOGI("MediaPlayerAudioTizen::%s(node)", __func__);
}

MediaPlayerAudioTizen::MediaPlayerAudioTizen(HTMLMediaElement* element)
    : MediaPlayerAudio(element)
{
    PLAYER_LOGI("MediaPlayerAudioTizen::%s(element)", __func__);
}

MediaPlayerAudioTizen::~MediaPlayerAudioTizen()
{
    PLAYER_LOGI("MediaPlayerAudioTizen::%s", __func__);
    destroy();
}

void MediaPlayerAudioTizen::destroy()
{
    PLAYER_LOGI("MediaPlayerAudioTizen::%s", __func__);
    MediaPlayerAudio::destroy();

    if (m_audioOut) {
        audio_out_unprepare(m_audioOut);
        audio_out_destroy(m_audioOut);
    }
}

void MediaPlayerAudioTizen::play()
{
    PLAYER_LOGI("MediaPlayerAudioTizen::%s", __func__);

    audio_out_write(m_audioOut, m_audioData.data(), m_audioData.size());
}

void MediaPlayerAudioTizen::prepare(ResourceURL* url)
{
    PLAYER_LOGI("MediaPlayerAudioTizen::%s", __func__);

    if (m_container == nullptr) {
        STARFISH_LOG_WARN("MediaPlayerAudioTizen::%s: container is null",
                          __func__);
        return;
    }

    if (!url->urlString()->endsWith(".wav", false)) {
        STARFISH_LOG_WARN("MediaPlayerAudioTizen::%s: format not supported",
                          __func__);
        return;
    }

    downloadAudioData(url);
}

void MediaPlayerAudioTizen::onAudioDownloadCompleted()
{
    int error_code = SOUND_MANAGER_ERROR_NONE;

    sound_stream_info_h streamInfo = nullptr;
    error_code = sound_manager_create_stream_information(
        SOUND_STREAM_TYPE_NOTIFICATION, nullptr, nullptr, &streamInfo);

    if (SOUND_MANAGER_ERROR_NONE != error_code) {
        STARFISH_LOG_ERROR(
            "sound_manager_create_stream_information has problem. , error "
            "code: %d\n",
            error_code);
        return;
    }

    error_code = AUDIO_IO_ERROR_NONE;
    error_code = audio_out_create_new(AUDIO_SAMPLE_RATE, AUDIO_CHANNEL_STEREO,
                                      AUDIO_SAMPLE_TYPE_S16_LE, &m_audioOut);

    if (AUDIO_IO_ERROR_NONE != error_code) {
        STARFISH_LOG_ERROR(
            "audio_out_create_new has problem. , error code: %d\n", error_code);
        return;
    }

    error_code = audio_out_set_sound_stream_info(m_audioOut, streamInfo);

    if (AUDIO_IO_ERROR_NONE != error_code) {
        STARFISH_LOG_ERROR(
            "audio_out_set_sound_stream_info has problem. , error code: %d\n",
            error_code);
        return;
    }

    error_code = audio_out_prepare(m_audioOut);

    if (AUDIO_IO_ERROR_NONE != error_code) {
        STARFISH_LOG_ERROR("audio_out_prepare has problem. , error code: %d\n",
                           error_code);
        return;
    }

    MessageLoop* msgLoop = m_container->webView()->messageLoop();
    msgLoop->addIdler(
        m_container->window(),
        [](size_t, void* data) {
            MediaPlayerAudioTizen* self = (MediaPlayerAudioTizen*)data;
            self->processNextOperationQueueInContainer();
            self->container()->mediaPlayerNotifyUpdateReadyStateItsContainer(
                HTMLMediaElement::HAVE_METADATA);
            self->container()->mediaPlayerNotifyUpdateReadyStateItsContainer(
                HTMLMediaElement::HAVE_ENOUGH_DATA);
        },
        this);
}

MediaPlayerAudio* MediaPlayerAudio::create(HTMLMediaElement* element)
{
    PLAYER_LOGI("MediaPlayerAudioTizen::%s", __func__);

    return new MediaPlayerAudioTizen(element);
}

MediaPlayerAudio* MediaPlayerAudio::create(AudioNode* element)
{
    PLAYER_LOGI("MediaPlayerAudioTizen::%s", __func__);

    return new MediaPlayerAudioTizen(element);
}
} // namespace Starfish

#endif
#endif
