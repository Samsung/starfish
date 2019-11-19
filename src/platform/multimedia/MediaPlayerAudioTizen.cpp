/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#if defined(STARFISH_ENABLE_WEBAUDIO)
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
    PLAYER_LOGI("%s\n", __func__);
    if (player_create(&m_player) != PLAYER_ERROR_NONE) {
        STARFISH_LOG_ERROR("failed to create a player\n");
    }

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            ((MediaPlayerAudioTizen*)obj)->~MediaPlayerAudioTizen();
        },
        NULL, NULL, NULL);
}

MediaPlayerAudioTizen::MediaPlayerAudioTizen(HTMLMediaElement* element)
    : MediaPlayerAudio(element)
{
    PLAYER_LOGI("%s\n", __func__);
    if (player_create(&m_player) != PLAYER_ERROR_NONE) {
        STARFISH_LOG_ERROR("failed to create a player\n");
    }

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            ((MediaPlayerAudioTizen*)obj)->~MediaPlayerAudioTizen();
        },
        NULL, NULL, NULL);
}

MediaPlayerAudioTizen::~MediaPlayerAudioTizen()
{
    destroy();
}

void MediaPlayerAudioTizen::setBuffer(uint8_t* buffer, uint32_t length)
{
    PLAYER_LOGI("%s\n", __func__);

    if (player_set_memory_buffer(m_player, buffer, length) !=
        PLAYER_ERROR_NONE) {
        STARFISH_LOG_ERROR("failed to set memory buffer\n");
    }
}

void MediaPlayerAudioTizen::destroy()
{
    PLAYER_LOGI("%s\n", __func__);
    if (m_player) {
        player_state_e state;
        player_get_state(m_player, &state);
        if (state == PLAYER_STATE_PLAYING) {
            player_stop(m_player);
        }

        if (player_unprepare(m_player) != PLAYER_ERROR_NONE) {
            STARFISH_LOG_ERROR("fail to unprepare player\n");
        }

        if (player_destroy(m_player) != PLAYER_ERROR_NONE) {
            STARFISH_LOG_ERROR("fail to destroy player\n");
        }
    }
}

void MediaPlayerAudioTizen::play()
{
    PLAYER_LOGI("%s\n", __func__);

    if (player_prepare(m_player) != PLAYER_ERROR_NONE) {
        STARFISH_LOG_ERROR("failed to prepare the player\n");
    }

    if (player_start(m_player) != PLAYER_ERROR_NONE) {
        STARFISH_LOG_ERROR("failed to start the player\n");
    }
}

void MediaPlayerAudioTizen::prepare(ResourceURL* url)
{
    int error = player_prepare(m_player);
    if (error != PLAYER_ERROR_NONE) {
        STARFISH_LOG_ERROR("failed to prepare the player\n");
    }
}

MediaPlayerAudio* MediaPlayerAudio::create(HTMLMediaElement* element)
{
    return new MediaPlayerAudioTizen(element);
}

MediaPlayerAudio* MediaPlayerAudio::create(AudioNode* element)
{
    return new MediaPlayerAudioTizen(element);
}
} // namespace Starfish

#endif
#endif
