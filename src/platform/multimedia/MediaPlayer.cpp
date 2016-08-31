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

#define PLAYER_LOGI(...) \
    STARFISH_LOG_ERROR("[MediaPlayer] "); \
    STARFISH_LOG_ERROR(__VA_ARGS__);

#define PLAYER_LOGE(errorcode, ...) \
    STARFISH_LOG_ERROR("[MediaPlayer] ERROR(%u)| ", errorcode); \
    STARFISH_LOG_ERROR(__VA_ARGS__);

namespace StarFish {

MediaPlayer::MediaPlayer()
    : m_inputUrl(nullptr)
    , m_state(MediaPlayer::STATE_NONE)
{
}

#if STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE)
VideoPlayer::VideoPlayer(HTMLVideoElement* videoElement)
    : MediaPlayer()
    , m_videoElement(videoElement)
    , m_currentUrl(nullptr)
    , m_cplayer(NULL)
    , m_lastRequest(VideoPlayer::REQUEST_NONE)
#ifdef STARFISH_TIZEN_TV
    , m_displayArea(Rect(0, 0, 0, 0))
#elif STARFISH_TIZEN_MOBILE
    , m_surface(nullptr)
#endif
    , m_isElementPointerLocked(false)
    , m_hasPendingUrl(false)
{
    GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
        PLAYER_LOGI("VideoPlayer::~VideoPlayer\n");
        VideoPlayer* player = (VideoPlayer*)obj;
        STARFISH_ASSERT(!player->isPublicState(MediaPlayer::STATE_PREPARING | MediaPlayer::STATE_PLAYING));
        player->destroyCPlayer();
    }, NULL, NULL, NULL);

    STARFISH_ASSERT(videoElement);
    // assureCPlayer();
}

static void __videoPlayerPrepareCB(void *user_data)
{
    // NOTE : this callback will not be invoked when using unprepare player api
    PLAYER_LOGI("__videoPlayerPrepareCB()\n");
    VideoPlayer* player = (VideoPlayer*)user_data;
    player->videoElement()->document()->window()->starFish()->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t, void* data) {
        ((VideoPlayer*)data)->postLoaded();
    }, player);
}

static void __videoPlayerCompleteCB(void *user_data)
{
    PLAYER_LOGI("__videoPlayerCompleteCB()\n");
    VideoPlayer* player = (VideoPlayer*)user_data;
    player->videoElement()->document()->window()->starFish()->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t, void* data) {
        ((VideoPlayer*)data)->postPlayFinished();
    }, player);
}

static void __videoPlayerErrorCB(int errorCode, void *user_data)
{
    PLAYER_LOGI("__videoPlayerErrorCB()\n");
}

bool VideoPlayer::assureCPlayer()
{
    // If there was an error, destroy cplayer
    if (m_cplayer && isPublicState(MediaPlayer::STATE_UNKNOWN_ERROR)) {
        destroyCPlayer();
    }
    // Create if cplayer is null
    if (!m_cplayer) {
        setPublicState(MediaPlayer::STATE_NONE);

        int errorCode = player_create(&m_cplayer);
        if (errorCode != PLAYER_ERROR_NONE) {
            PLAYER_LOGE(errorCode, "player_create()\n");
            m_cplayer = NULL;
            return false;
        }
        // Set looping
        setLoop(m_videoElement->loop());

        // Set callbacks
        player_set_completed_cb(m_cplayer, __videoPlayerCompleteCB, (void*)this);
        player_set_error_cb(m_cplayer, __videoPlayerErrorCB, (void*)this);

        // Set display options
#ifdef STARFISH_TIZEN_TV
        Evas_Object* win = (Evas_Object*) m_videoElement->document()->window()->unwrap();
        player_display_h display_handle = GET_DISPLAY(elm_win_xwindow_get(win));
        player_display_type_e display_type = PLAYER_DISPLAY_TYPE_X11;
        player_display_mode_e display_mode = PLAYER_DISPLAY_MODE_DST_ROI;
        player_display_roi_mode_e roi_mode = PLAYER_DISPLAY_ROI_MODE_LETTER_BOX;

        player_set_display(m_cplayer, (player_display_type_e) display_type, display_handle);
        player_set_display_mode(m_cplayer, display_mode);
        player_set_x11_display_roi_mode(m_cplayer, roi_mode);
#elif STARFISH_TIZEN_MOBILE
        player_display_h display_handle = GET_DISPLAY((Evas_Object*) m_surface->unwrap());
        player_display_type_e display_type = PLAYER_DISPLAY_TYPE_EVAS;
        player_display_mode_e display_mode = PLAYER_DISPLAY_MODE_ORIGIN_OR_LETTER;

        player_set_display(m_cplayer, display_type, display_handle);
        player_set_display_mode(m_cplayer, display_mode);
#endif
    }
    return true;
}

void VideoPlayer::lockElementPointer()
{
    STARFISH_ASSERT(m_videoElement);
    if (!m_isElementPointerLocked) {
        m_videoElement->document()->window()->starFish()->addPointerInRootSet(m_videoElement);
        m_isElementPointerLocked = true;
    }
}

void VideoPlayer::unlockElementPointer()
{
    STARFISH_ASSERT(m_videoElement);
    if (m_isElementPointerLocked) {
        m_videoElement->document()->window()->starFish()->removePointerFromRootSet(m_videoElement);
        m_isElementPointerLocked = false;
    }
}

bool VideoPlayer::hasPendingUrl()
{
    // NOTE: m_inputUrl(new request) can equals to m_currentUrl
    return m_hasPendingUrl;
}

void VideoPlayer::pushPendingUrl()
{
    m_hasPendingUrl = true;
}

void VideoPlayer::popPendingUrl()
{
    STARFISH_ASSERT(m_hasPendingUrl);
    // Only one pending
    m_hasPendingUrl = false;
    m_currentUrl = m_inputUrl;
}

void VideoPlayer::postLoaded()
{
    unlockElementPointer();
    // If public state changed during PREPARING, exit.
    if (!isPublicState(MediaPlayer::STATE_PREPARING) || !m_cplayer) {
        return;
    }
    // If cplayer's state is not ready -> load fail
    player_state_e state;
    if (player_get_state(m_cplayer, &state) != PLAYER_ERROR_NONE || state != PLAYER_STATE_READY) {
        setPublicState(MediaPlayer::STATE_UNKNOWN_ERROR);
        onPrepared(true);
        return;
    }

    setPublicState(MediaPlayer::STATE_READY);
    // If there was another prepare() request during PREPARING,
    // forget current request and start to load new request.
    if (hasPendingUrl()) {
        unprepareCPlayer();
        popPendingUrl();
        prepareCPlayer();
        return;
    }
    onPrepared(false);

    // If there was a play() request during PREPARING, start playing
    if (lastRequestIs(VideoPlayer::REQUEST_PLAY)) {
        playCPlayer();
    }

#if 0 // Debug code
#ifdef STARFISH_TIZEN_TV
    if (m_cplayer) {
        player_video_track_info info;
        int errorCode = player_get_video_info(m_cplayer, &info);
        if (errorCode != PLAYER_ERROR_NONE) {
            PLAYER_LOGI("player_get_video_info() is failed(%d)\n", errorCode);
        }
        PLAYER_LOGI("video info----------------------------------------\n");
        PLAYER_LOGI("- resolution : %u x %u\n", info.width, info.height);
        PLAYER_LOGI("- bitrate : %u bps\n", info.bitrate);
        PLAYER_LOGI("--------------------------------------------------\n");
    }
#endif
#endif
}

void VideoPlayer::postPlayFinished()
{
    unlockElementPointer();
    // If public state changed during PREPARING, exit.
    if (!isPublicState(MediaPlayer::STATE_PLAYING) || !m_cplayer) {
        return;
    }
    // Normal case,
    onPlayFinished();
}

void VideoPlayer::destroyCPlayer()
{
    if (!m_cplayer) {
        STARFISH_ASSERT(isPublicState(MediaPlayer::STATE_NONE));
        return;
    }
    player_stop(m_cplayer);
    player_unprepare(m_cplayer);
    m_currentUrl = nullptr;
    m_cplayer = NULL;

    // Set public state
    setPublicState(MediaPlayer::STATE_NONE);
    unlockElementPointer();
}

void VideoPlayer::prepareCPlayer()
{
    if (!m_cplayer) {
        STARFISH_ASSERT(isPublicState(MediaPlayer::STATE_NONE));
        return;
    }
    if (!m_currentUrl) {
        return;
    }
#ifdef STARFISH_TIZEN_MOBILE
    if (!m_surface) {
        return;
    }
#endif
    if (m_currentUrl->isFileURL() || m_currentUrl->isNetworkURL()) {
        PLAYER_LOGI("prepare() - url : %s\n", m_currentUrl->urlString()->utf8Data());
        player_set_uri(m_cplayer, const_cast<char*>(m_currentUrl->urlString()->utf8Data()));

        setPublicState(MediaPlayer::STATE_PREPARING);
        lockElementPointer();
        int errorCode = player_prepare_async(m_cplayer, __videoPlayerPrepareCB, (void*)this);
        if (errorCode != PLAYER_ERROR_NONE) {
            PLAYER_LOGE(errorCode, "player_prepare_async()\n");
            unlockElementPointer();
            setPublicState(MediaPlayer::STATE_NONE);
            return;
        }
    }
}

void VideoPlayer::unprepareCPlayer()
{
    if (!m_cplayer) {
        STARFISH_ASSERT(isPublicState(MediaPlayer::STATE_NONE));
        return;
    }
    player_unprepare(m_cplayer);
    // If cplayer's state is not [IDLE] -> something wrong
    player_state_e state;
    if (UNLIKELY(player_get_state(m_cplayer, &state) != PLAYER_ERROR_NONE || state != PLAYER_STATE_IDLE)) {
        setPublicState(MediaPlayer::STATE_UNKNOWN_ERROR);
        return;
    }
    // Set public state
    setPublicState(MediaPlayer::STATE_NONE);
    unlockElementPointer();
    m_currentUrl = nullptr;
}

void VideoPlayer::playCPlayer()
{
    if (!m_cplayer) {
        STARFISH_ASSERT(isPublicState(MediaPlayer::STATE_NONE));
        return;
    }

#ifdef STARFISH_TIZEN_TV
    player_set_x11_display_dst_roi(m_cplayer, m_displayArea.x(), m_displayArea.y(), m_displayArea.width(), m_displayArea.height());
    PLAYER_LOGI("video display area : %f %f %f %f\n", m_displayArea.x(), m_displayArea.y(), m_displayArea.width(), m_displayArea.height());
#endif

    if (player_start(m_cplayer) != PLAYER_ERROR_NONE) {
        // NOTE: wrong url can cause this
        setPublicState(MediaPlayer::STATE_UNKNOWN_ERROR);
        return;
    }
    // If cplayer's state is not [IDLE|PLAYING] -> something wrong
    player_state_e state;
    if (UNLIKELY(player_get_state(m_cplayer, &state) != PLAYER_ERROR_NONE
        || !(state == PLAYER_STATE_IDLE || state == PLAYER_STATE_PLAYING))) {
        setPublicState(MediaPlayer::STATE_UNKNOWN_ERROR);
        return;
    }
    // Set public state
    if (state == PLAYER_STATE_IDLE) {
        setPublicState(MediaPlayer::STATE_NONE);
    } else {
        STARFISH_ASSERT(state == PLAYER_STATE_PLAYING);
        setPublicState(MediaPlayer::STATE_PLAYING);
        lockElementPointer();
        PLAYER_LOGI("play!!!\n");
    }
}

void VideoPlayer::pauseCPlayer()
{
    if (!m_cplayer) {
        STARFISH_ASSERT(isPublicState(MediaPlayer::STATE_NONE));
        return;
    }
    player_pause(m_cplayer);
    // TODO : confirm state
    // If cplayer's state is not [IDLE|PAUSED] -> something wrong
    player_state_e state;
    if (UNLIKELY(player_get_state(m_cplayer, &state) != PLAYER_ERROR_NONE
        || !(state == PLAYER_STATE_IDLE || state == PLAYER_STATE_PAUSED))) {
        setPublicState(MediaPlayer::STATE_UNKNOWN_ERROR);
        return;
    }
    // Set public state
    if (state == PLAYER_STATE_IDLE) {
        setPublicState(MediaPlayer::STATE_NONE);
    } else {
        STARFISH_ASSERT(state == PLAYER_STATE_PAUSED);
        setPublicState(MediaPlayer::STATE_PAUSED);
    }
    unlockElementPointer();
}

void VideoPlayer::stopCPlayer()
{
    if (!m_cplayer) {
        STARFISH_ASSERT(isPublicState(MediaPlayer::STATE_NONE));
        return;
    }
    player_stop(m_cplayer);
    // If cplayer's state is not [IDLE|READY] -> something wrong
    player_state_e state;
    if (UNLIKELY(player_get_state(m_cplayer, &state) != PLAYER_ERROR_NONE
        || !(state == PLAYER_STATE_IDLE || state == PLAYER_STATE_READY))) {
        setPublicState(MediaPlayer::STATE_UNKNOWN_ERROR);
        return;
    }
    // Set public state
    if (state == PLAYER_STATE_IDLE) {
        setPublicState(MediaPlayer::STATE_NONE);
    } else {
        STARFISH_ASSERT(state == PLAYER_STATE_READY);
        setPublicState(MediaPlayer::STATE_READY);
    }
    unlockElementPointer();
}

void VideoPlayer::prepare()
{
    if (!assureCPlayer()) {
        PLAYER_LOGI("prepare() fail : could not create player\n");
        return;
    }
    if (isPublicState(MediaPlayer::STATE_PREPARING)) {
        pushPendingUrl();
        return;
    }
    if (isPublicState(MediaPlayer::STATE_PLAYING | MediaPlayer::STATE_PAUSED)) {
        stopCPlayer();
    }
    if (isPublicState(MediaPlayer::STATE_READY)) {
        unprepareCPlayer();
    }
    if (isPublicState(MediaPlayer::STATE_UNKNOWN_ERROR)) {
        PLAYER_LOGI("prepare() fail : unlown error\n");
        return;
    }
    m_currentUrl = m_inputUrl;
    prepareCPlayer();
}

#ifdef STARFISH_TIZEN_TV
void VideoPlayer::setDisplayArea(int x, int y, int width, int height)
{
    m_displayArea = Rect(x, y, width, height);
    if (m_cplayer && isPublicState(MediaPlayer::STATE_PLAYING)) {
        player_set_x11_display_dst_roi(m_cplayer, m_displayArea.x(), m_displayArea.y(), m_displayArea.width(), m_displayArea.height());
    }
}
#elif STARFISH_TIZEN_MOBILE
void VideoPlayer::setDisplayArea(CanvasSurface* surface)
{
    m_surface = surface;
}
#endif

void VideoPlayer::play()
{
    m_lastRequest = VideoPlayer::REQUEST_PLAY;
    if (!assureCPlayer()) {
        PLAYER_LOGI("play() fail : could not create player\n");
        return;
    }
    if (isPublicState(MediaPlayer::STATE_PREPARING | MediaPlayer::STATE_PLAYING)) {
        return;
    }
    if (isPublicState(MediaPlayer::STATE_UNKNOWN_ERROR)) {
        PLAYER_LOGI("play() fail : unknown error\n");
        return;
    }
    if (isPublicState(MediaPlayer::STATE_NONE)) {
        if (m_inputUrl) {
            m_currentUrl = m_inputUrl;
            prepareCPlayer();
        } else {
            PLAYER_LOGI("play() fail : setURL() first\n");
        }
        return;
    }
    playCPlayer();
}

void VideoPlayer::pause()
{
    m_lastRequest = REQUEST_PAUSE;
    if (!assureCPlayer()) {
        PLAYER_LOGI("pause() fail : could not create player\n");
        return;
    }
    if (isPublicState(MediaPlayer::STATE_NONE | MediaPlayer::STATE_PREPARING | MediaPlayer::STATE_PAUSED)) {
        return;
    }
    if (isPublicState(MediaPlayer::STATE_UNKNOWN_ERROR)) {
        PLAYER_LOGI("pause() fail : unknown error\n");
        return;
    }
    pauseCPlayer();
}

void VideoPlayer::setLoop(bool loop)
{
    if (!m_cplayer) {
        return;
    }
    int errorCode = player_set_looping(m_cplayer, loop);
    if (PLAYER_ERROR_NONE != errorCode) {
        PLAYER_LOGE(errorCode, "player_set_looping()\n");
        return;
    }
}

int VideoPlayer::width()
{
    if (!m_cplayer)
        return 0;

#ifdef STARFISH_TIZEN_TV
    player_video_track_info info;
    int errorCode = player_get_video_info(m_cplayer, &info);
    if (errorCode != PLAYER_ERROR_NONE) {
        PLAYER_LOGI("player_get_video_info() is failed(%d)\n", errorCode);
    }
    return info.width;
#else
    // TODO
    return 0;
#endif
}

int VideoPlayer::height()
{
    if (!m_cplayer)
        return 0;
#ifdef STARFISH_TIZEN_TV
    player_video_track_info info;
    int errorCode = player_get_video_info(m_cplayer, &info);
    if (errorCode != PLAYER_ERROR_NONE) {
        PLAYER_LOGI("player_get_video_info() is failed(%d)\n", errorCode);
    }
    return info.height;
#else
    // TODO
    return 0;
#endif
}

#endif /* STARFISH_TIZEN && !(STARFISH_TIZEN_WEARABLE) */
#undef PLAYER_LOGI
#undef PLAYER_LOGE
}
#endif /* STARFISH_ENABLE_MULTIMEDIA */
