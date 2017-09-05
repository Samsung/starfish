
/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/extra/Avplay.h"
#include "core/modules/message_loop/MessageLoop.h"
#include <Elementary.h>
#include <Ecore_X.h>

namespace StarFish {

static void _videoPlayerPrepareCB(void* user_data)
{
    STARFISH_LOG_INFO("avplay::_videoPlayerPrepareCB()\n");
    Avplay* self = (Avplay*)user_data;
    self->starFish()->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
        [](size_t, void* data) {
            Avplay* self = (Avplay*)data;
            self->callJSCallback(Avplay::prepare_async_CALLBACK);
        },
        user_data);
}

static void _videoPlayerCompletedCB(void* user_data)
{
    STARFISH_LOG_INFO("avplay::_videoPlayerCompletedCB()\n");
    Avplay* self = (Avplay*)user_data;
    self->starFish()->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
        [](size_t, void* data) {
            Avplay* self = (Avplay*)data;
            self->callJSCallback(Avplay::onstreamcompleted_CALLBACK);
        },
        user_data);
}

static void _videoPlayerbufferingCBNative(int percent, void* user_data)
{
    STARFISH_LOG_INFO("avplay::_videoPlayerbufferingCBNative() %d\n", percent);
    Avplay* self = (Avplay*)user_data;
    if (percent < 100) {
        self->setBufferingPercent(percent);
        self->starFish()->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
            [](size_t, void* data) {
                Avplay* self = (Avplay*)data;
                self->callJSCallback(Avplay::onbufferingprogress_CALLBACK);
            },
            user_data);
    } else if (percent == 100) {
        self->setBufferingPercent(percent);
        self->starFish()->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
            [](size_t, void* data) {
                Avplay* self = (Avplay*)data;
                self->callJSCallback(Avplay::onbufferingcomplete_CALLBACK);
            },
            user_data);
    }
}

static void _videoPlayerEventCBNative(int msg, void* msg_data, void* user_data)
{
    STARFISH_LOG_INFO("avplay::_videoPlayerEventCBNative()\n");
}

static void _videoPlayerErrorEventCBNative(int error_code, void* user_data)
{
    STARFISH_LOG_INFO("avplay::_videoPlayerErrorEventCBNative()\n");
    Avplay* self = (Avplay*)user_data;
    self->starFish()->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
        [](size_t, void* data) {
            Avplay* self = (Avplay*)data;
            self->callJSCallback(Avplay::onerror_CALLBACK);
        },
        user_data);
}

void printNativePlayerError(int errorCode)
{
    switch (errorCode) {
#define GEN_ERROR_PRINTS(errorenum)            \
    case errorenum:                            \
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

Avplay::Avplay(Document* document)
    : ScriptWrappable(this)
    , DocumentHoldable(document)
    , m_offsetLeft(0)
    , m_offsetTop(0)
    , m_offsetWidth(0)
    , m_offsetHeight(0)
    , m_bufferingPercent(0)
    , m_nativePlayer(nullptr)
    , m_prepare_async(ESValue(ESValue::ESNull))
    , m_listener(ESValue(ESValue::ESNull))
{
}

Avplay::~Avplay()
{
    close();
}

void Avplay::open(String* url)
{
    STARFISH_LOG_INFO("avplay::open() :: URL %s\n",
                      url->toUTF8NonGCString().data());
    if (m_nativePlayer) {
        player_destroy(m_nativePlayer);
        m_nativePlayer = nullptr;
    }
    int ret = player_create(&m_nativePlayer);
    if (ret != PLAYER_ERROR_NONE) {
        printNativePlayerError(ret);
    }

    ret =
        player_set_completed_cb(m_nativePlayer, _videoPlayerCompletedCB, this);
    if (ret != PLAYER_ERROR_NONE) {
        printNativePlayerError(ret);
    }

    // ret = player_set_others_event_cb(m_nativePlayer,
    // _videoPlayerEventCBNative, this);
    // if (ret != PLAYER_ERROR_NONE) {
    //     printNativePlayerError(ret);
    // }

    ret = player_set_error_cb(m_nativePlayer, _videoPlayerErrorEventCBNative,
                              this);
    if (ret != PLAYER_ERROR_NONE) {
        printNativePlayerError(ret);
    }

    ret = player_set_buffering_cb(m_nativePlayer, _videoPlayerbufferingCBNative,
                                  this);
    if (ret != PLAYER_ERROR_NONE) {
        printNativePlayerError(ret);
    }

    player_display_h display_handle = GET_DISPLAY(
        elm_win_xwindow_get((Evas_Object*)starFish()->window()->unwrap()));
    player_display_type_e display_type = PLAYER_DISPLAY_TYPE_X11;
    player_display_mode_e display_mode = PLAYER_DISPLAY_MODE_DST_ROI;
    player_display_roi_mode_e roi_mode = PLAYER_DISPLAY_ROI_MODE_LETTER_BOX;
    player_set_display(m_nativePlayer, (player_display_type_e)display_type,
                       display_handle);
    player_set_display_mode(m_nativePlayer, display_mode);
    player_set_x11_display_roi_mode(m_nativePlayer, roi_mode);

    ret = player_set_uri(m_nativePlayer, url->toUTF8NonGCString().data());
    if (ret != PLAYER_ERROR_NONE) {
        printNativePlayerError(ret);
    }
}

void Avplay::prepare()
{
    int ret = player_prepare(m_nativePlayer);
    if (ret != PLAYER_ERROR_NONE) {
        printNativePlayerError(ret);
    }
}

void Avplay::setDisplayRect(double offsetLeft, double offsetTop,
                            double offsetWidth, double offsetHeight)
{
    if (!isnan(offsetLeft)) {
        m_offsetLeft = offsetLeft;
    }
    if (!isnan(offsetTop)) {
        m_offsetTop = offsetTop;
    }
    if (!isnan(offsetWidth)) {
        m_offsetWidth = offsetWidth;
    }
    if (!isnan(offsetHeight)) {
        m_offsetHeight = offsetHeight;
    }

    STARFISH_LOG_INFO("avplay::setDisplayRect() %lf %lf %lf %lf !!!\n",
                      m_offsetLeft, m_offsetTop, m_offsetWidth, m_offsetHeight);
}

void Avplay::play()
{
    STARFISH_LOG_INFO("avplay::play() \n");
    player_set_x11_display_dst_roi(m_nativePlayer, m_offsetLeft, m_offsetTop,
                                   m_offsetWidth, m_offsetHeight);
    int ret = player_start(m_nativePlayer);
    if (ret != PLAYER_ERROR_NONE) {
        printNativePlayerError(ret);
    }
}

void Avplay::close()
{
    STARFISH_LOG_INFO("avplay::close()\n");
    if (m_nativePlayer) {
        player_unset_buffering_cb(m_nativePlayer);

        player_unset_completed_cb(m_nativePlayer);

        // player_unset_others_event_cb(m_nativePlayer);

        player_unset_error_cb(m_nativePlayer);

        player_destroy(m_nativePlayer);
        m_nativePlayer = nullptr;
    }
}

void Avplay::pause()
{
    STARFISH_LOG_INFO("avplay::pause()\n");
    player_state_e state;
    player_get_state(m_nativePlayer, &state);

    if (state >= PLAYER_STATE_READY) {
        int ret = player_pause(m_nativePlayer);
        if (ret != PLAYER_ERROR_NONE) {
            printNativePlayerError(ret);
        }
    }
}

void Avplay::stop()
{
    STARFISH_LOG_INFO("avplay::stop()\n");
    int ret = player_stop(m_nativePlayer);
    if (ret != PLAYER_ERROR_NONE) {
        printNativePlayerError(ret);
    }
    close();
}

String* Avplay::getState()
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

double Avplay::getCurrentTime()
{
    STARFISH_LOG_INFO("avplay::getCurrentTime()\n");
    int position = 0;
    player_get_position(m_nativePlayer, &position);
    return (double)position;
}

double Avplay::getDuration()
{
    STARFISH_LOG_INFO("avplay::getDuration()\n");
    int duration = 0;
    player_state_e state;
    player_get_state(m_nativePlayer, &state);
    if (state >= PLAYER_STATE_READY) {
        player_get_duration(m_nativePlayer, &duration);
    }
    return (double)duration;
}

void Avplay::seekTo(double seekTime)
{
    // TODO
    STARFISH_LOG_INFO("avplay::seekTo()\n");
    player_state_e state;
    player_get_state(m_nativePlayer, &state);

    if (state >= PLAYER_STATE_READY) {
        int ret = player_set_position(m_nativePlayer, seekTime, NULL, NULL);
        if (ret != PLAYER_ERROR_NONE) {
            printNativePlayerError(ret);
        }
    }
}

void Avplay::suspend()
{
    // TODO
}

void Avplay::restore()
{
    // TODO
}

void Avplay::prepareAsync(ScriptValue listener)
{
    callJSCallback(onbufferingstart_CALLBACK);

    STARFISH_LOG_INFO("avplay::prepareAsync()\n");
    m_prepare_async = listener;
    int ret = player_prepare_async(m_nativePlayer, _videoPlayerPrepareCB, this);
    if (ret != PLAYER_ERROR_NONE) {
        printNativePlayerError(ret);
    }
    // TODO
}

void Avplay::setListener(ScriptValue listener)
{
    // TODO
    m_listener = listener;

    // Test Code
    /*
    {
        callJSCallback(onbufferingstart_CALLBACK);
        callJSCallback(onbufferingprogress_CALLBACK);
        callJSCallback(onbufferingcomplete_CALLBACK);
        callJSCallback(oncurrentplaytime_CALLBACK);
        callJSCallback(onevent_CALLBACK);
        callJSCallback(onerror_CALLBACK);
        callJSCallback(onsubtitlechange_CALLBACK);
        callJSCallback(ondrmevent_CALLBACK);
        callJSCallback(onstreamcompleted_CALLBACK);
    }
    */
}

void Avplay::setStreamingProperty(String* arg1, String* arg2)
{
    // TODO
}

void Avplay::callJSCallback(AVPLAY_CALLBACK_TYPE type)
{
    ScriptValue thisValue =
        escargot::ESVMInstance::currentInstance()->globalObject();
    ScriptValue fn = ESValue(ESValue::ESNull);
    ScriptValue* argv = {};
    size_t argc = 0;
    switch (type) {
    case prepare_async_CALLBACK:
        fn = m_prepare_async;
        break;

    case onbufferingstart_CALLBACK:
        fn = m_listener.asESPointer()->asESObject()->get(
            escargot::ESString::create("onbufferingstart"));
        break;

    case onbufferingprogress_CALLBACK:
        argv = new ScriptValue(m_bufferingPercent);
        argc = 1;
        fn = m_listener.asESPointer()->asESObject()->get(
            escargot::ESString::create("onbufferingprogress"));
        break;

    case onbufferingcomplete_CALLBACK:
        fn = m_listener.asESPointer()->asESObject()->get(
            escargot::ESString::create("onbufferingcomplete"));
        break;

    case oncurrentplaytime_CALLBACK:
        fn = m_listener.asESPointer()->asESObject()->get(
            escargot::ESString::create("oncurrentplaytime"));
        break;

    case onevent_CALLBACK:
        fn = m_listener.asESPointer()->asESObject()->get(
            escargot::ESString::create("onevent"));
        break;

    case onerror_CALLBACK:
        fn = m_listener.asESPointer()->asESObject()->get(
            escargot::ESString::create("onerror"));
        break;

    case onsubtitlechange_CALLBACK:
        fn = m_listener.asESPointer()->asESObject()->get(
            escargot::ESString::create("onsubtitlechange"));
        break;

    case ondrmevent_CALLBACK:
        fn = m_listener.asESPointer()->asESObject()->get(
            escargot::ESString::create("ondrmevent"));
        break;

    case onstreamcompleted_CALLBACK:
        fn = m_listener.asESPointer()->asESObject()->get(
            escargot::ESString::create("onstreamcompleted"));
        break;

    default:
        STARFISH_LOG_INFO("avplay::callJSCallback() ERROR!\n");
    }
    callScriptFunction(fn, argv, argc, thisValue);
    delete argv;
}
}
#endif
