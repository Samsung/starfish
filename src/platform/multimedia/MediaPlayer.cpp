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
#include <efl_extension.h>
#include <Elementary.h>
#include <Ecore_X.h>
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
    , m_url(nullptr)
{
}

static void __player_prepare_cb(void *user_data)
{
    STARFISH_LOG_ERROR("__player_prepare_cb()\n");
    MediaPlayer* player = (MediaPlayer*)user_data;
    if (player->isReady())
        player->onPrepared(false);
    else
        player->onPrepared(true);
}

static void __player_complete_cb(void *user_data)
{
    STARFISH_LOG_ERROR("__player_complete_cb()\n");
    MediaPlayer* player = (MediaPlayer*)user_data;
    player->onPlayFinished();
}

static void __error_cb(int error_code, void *user_data)
{
    STARFISH_LOG_ERROR("__error_cb()\n");
}

void MediaPlayer::prepare(Document* document, CanvasSurface* surface, String* path)
{
    m_url = URL::createURL(document->documentURI()->urlString(), path);
    if (m_url->isFileURL() || m_url->isNetworkURL()) {
#ifdef STARFISH_TIZEN_2_4
        STARFISH_LOG_ERROR("prepare() - url : %s", m_url->urlString()->utf8Data());
        String* urlStr = m_url->urlString();
#ifdef STARFISH_TIZEN_TV
        Evas_Object* win = (Evas_Object*) document->window()->unwrap();
        player_display_h display_handle = GET_DISPLAY(elm_win_xwindow_get(win));
        int display_type = PLAYER_DISPLAY_TYPE_OVERLAY;
        int display_mode = PLAYER_DISPLAY_MODE_FULL_SCREEN;
#elif STARFISH_TIZEN_MOBILE
        player_display_h display_handle = GET_DISPLAY((Evas_Object*) surface->unwrap());
        int display_type = PLAYER_DISPLAY_TYPE_EVAS;
        int display_mode = PLAYER_DISPLAY_MODE_ORIGIN_OR_LETTER;
#endif
        if (m_player != NULL) {
            player_unprepare(m_player);
            player_destroy(m_player);
        }
        m_player = NULL;
        if (player_create(&m_player) != PLAYER_ERROR_NONE) {
            STARFISH_LOG_ERROR("prepare() - player create is failed\n");
            __player_complete_cb(this);
            return;
        }
        player_set_uri(m_player, const_cast<char*>(urlStr->utf8Data()));
        player_set_display(m_player, (player_display_type_e) display_type, display_handle);
        player_set_display_mode(m_player, (player_display_mode_e) display_mode);
        player_set_completed_cb(m_player, __player_complete_cb, (void*)this);
        player_set_error_cb(m_player, __error_cb, (void*)this);

        if (player_prepare_async(m_player, __player_prepare_cb, (void*)this) != PLAYER_ERROR_NONE) {
            STARFISH_LOG_ERROR("prepare() - prepare is failed\n");
            __player_complete_cb(this);
            return;
        }
#else
        // Fire error onPrepared
        onPrepared(true);
#endif
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

    STARFISH_LOG_ERROR("play()\n");
    player_start(m_player);
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
            STARFISH_LOG_ERROR("player_stop() - fail\n");
            return;
        }
    }
}

#else

MediaPlayer::MediaPlayer() { }

void MediaPlayer::create() { }

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
