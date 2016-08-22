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

#include <limits.h>
#include <stdlib.h>

#include "StarFishConfig.h"
#include "MediaPlayer.h"
#include "util/URL.h"
#include "dom/Document.h"

#ifdef STARFISH_TIZEN
#include <player.h>
#include <efl_extension.h>
#endif

namespace StarFish {

#if STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE)
bool MediaPlayer::isReady()
{
    if (!m_player)
        return false;

    player_state_e state;
    int error_code = player_get_state(m_player, &state);
    if (PLAYER_ERROR_NONE != error_code) {
        return false;
    }
    return (state == PLAYER_STATE_READY);
}

MediaPlayer::MediaPlayer()
    : m_player(NULL)
    , m_url(nullptr) { }

static void __player_prepare(void *user_data)
{
    MediaPlayer* player = (MediaPlayer*)user_data;
    STARFISH_ASSERT(player->isReady());
    player->onPrepared(false);
}

void MediaPlayer::prepare(Document* document, CanvasSurface* surface, String* path)
{
    Evas_Object* player_display = (Evas_Object*) surface->unwrap();

    // Create a handle to the player.
    if (!m_player) {
        int error_code = player_create(&m_player);
        if (PLAYER_ERROR_NONE != error_code) {
            STARFISH_LOG_ERROR("ERROR(%d) : player_create()", error_code);
            return;
        }
    }

    // Get the path to the file which will be used for playback.
    m_url = URL::createURL(document->documentURI()->urlString(), path);
    STARFISH_LOG_ERROR("url : %s", m_url->urlString()->utf8Data());

    if (m_url->isFileURL() || m_url->isNetworkURL()) {
        // Set the obtained file path as a source for the Player.
        int error_code = player_set_uri(m_player, m_url->urlString()->utf8Data());
        if (PLAYER_ERROR_NONE != error_code) {
            STARFISH_LOG_ERROR("ERROR(%d) : player_set_uri()", error_code);
            return;
        }

        // Set the display for the video.
        error_code = player_set_display(m_player, PLAYER_DISPLAY_TYPE_EVAS, GET_DISPLAY(player_display));
        if (PLAYER_ERROR_NONE != error_code) {
            STARFISH_LOG_ERROR("ERROR(%d) : player_set_display()", error_code);
            return;
        }

        error_code = player_prepare_async(m_player, __player_prepare, this);
        if (PLAYER_ERROR_NONE != error_code) {
            STARFISH_LOG_ERROR("ERROR(%d) : player_prepare_async()", error_code);
            onPrepared(true);
        }
    }

}

void MediaPlayer::destroy()
{
    if (!m_player)
        return;

    // Stop playing the video.
    player_stop(m_player);

    // Reset the player handle.
    player_unprepare(m_player);

    // Release the memory allocated for the player instance.
    player_destroy(m_player);
    m_player = NULL;    
}

void MediaPlayer::play()
{
    if (!m_player)
        return;

    if (isReady()) {
        STARFISH_LOG_ERROR("player_start()");
        int error_code = player_start(m_player);
        if (PLAYER_ERROR_NONE != error_code) {
            STARFISH_LOG_ERROR("ERROR(%d) : player_start()", error_code);
            return;
        }
    }
    // Check the current state of the player.
    player_state_e state;
    int error_code = player_get_state(m_player, &state);
    if (PLAYER_ERROR_NONE != error_code) {
        STARFISH_LOG_ERROR("ERROR(%d) : player_get_state()", error_code);
        return;
    }
}

void MediaPlayer::pause()
{
    if (!m_player)
        return;
    // TODO
}

void MediaPlayer::stop()
{
    if (!m_player)
        return;

    player_state_e state;
    int error_code = player_get_state(m_player, &state);
    if (PLAYER_ERROR_NONE != error_code) {
        return;
    }
    if (state == PLAYER_STATE_PLAYING || state == PLAYER_STATE_PAUSED) {
        int error_code = player_stop(m_player);
        if (PLAYER_ERROR_NONE != error_code) {
            STARFISH_LOG_ERROR("ERROR(%d) : player_stop()", error_code);
            return;
        }
    }
}

#else

MediaPlayer::MediaPlayer()
{ }

bool MediaPlayer::isReady()
{
    // TODO
    return false;
}

void MediaPlayer::prepare(Document* document, CanvasSurface* surface, String* path)
{
    // TODO
}

void MediaPlayer::play()
{
    // TODO
}

void MediaPlayer::pause()
{
    // TODO
}

void MediaPlayer::stop()
{
    // TODO
}

void MediaPlayer::destroy()
{
    // TODO
}
#endif
}
#endif
