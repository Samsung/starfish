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
#include "dom/HTMLVideoElement.h"
#include "platform/message_loop/MessageLoop.h"

#ifdef STARFISH_TIZEN
#include <efl_extension.h>
#include <Elementary.h>
#include <Ecore_X.h>
#endif

#define PLAYER_LOG(...) \
    STARFISH_LOG_ERROR("[MediaPlayer] "); \
    STARFISH_LOG_ERROR(__VA_ARGS__);

namespace StarFish {

MediaPlayer::MediaPlayer()
    : m_url(nullptr)
    , m_lastRequest(MediaPlayer::REQUEST_NONE)
{
}

#if STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE)
VideoPlayer::VideoPlayer(HTMLVideoElement* videoElement)
    : MediaPlayer()
    , m_videoElement(videoElement)
    , m_player(NULL)
#ifdef STARFISH_TIZEN_TV
    , m_displayArea(Rect(0, 0, 0, 0))
    , m_videoMixerHandle(-1)
#elif STARFISH_TIZEN_MOBILE
    , m_surface(nullptr)
#endif
{
}

static void __video_player_prepare_cb(void *user_data)
{
    // NOTE : this callback will not be invoked when using unprepare player api
    PLAYER_LOG("__video_player_prepare_cb()\n");
    VideoPlayer* player = (VideoPlayer*)user_data;
    player->videoElement()->document()->window()->starFish()->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t, void* data) {
        VideoPlayer* player = (VideoPlayer*)data;
        player->postPrepare();
    }, player);
}

static void __video_player_complete_cb(void *user_data)
{
    PLAYER_LOG("__video_player_complete_cb()\n");
    VideoPlayer* player = (VideoPlayer*)user_data;
    player->videoElement()->document()->window()->starFish()->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t, void* data) {
        VideoPlayer* player = (VideoPlayer*)data;
        player->postPlayFinished();
    }, player);
}

static void __error_cb(int error_code, void *user_data)
{
    PLAYER_LOG("__error_cb()\n");
}

void VideoPlayer::postPrepare()
{
    if (isReady())
        onPrepared(false);
    else
        onPrepared(true);

#ifdef STARFISH_TIZEN_TV
    // Debug code
    if (m_player) {
        player_video_track_info info;
        int error_code = player_get_video_info(m_player, &info);
        if (error_code != PLAYER_ERROR_NONE) {
            PLAYER_LOG("player_get_video_info() is failed(%d)\n", error_code);
        }
        PLAYER_LOG("video info----------------------------------------\n");
        PLAYER_LOG("- resolution : %u x %u\n", info.width, info.height);
        PLAYER_LOG("- bitrate : %u bps\n", info.bitrate);
        PLAYER_LOG("--------------------------------------------------\n");
    }
#endif

    // NOTE: setLoop() does not work well,
    // if (m_videoElement)
    //     setLoop(m_videoElement->loop());

    if (lastRequestIs(MediaPlayer::REQUEST_PLAY))
        playInternal();
}

void VideoPlayer::postPlayFinished()
{
    // NOTE: player state is not READY
    onPlayFinished();
}

void VideoPlayer::prepareAsync(Document* document, String* path)
{
    STARFISH_ASSERT(m_videoElement);
    destroyInternal();
    m_videoElement->document()->window()->starFish()->addPointerInRootSet(this);

    m_url = URL::createURL(document->documentURI()->urlString(), path);
    if (m_url->isFileURL() || m_url->isNetworkURL()) {
        PLAYER_LOG("prepare() - url : %s", m_url->urlString()->utf8Data());
        String* urlStr = m_url->urlString();
#ifdef STARFISH_TIZEN_TV
        Evas_Object* win = (Evas_Object*) document->window()->unwrap();
        player_display_h display_handle = GET_DISPLAY(elm_win_xwindow_get(win));
        // int display_type = PLAYER_DISPLAY_TYPE_OVERLAY;
        int display_type = PLAYER_DISPLAY_TYPE_X11;
        int display_mode = PLAYER_DISPLAY_MODE_ORIGIN_OR_LETTER;
#elif STARFISH_TIZEN_MOBILE
        if (!m_surface)
            return;
        player_display_h display_handle = GET_DISPLAY((Evas_Object*) m_surface->unwrap());
        int display_type = PLAYER_DISPLAY_TYPE_EVAS;
        int display_mode = PLAYER_DISPLAY_MODE_ORIGIN_OR_LETTER;
#endif
        if (player_create(&m_player) != PLAYER_ERROR_NONE) {
            PLAYER_LOG("prepareAsync() - player create is failed\n");
            return;
        }
        player_set_uri(m_player, const_cast<char*>(urlStr->utf8Data()));
        player_set_display(m_player, (player_display_type_e) display_type, display_handle);
        player_set_display_mode(m_player, (player_display_mode_e) display_mode);
        player_set_completed_cb(m_player, __video_player_complete_cb, (void*)this);
        player_set_error_cb(m_player, __error_cb, (void*)this);
#ifdef STARFISH_TIZEN_TV
        player_video_mixer_open(m_player, PLAYER_MIXER_DECODER_HW_MAIN);
        // player_video_mixer_get_instance(m_player, &m_videoMixerHandle);
#endif
        if (player_prepare_async(m_player, __video_player_prepare_cb, (void*)this) != PLAYER_ERROR_NONE) {
            PLAYER_LOG("prepareAsync() - prepare is failed\n");
            return;
        }
    }
}

void VideoPlayer::destroy()
{
    MediaPlayer::destroy();
    destroyInternal();
}

void VideoPlayer::destroyInternal()
{
    if (!m_player)
        return;

    // Stop playing the video.
    if (isPlaying() || isPaused()) {
        stopInternal();
    }

    // Reset the player handle.
    if (isReady()) {
        player_unprepare(m_player);
    }

#ifdef STARFISH_TIZEN_TV
    if (m_videoMixerHandle != -1)
        player_video_mixer_close(m_player, m_videoMixerHandle);
#endif

    // Release the memory allocated for the player instance.
    player_destroy(m_player);
    m_player = NULL;

    STARFISH_ASSERT(m_videoElement);
    m_videoElement->document()->window()->starFish()->removePointerFromRootSet(this);
}

#ifdef STARFISH_TIZEN_TV
void VideoPlayer::setDisplayArea(int x, int y, int width, int height)
{
    m_displayArea = Rect(x, y, width, height);
    if (m_player && isPlaying()) {
        if (m_videoMixerHandle == -1)
            player_video_mixer_get_instance(m_player, &m_videoMixerHandle);
        int mixer_display_handle = -1;
        player_video_mixer_set_displayarea(m_player, m_videoMixerHandle, &mixer_display_handle, m_displayArea.x(), m_displayArea.y(), m_displayArea.width(), m_displayArea.height()); 
    }
}
#elif STARFISH_TIZEN_MOBILE
void VideoPlayer::setVideoSurface(CanvasSurface* surface)
{
    m_surface = surface;
}
#endif

void VideoPlayer::play()
{
    MediaPlayer::play();
    if (isReady() || isPaused())
        playInternal();
}

void VideoPlayer::playInternal()
{
    STARFISH_ASSERT(m_player);
    STARFISH_ASSERT(isReady() || isPaused());
#ifdef STARFISH_TIZEN_TV
    if (m_videoMixerHandle == -1)
        player_video_mixer_get_instance(m_player, &m_videoMixerHandle);
    // player_video_mixer_set_resolution_of_mixedframe(m_player, videomixerhandle, 1920, 1080);
    // player_video_mixer_set_position_of_mixedframe(m_player, videomixerhandle, 0, 0, 1920, 1080);
    int mixer_display_handle = -1;
    player_video_mixer_set_displayarea(m_player, m_videoMixerHandle, &mixer_display_handle, m_displayArea.x(), m_displayArea.y(), m_displayArea.width(), m_displayArea.height());
    PLAYER_LOG("video display area : %f %f %f %f\n", m_displayArea.x(), m_displayArea.y(), m_displayArea.width(), m_displayArea.height());
#endif
    PLAYER_LOG("play()\n");
    // int error_code = player_start_async(m_player, __player_start_completed_cb, (void*)this);
    int error_code = player_start(m_player);
    if (PLAYER_ERROR_NONE != error_code) {
        PLAYER_LOG("player_start() - fail(%d)\n", error_code);
        return;
    }
}

void VideoPlayer::pause()
{
    MediaPlayer::pause();
    if (isPlaying())
        pauseInternal();
}

void VideoPlayer::pauseInternal()
{
    STARFISH_ASSERT(m_player);
    STARFISH_ASSERT(isPlaying());
    int error_code = player_pause(m_player);
}

void VideoPlayer::stop()
{
    MediaPlayer::stop();
    if (isPlaying() || isPaused())
        stopInternal();
}

void VideoPlayer::stopInternal()
{
    STARFISH_ASSERT(m_player);
    STARFISH_ASSERT(isPlaying() || isPaused());
    int error_code = player_stop(m_player);
    if (PLAYER_ERROR_NONE != error_code) {
        PLAYER_LOG("player_stop() - fail(%d)\n", error_code);
        return;
    }
}

void VideoPlayer::setLoop(bool loop)
{
    if (!m_player) {
        return;
    }
    int error_code = player_set_looping(m_player, loop);
    if (PLAYER_ERROR_NONE != error_code) {
        PLAYER_LOG("player_set_looping() - fail(%d)\n", error_code);
        return;
    }
}

bool VideoPlayer::isReady()
{
#define PLAYER_GET_STATE() \
    if (!m_player) \
        return false; \
    player_state_e state; \
    int error_code = player_get_state(m_player, &state); \
    if (PLAYER_ERROR_NONE != error_code) { \
        return false; \
    }
    PLAYER_GET_STATE();
    return (state == PLAYER_STATE_READY);
}

bool VideoPlayer::isPlaying()
{
    PLAYER_GET_STATE();
    return (state == PLAYER_STATE_PLAYING);
}

bool VideoPlayer::isPaused()
{
    PLAYER_GET_STATE();
    return (state == PLAYER_STATE_PAUSED);
#undef PLAYER_GET_STATE
}

int VideoPlayer::width()
{
    if (!m_player)
        return 0;

#ifdef STARFISH_TIZEN_TV
    player_video_track_info info;
    int error_code = player_get_video_info(m_player, &info);
    if (error_code != PLAYER_ERROR_NONE) {
        PLAYER_LOG("player_get_video_info() is failed(%d)\n", error_code);
    }
    return info.width;
#else
    // TODO
    return 0;
#endif
}

int VideoPlayer::height()
{
    if (!m_player)
        return 0;
#ifdef STARFISH_TIZEN_TV
    player_video_track_info info;
    int error_code = player_get_video_info(m_player, &info);
    if (error_code != PLAYER_ERROR_NONE) {
        PLAYER_LOG("player_get_video_info() is failed(%d)\n", error_code);
    }
    return info.height;
#else
    // TODO
    return 0;
#endif
}

#endif /* STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE) */
#undef PLAYER_LOG
}
#endif /* STARFISH_ENABLE_MULTIMEDIA */
