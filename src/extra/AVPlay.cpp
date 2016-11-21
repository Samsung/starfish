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
#if defined(STARFISH_TIZEN_TV) && defined (STARFISH_ENABLE_AVPLAY)
#include "StarFishConfig.h"
#include "AVPlay.h"
#include "platform/message_loop/MessageLoop.h"
#include <Elementary.h>
#include <Ecore_X.h>

namespace StarFish {

void printNativePlayerError(int errorCode)
{
    switch (errorCode) {
#define GEN_ERROR_PRINTS(errorenum) \
    case errorenum: \
        STARFISH_LOG_INFO("%s\n", #errorenum); \
        return;
        GEN_ERROR_PRINTS(PLAYER_ERROR_OUT_OF_MEMORY)
        GEN_ERROR_PRINTS(PLAYER_ERROR_INVALID_PARAMETER)
        GEN_ERROR_PRINTS(PLAYER_ERROR_NO_SUCH_FILE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_INVALID_OPERATION)
        GEN_ERROR_PRINTS(PLAYER_ERROR_FILE_NO_SPACE_ON_DEVICE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_SEEK_FAILED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_INVALID_STATE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_NOT_SUPPORTED_FILE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_INVALID_URI)
        GEN_ERROR_PRINTS(PLAYER_ERROR_SOUND_POLICY)
        GEN_ERROR_PRINTS(PLAYER_ERROR_CONNECTION_FAILED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_VIDEO_CAPTURE_FAILED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_DRM_EXPIRED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_DRM_NO_LICENSE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_DRM_FUTURE_USE)
        GEN_ERROR_PRINTS(PLAYER_ERROR_DRM_NOT_PERMITTED)
        GEN_ERROR_PRINTS(PLAYER_ERROR_RESOURCE_LIMIT)
        GEN_ERROR_PRINTS(PLAYER_ERROR_PERMISSION_DENIED)
#undef GEN_ERROR_PRINTS
    default:
        STARFISH_LOG_INFO("Unknown error\n");
        return;
    }
}

webapis::webapis(StarFish* starFish)
    : ScriptWrappable(this)
    , m_starFish(starFish)
{
    m_avplay = new avplay(m_starFish);
}


avplay::avplay(StarFish* starFish)
    : ScriptWrappable(this)
    , m_starFish(starFish)
    , m_offsetLeft(0)
    , m_offsetTop(0)
    , m_offsetWidth(0)
    , m_offsetHeight(0)
    , m_nativePlayer(nullptr)
    , m_async_prepre(ScriptValueNull)
{
}

void avplay::open(String* url)
{
    STARFISH_LOG_INFO("avplay::open() %s\n", url);
    if (m_nativePlayer) {
        player_destroy(m_nativePlayer);
        m_nativePlayer = nullptr;
    }
    int ret = player_create(&m_nativePlayer);
    if (ret != PLAYER_ERROR_NONE) {
        printNativePlayerError(ret);
    }

    player_display_h display_handle = GET_DISPLAY(elm_win_xwindow_get((Evas_Object*)starFish()->window()->unwrap()));
    player_display_type_e display_type = PLAYER_DISPLAY_TYPE_X11;
    player_display_mode_e display_mode = PLAYER_DISPLAY_MODE_DST_ROI;
    player_display_roi_mode_e roi_mode = PLAYER_DISPLAY_ROI_MODE_LETTER_BOX;
    player_set_display(m_nativePlayer, (player_display_type_e) display_type, display_handle);
    player_set_display_mode(m_nativePlayer, display_mode);
    player_set_x11_display_roi_mode(m_nativePlayer, roi_mode);

    ret = player_set_uri(m_nativePlayer, url->utf8Data());
    if (ret != PLAYER_ERROR_NONE) {
        printNativePlayerError(ret);
    }
}

void avplay::prepare()
{
    int ret = player_prepare(m_nativePlayer);
    if (ret != PLAYER_ERROR_NONE) {
        printNativePlayerError(ret);
    }
}

void avplay::setDisplayRect(double offsetLeft, double offsetTop, double offsetWidth, double offsetHeight)
{
    if (!isnan(offsetLeft))
        m_offsetLeft = offsetLeft;
    if (!isnan(offsetTop))
        m_offsetTop = offsetTop;
    if (!isnan(offsetWidth))
        m_offsetWidth = offsetWidth;
    if (!isnan(offsetHeight))
        m_offsetHeight = offsetHeight;

    STARFISH_LOG_INFO("avplay::setDisplayRect() %lf %lf %lf %lf !!!\n", m_offsetLeft , m_offsetTop, m_offsetWidth, m_offsetHeight);
}


void avplay::play()
{
    STARFISH_LOG_INFO("avplay::play() \n");
    player_set_x11_display_dst_roi(m_nativePlayer, m_offsetLeft, m_offsetTop, m_offsetWidth, m_offsetHeight);
    int ret = player_start(m_nativePlayer);
    if (ret != PLAYER_ERROR_NONE) {
        printNativePlayerError(ret);
    }

}

void avplay::close()
{
    if (m_nativePlayer) {
        player_destroy(m_nativePlayer);
        m_nativePlayer = nullptr;
    }
}

void avplay::pause()
{
    player_pause(m_nativePlayer);
}

void avplay::stop()
{
    player_stop(m_nativePlayer);
}

String* avplay::getState()
{
    player_state_e state;
    player_get_state(m_nativePlayer, &state);
    if (state == PLAYER_STATE_NONE) {
        return String::fromUTF8("NONE");
    } else if (state == PLAYER_STATE_IDLE) {
        return String::fromUTF8("IDLE");
    } else if (state == PLAYER_STATE_READY) {
        return String::fromUTF8("READY");
    } else if (state == PLAYER_STATE_PLAYING) {
        return String::fromUTF8("PLAYING");
    } else if (state == PLAYER_STATE_PAUSED) {
        return String::fromUTF8("PAUSED");
    }
    return nullptr;
}

double avplay::getCurrentTime()
{
    int position = 0;
    player_get_position(m_nativePlayer, &position);
    return (double)position;
}

double avplay::getDuration()
{
    int duration = 0;
    player_get_duration(m_nativePlayer, &duration);
    return (double)duration;
}

void avplay::seekTo(double seekTime)
{
    // TODO
    int ret = player_set_position(m_nativePlayer, seekTime, NULL, NULL);
    if (ret != PLAYER_ERROR_NONE) {
        printNativePlayerError(ret);
    }
}

void avplay::suspend()
{
    // TODO
}

void avplay::restore()
{
    // TODO
}

static void _videoPlayerPrepareCB(void *user_data)
{
    STARFISH_LOG_INFO("avplay::_videoPlayerPrepareCB()\n");
    avplay* self = (avplay*)user_data;
    self->starFish()->messageLoop()->addIdlerWithNoGCRootingInOtherThread([](size_t, void* data) {
        avplay* self = (avplay*)data;
        self->callprepareCallback();
    }, user_data);
}

void avplay::prepareAsync(ScriptValue listener)
{
    STARFISH_LOG_INFO("avplay::prepareAsync()\n");
    m_async_prepre = listener;
    int ret = player_prepare_async(m_nativePlayer, _videoPlayerPrepareCB, this);
    if (ret != PLAYER_ERROR_NONE) {
        printNativePlayerError(ret);
    }
    // TODO
}

void avplay::setStreamingProperty(String* arg1, String* arg2)
{
    // TODO
}

void avplay::callprepareCallback()
{
    callScriptFunction(m_async_prepre, { }, 0, escargot::ESVMInstance::currentInstance()->globalObject());
}

// void setListener(listener);



}
#endif
