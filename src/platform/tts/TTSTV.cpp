/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_TIZEN) && defined(STARFISH_ENABLE_TTS)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/Event.h"
#include "core/page/Window.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/tts/TTS.h"
#include "core/page/WebView.h"

#include <Elementary.h>
#include "core/modules/message_loop/MessageLoop.h"
#include <vconf/vconf.h>

// NOTE: Original TTS_MODE_INTERRUPT is defined in tts_internal.h.
#define TTS_MODE_INTERRUPT 3

namespace Starfish {

static void onAccessibilityChanged(keynode_t* keynodeName, void* data)
{
    STARFISH_LOG_INFO("onAccessibilityChanged");
    // Do not free data
    TTS* tts = (TTS*)data;
    int at = 0;
    int vconf_ret = vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &at);

    tts->setTTSEnabled((vconf_ret == 0 && at == 1));
}

void stateChangedCB(tts_h ttsHandle, tts_state_e previous, tts_state_e current,
                    void* data)
{
    // TODO: If Needed
    TTS* ttsData = (TTS*)data;
    STARFISH_LOG_INFO("TTS: State Changed from %s to %s\n",
                      ttsData->state(previous), ttsData->state(current));
}

void utteranceErrorCB(tts_h tts, int utteranceId, tts_error_e reason,
                      void* data)
{
    STARFISH_LOG_INFO("TTS: utteranceErrorCB [ID : %d][Error : %d] \n",
                      utteranceId, (int)reason);
}

void utteranceStartedCB(tts_h ttsHandle, int utteranceId, void* data)
{
    TTS* ttsData = (TTS*)data;
    if (ttsData) {
        Element* element = ttsData->element();
        String* eventName =
            element->starfish()->staticStrings()->m_ttsstart.localName();
        Event* e = new Event(element->document(), eventName);

        element->window()->dispatchEventByUA(e);
    }
}

void utterenceCompletedCB(tts_h tts_handle, int utteranceId, void* data)
{
    TTS* ttsData = (TTS*)data;
    if (ttsData) {
        Element* element = ttsData->element();
        String* eventName =
            element->starfish()->staticStrings()->m_ttsend.localName();
        Event* e = new Event(element->document(), eventName);

        element->window()->dispatchEventByUA(e);
    }
}

void TTS::initialize()
{
    int at = 0;
    int vconf_ret = vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &at);

    m_isTTSEnabled = (vconf_ret == 0 && at == 1);

    // Add listener
    vconf_notify_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS,
                             onAccessibilityChanged, this);

    if (m_handle == NULL) {
        ecore_main_loop_thread_safe_call_async(
            [](void* data) -> void {
                TTS* tts = (TTS*)data;
                tts->createHandle();
            },
            this);
    }
}

void TTS::destroy()
{
    int ret = 0;

    if (m_handle) {
        if ((ret = tts_stop(m_handle)) != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("tts_stop failed : %d", ret);
        }
        if ((ret = tts_unprepare(m_handle)) != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("tts_unprepare failed : %d", ret);
        }
        if ((ret = tts_unset_state_changed_cb(m_handle)) != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("tts_unset_state_changed_cb failed : %d", ret);
        }
        if ((ret = tts_unset_utterance_started_cb(m_handle)) !=
            TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("tts_unset_utterance_started_cb failed : %d",
                               ret);
        }
        if ((ret = tts_unset_error_cb(m_handle)) != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("tts_unset_error_cb failed : %d", ret);
        }
        if ((ret = tts_unset_utterance_completed_cb(m_handle)) !=
            TTS_ERROR_NONE) {
            STARFISH_LOG_INFO("tts_unset_utterance_completed_cb failed : %d",
                              ret);
        }
        if ((ret = tts_destroy(m_handle)) != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("tts_destroy failed : %d", ret);
        }

        STARFISH_LOG_INFO("TTS destroyed successfully");
        m_handle = NULL;
    } else {
        STARFISH_LOG_INFO("tts handle is null in destroyTTSHandle()");
    }
}

void TTS::speech(Element* element, String* text)
{
    if (!text->equals(String::emptyString)) {
        auto u8String = text->toUTF8NonGCString();
        char* buf = (char*)malloc(u8String.length() + 1);
        memcpy(buf, u8String.data(), u8String.length());
        buf[u8String.length()] = 0;
        m_element = element;

        struct Dummy {
            TTS* tts;
            char* buf;
        };

        Dummy* d = new Dummy;
        d->tts = this;
        d->buf = buf;

        ecore_main_loop_thread_safe_call_async(
            [](void* data) -> void {
                Dummy* d = (Dummy*)data;
                TTS* tts = d->tts;
                char* text = d->buf;

                STARFISH_LOG_INFO("[TTS Play] speech TV : %s\n", text);
                tts->startPlay(text);

                free(text);
                delete d;
            },
            d);
    }
}

bool TTS::createHandle()
{
    int ret = 0;
    if ((ret = tts_create(&m_handle)) != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("tts_create failed : %d", ret);
        return false;
    }
    if (m_handle == NULL) {
        STARFISH_LOG_ERROR("tts handle is null in createTTSHandle()");
        return false;
    }

    // NOTE: Because we need to use interrupt mode though we should use
    // managed APIs, we set only the value.
    if ((ret = tts_set_mode(m_handle, (tts_mode_e)TTS_MODE_INTERRUPT)) !=
        TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("tts_set_mode failed : %d", ret);
        return false;
    }
    if ((ret = tts_prepare(m_handle)) != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("tts_prepare failed : %d", ret);
        return false;
    }
    if ((ret = tts_set_state_changed_cb(m_handle, stateChangedCB, this)) !=
        TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("tts_set_state_changed_cb failed : %d", ret);
        return false;
    }
    if ((ret = tts_set_error_cb(m_handle, utteranceErrorCB, this)) !=
        TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("tts_set_error_cb failed : %d", ret);
        return false;
    }
    if ((ret = tts_set_utterance_started_cb(m_handle, utteranceStartedCB,
                                            this)) != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("tts_set_utterance_started_cb failed : %d", ret);
        return false;
    }
    if ((ret = tts_set_utterance_completed_cb(m_handle, utterenceCompletedCB,
                                              this)) != TTS_ERROR_NONE) {
        STARFISH_LOG_INFO("tts_set_utterance_completed_cb failed : %d", ret);
        return false;
    }

    return true;
}

bool TTS::startPlay(const char* text)
{
    if (m_handle == NULL) {
        STARFISH_LOG_ERROR("tts handle is null in startTTSPlay()");
        return false;
    }

    addText(text);
    return true;
}

bool TTS::stopPlay()
{
    if (m_handle == NULL) {
        STARFISH_LOG_ERROR("tts handle is null in stopTTSPlay()");
        return false;
    }

    int ret = 0;
    if ((ret = tts_stop(m_handle)) != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("tts_stop failed : %d", ret);
        return false;
    }
    return true;
}

bool TTS::addText(const char* text)
{
    if (m_handle == NULL) {
        STARFISH_LOG_ERROR("tts handle is null in addTTSText()");
        return false;
    }

    int ret = 0;
    int utt_id = 0;

    if ((ret = tts_stop(m_handle)) != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("tts_stop failed : %d", ret);
        return false;
    }
    if ((ret = tts_add_text(m_handle, text, NULL, TTS_VOICE_TYPE_AUTO,
                            TTS_SPEED_AUTO, &utt_id)) != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("tts_add_text failed : %d", ret);
        return false;
    }
    if ((ret = tts_play(m_handle)) != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("tts_play failed : %d", ret);
        return false;
    }

    STARFISH_LOG_INFO("tts_play = %s ", text);
    return true;
}

const char* TTS::state(int state)
{
    const char* stateString[4] = { "TTS_STATE_CREATED", "TTS_STATE_READY",
                                   "TTS_STATE_PLAYING", "TTS_STATE_PAUSED" };

    if (state < 0 ||
        state >= (int)(sizeof(stateString) / sizeof(const char*))) {
        STARFISH_LOG_INFO("TTS Invalid state value [%d/%d]", state,
                          sizeof(stateString) / sizeof(const char*));
        return "";
    }

    return (stateString[state]);
}
}
#endif
