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

#if defined(STARFISH_TIZEN) && defined(STARFISH_ENABLE_TTS) && \
    defined(STARFISH_TIZEN_PROD_TV)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/ErrorEvent.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/tts/TTS.h"
#include "core/modules/tts/SpeechSynthesisEvent.h"
#include "core/modules/profiling/Profiling.h"

#include <Elementary.h>
#include <vconf/vconf.h>

// NOTE: Original TTS_MODE_INTERRUPT is defined in tts_internal.h.
#define TTS_MODE_INTERRUPT 3

namespace Starfish {

int gUtteranceId = 1;

int stringToVoiceType(String* name)
{
    if (name->equalsIgnoreCase("Male")) {
        return TTS_VOICE_TYPE_MALE;
    } else if (name->equalsIgnoreCase("Female")) {
        return TTS_VOICE_TYPE_FEMALE;
    } else if (name->equalsIgnoreCase("Child")) {
        return TTS_VOICE_TYPE_CHILD;
    } else {
        return TTS_VOICE_TYPE_AUTO;
    }
}

const char* errorToString(int error)
{
    switch (error) {
    case TTS_ERROR_NONE:
        return "TTS - Successful";
    case TTS_ERROR_OUT_OF_MEMORY:
        return "TTS - Out of Memory";
    case TTS_ERROR_IO_ERROR:
        return "TTS - I/O error";
    case TTS_ERROR_INVALID_PARAMETER:
        return "TTS - Invalid parameter";
    case TTS_ERROR_OUT_OF_NETWORK:
        return "TTS - Out of network";
    case TTS_ERROR_INVALID_STATE:
        return "TTS - Invalid state";
    case TTS_ERROR_INVALID_VOICE:
        return "TTS - Invalid voice";
    case TTS_ERROR_ENGINE_NOT_FOUND:
        return "TTS - No available engine";
    case TTS_ERROR_TIMED_OUT:
        return "TTS - No answer from the daemon";
    case TTS_ERROR_OPERATION_FAILED:
        return "TTS - Operation failed";
    case TTS_ERROR_AUDIO_POLICY_BLOCKED:
        return "TTS - Audio policy blocked";
    default:
        return "TTS - Unknown Error";
    }
}

const char* stateToString(int state)
{
    switch (state) {
    case TTS_STATE_CREATED:
        return "CREATED";
    case TTS_STATE_READY:
        return "READY";
    case TTS_STATE_PLAYING:
        return "PLAYING";
    case TTS_STATE_PAUSED:
        return "PAUSED";
    default:
        return "Unknown State";
    }
}

static void accessibilityChangedCB(keynode_t* keynodeName, void* data)
{
    STARFISH_LOG_INFO("[TTS] accessibilityChangedCB");

    // Do not free data
    TTS* t = (TTS*)data;
    int result = 0;
    if (vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &result) != 0) {
        return;
    }
    t->setAccessibilityMode(result == 1);
}

static SpeechSynthesisUtterance* findUtterance(TTS* t, int id)
{
    if (t) {
        auto iter = t->utteranceList().find(id);
        if (iter != t->utteranceList().end()) {
            return iter->second;
        }
    }
    return nullptr;
}

static void stateChangedCB(tts_h handle, tts_state_e prev, tts_state_e cur,
                           void* data)
{
    TTS* tts = (TTS*)data;
    if (!tts) {
        return;
    }

    int utterId = tts->currentUtterId();
    STARFISH_LOG_INFO("[TTS] State changed Callback [ID:%d]: [%s] to [%s]\n",
                      utterId, stateToString(prev), stateToString(cur));

    if (prev == TTS_STATE_PAUSED && cur == TTS_STATE_PLAYING && utterId) {
        SpeechSynthesisUtterance* u = findUtterance(tts, utterId);
        if (u) {
            STARFISH_LOG_INFO("[TTS] TTS_STATE_RESUME !!!!");
            String* eventName =
                u->webView()->starfish()->staticStrings()->m_resume.localName();
            SpeechSynthesisEvent* e = new SpeechSynthesisEvent(
                u->executionContext(), eventName,
                SpeechSynthesisEventInit(u, 0, timestamp() - u->startTime(),
                                         String::emptyString));
            u->dispatchEventByUA(e);
        }
    } else if (prev == TTS_STATE_PLAYING && cur == TTS_STATE_PAUSED &&
               utterId) {
        SpeechSynthesisUtterance* u = findUtterance(tts, utterId);
        if (u) {
            STARFISH_LOG_INFO("[TTS] TTS_STATE_PAUSED !!!!");
            String* eventName =
                u->webView()->starfish()->staticStrings()->m_pause.localName();
            SpeechSynthesisEvent* e = new SpeechSynthesisEvent(
                u->executionContext(), eventName,
                SpeechSynthesisEventInit(u, 0, timestamp() - u->startTime(),
                                         String::emptyString));
            u->dispatchEventByUA(e);
        }
    }
}

static void dispatchErrorEvent(TTS* t, int id, const char* errorCode,
                               const char* errorMsg)
{
    STARFISH_LOG_ERROR("[TTS] Occurred Error [ID:%d] : %s", id, errorMsg);
    if (t != nullptr) {
        SpeechSynthesisUtterance* u = findUtterance(t, id);
        if (u != nullptr) {
            ErrorEventInit errorInfo;
            errorInfo.setMessage(String::fromUTF8(errorMsg, strlen(errorMsg)));
            errorInfo.setError(
                ValueRef::create(StringRef::fromASCII(errorCode)));
            Event* errorEvent = new ErrorEvent(
                u->executionContext(),
                u->webView()->starfish()->staticStrings()->m_error.localName(),
                errorInfo);
            u->dispatchEventByUA(errorEvent);
            t->utteranceList().erase(id);
        }

        // NOTE : Code below is required in case of accessibility.
        Element* element = t->element();
        if (u == nullptr && element != nullptr) {
            ErrorEventInit errorInfo;
            errorInfo.setMessage(String::fromUTF8(errorMsg, strlen(errorMsg)));
            errorInfo.setError(
                ValueRef::create(StringRef::fromASCII(errorCode)));
            element->window()->dispatchErrorEvent(errorInfo);
        }
    }
}

static void errorCB(tts_h handle, int utteranceId, tts_error_e reason,
                    void* data)
{
    dispatchErrorEvent((TTS*)data, utteranceId, errorToString(reason),
                       errorToString(reason));
}

static void dispatchStartEvent(TTS* t, int id)
{
    STARFISH_LOG_INFO("[TTS] Started Speaking! [ID:%d]\n", id);
    if (t != nullptr) {
        SpeechSynthesisUtterance* u = findUtterance(t, id);
        if (u) {
            t->setUtterance(u);
            String* eventName =
                u->webView()->starfish()->staticStrings()->m_start.localName();
            u->setStartTime(timestamp());
            SpeechSynthesisEvent* e = new SpeechSynthesisEvent(
                u->executionContext(), eventName,
                SpeechSynthesisEventInit(u, 0, timestamp() - u->startTime(),
                                         String::emptyString));
            u->dispatchEventByUA(e);
        }

        // NOTE : Hold this code according to VD requirement for now.
        Element* element = t->element();
        if (!u && element) {
            String* eventName =
                element->starfish()->staticStrings()->m_ttsstart.localName();
            Event* e = new Event(element->executionContext(), eventName);
            element->window()->dispatchEventByUA(e);
        }
    }
}

static void startedCB(tts_h handle, int utteranceId, void* data)
{
    ((TTS*)data)->setCurrentUtterId(utteranceId);
    dispatchStartEvent((TTS*)data, utteranceId);
}

static void dispatchCompleteEvent(TTS* t, int id)
{
    STARFISH_LOG_INFO("[TTS] Completed Speaking! [ID:%d]\n", id);
    if (t) {
        SpeechSynthesisUtterance* u = findUtterance(t, id);
        if (u) {
            String* eventName =
                u->webView()->starfish()->staticStrings()->m_end.localName();
            SpeechSynthesisEvent* e = new SpeechSynthesisEvent(
                u->executionContext(), eventName,
                SpeechSynthesisEventInit(u, 0, timestamp() - u->startTime(),
                                         String::emptyString));
            u->dispatchEventByUA(e);

            t->utteranceList().erase(id);
        }

        // NOTE : Hold this code according to VD requirement for now.
        Element* element = t->element();
        if (!u && element) {
            String* eventName =
                element->starfish()->staticStrings()->m_ttsend.localName();
            Event* e = new Event(element->executionContext(), eventName);
            element->window()->dispatchEventByUA(e);
        }
    }
}

static void completedCB(tts_h handle, int utteranceId, void* data)
{
    dispatchCompleteEvent((TTS*)data, utteranceId);
    ((TTS*)data)->setCurrentUtterId(0);
}

static bool supportedVoiceCB(tts_h handle, const char* language, int voiceType,
                             void* data)
{
    TTS* t = (TTS*)data;
    if (t != nullptr) {
        t->supportedVoiceList().insert(std::make_pair(
            String::fromUTF8(language, strlen(language)), voiceType));
        return true;
    }
    return false;
}

static void defaultVoiceChangedCB(tts_h handle, const char* prevLang,
                                  int prevVoiceType, const char* curLang,
                                  int curVoiceType, void* data)
{
    TTS* t = (TTS*)data;
    if (t != nullptr) {
        t->changeDefaultVoice(String::fromUTF8(curLang, strlen(curLang)),
                              curVoiceType);
    }
}

static int voiceSpeed(tts_h handle, float rate)
{
    const float rateMin = 0.6f, rateMax = 2.0f, rateNormal = 1.0f;
    int speedMin = 0, speedNormal = 0, speedMax = 0;
    int ret = tts_get_speed_range(handle, &speedMin, &speedNormal, &speedMax);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_get_speed_range() failed");
        return TTS_SPEED_AUTO;
    }

    // Epsilon value used to compare float values to zero
    const float epsilon = 1e-8f;
    if (abs(rate - rateNormal) < epsilon) {
        return TTS_SPEED_AUTO;
    }

    if (rate <= rateMin) {
        return speedMin;
    }
    if (rate >= rateMax) {
        return speedMax;
    }

    // Piecewise linear interpolation from |rate| to TTS internal speed value.
    if (rate < rateNormal) {
        return (speedMin +
                (rate - rateMin) *
                    ((speedNormal - speedMin) / (rateNormal - rateMin)));
    } else {
        return (speedNormal +
                (rate - rateNormal) *
                    ((speedMax - speedNormal) / (rateMax - rateNormal)));
    }
}

void TTS::initialize()
{
    int result = 0;
    if (vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &result) != 0) {
        return;
    }
    setAccessibilityMode(result == 1);

    // Add listener
    vconf_notify_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS,
                             accessibilityChangedCB, this);

    if (m_handle == NULL) {
        ecore_main_loop_thread_safe_call_async(
            [](void* data) -> void {
                TTS* tts = (TTS*)data;
                int ret = tts->createHandle();
                if (ret != TTS_ERROR_NONE) {
                    dispatchErrorEvent(tts, 0, errorToString(ret),
                                       errorToString(ret));
                }
            },
            this);
    }
}

int TTS::createHandle()
{
    // Create TTS Handle
    int ret = tts_create(&m_handle);
    if (ret != TTS_ERROR_NONE || m_handle == NULL) {
        STARFISH_LOG_ERROR("[TTS] tts_create failed : %d", ret);
        return ret;
    }

    // Get Default Setting
    char* lang = nullptr;
    int voiceType = 0;
    ret = tts_get_default_voice(m_handle, &lang, &voiceType);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_get_default_voice failed : %d", ret);
        m_defaultLanguage = String::fromUTF8("en_GB");
        m_defaultVoiceType = TTS_VOICE_TYPE_AUTO;
    } else {
        m_defaultLanguage = String::fromUTF8(lang, strlen(lang));
        m_defaultVoiceType = voiceType;
        free(lang);
    }

    // Set callback functions
    ret = tts_set_state_changed_cb(m_handle, stateChangedCB, this);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_set_state_changed_cb failed : %d", ret);
        return ret;
    }

    ret = tts_set_error_cb(m_handle, errorCB, this);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_set_error_cb failed : %d", ret);
        return ret;
    }

    ret = tts_set_utterance_started_cb(m_handle, startedCB, this);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_set_utterance_started_cb failed : %d",
                           ret);
        return ret;
    }

    ret = tts_set_utterance_completed_cb(m_handle, completedCB, this);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_INFO("[TTS] tts_set_utterance_completed_cb failed : %d",
                          ret);
        return ret;
    }

    ret = tts_foreach_supported_voices(m_handle, supportedVoiceCB, this);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_INFO("[TTS] tts_foreach_supported_voices failed : %d",
                          ret);
        return ret;
    }

    ret =
        tts_set_default_voice_changed_cb(m_handle, defaultVoiceChangedCB, this);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_INFO("[TTS] tts_set_default_voice_changed_cb failed : %d",
                          ret);
        return ret;
    }

    // Initialize engine
    return prepare();
}

void TTS::setMode(LWE::TTSMode mode)
{
    if (m_mode != mode) {
        unprepare();
        m_mode = mode;
        prepare();
    }
}

int TTS::prepare()
{
    tts_mode_e mode = TTS_MODE_SCREEN_READER;
    if (m_mode == LWE::TTSMode::Forced) {
        STARFISH_LOG_INFO("[TTS] LWE::TTSMode::Forced");
        mode = TTS_MODE_DEFAULT;
    }

    int ret = tts_set_mode(m_handle, mode);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_set_mode failed : %d", ret);
        return ret;
    }

    ret = tts_prepare(m_handle);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_prepare failed : %d", ret);
    }

    return ret;
}

void TTS::unprepare()
{
    int state = ttsState();
    if (state == TTS_STATE_PLAYING || state == TTS_STATE_PAUSED) {
        int ret = tts_stop(m_handle);
        if (ret != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("[TTS] tts_stop failed : %d", ret);
        }
        ret = tts_unprepare(m_handle);
        if (ret != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("[TTS] tts_unprepare failed : %d", ret);
        }
    } else if (state == TTS_STATE_READY) {
        int ret = tts_unprepare(m_handle);
        if (ret != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("[TTS] tts_unprepare failed : %d", ret);
        }
    }
}

void TTS::destroy()
{
    if (m_handle) {
        unprepare();

        int ret = tts_unset_state_changed_cb(m_handle);
        if (ret != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("[TTS] tts_unset_state_changed_cb failed : %d",
                               ret);
        }

        ret = tts_unset_utterance_started_cb(m_handle);
        if (ret != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR(
                "[TTS] tts_unset_utterance_started_cb failed : %d", ret);
        }

        ret = tts_unset_error_cb(m_handle);
        if (ret != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("[TTS] tts_unset_error_cb failed : %d", ret);
        }

        ret = tts_unset_utterance_completed_cb(m_handle);
        if (ret != TTS_ERROR_NONE) {
            STARFISH_LOG_INFO(
                "[TTS] tts_unset_utterance_completed_cb failed : %d", ret);
        }

        ret = tts_unset_default_voice_changed_cb(m_handle);
        if (ret != TTS_ERROR_NONE) {
            STARFISH_LOG_INFO(
                "[TTS] tts_unset_default_voice_changed_cb failed : %d", ret);
        }

        ret = tts_destroy(m_handle);
        if (ret != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("[TTS] tts_destroy failed : %d", ret);
        }

        STARFISH_LOG_INFO("[TTS] destroyed successfully");
        m_handle = NULL;
    } else {
        STARFISH_LOG_INFO("[TTS] handle is null in destroyTTSHandle()");
    }

    vconf_ignore_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS,
                             accessibilityChangedCB);
}

void TTS::speech(SpeechSynthesisUtterance* utterance)
{
    if (!utterance || utterance->text()->length() == 0) {
        return;
    }

    struct Dummy {
        TTS* t;
        SpeechSynthesisUtterance* u;
    };

    Dummy* d = new Dummy();
    d->t = this;
    d->u = utterance;

    ecore_main_loop_thread_safe_call_async(
        [](void* data) -> void {
            STARFISH_LOG_INFO("[TTS] speech(SpeechSynthesisUtterance*)");
            Dummy* d = (Dummy*)data;
            TTS* tts = d->t;
            SpeechSynthesisUtterance* utter = d->u;

            if (tts->handle() != NULL) {
                tts->setUtterance(utter);

                int ret = tts->speechUtterances();
                if (ret != TTS_ERROR_NONE) {
                    dispatchErrorEvent(tts, tts->utterance()->id(),
                                       errorToString(ret), errorToString(ret));
                }
            } else {
                STARFISH_LOG_ERROR("[TTS] handle is null");
            }
            delete (d);
        },
        d);
}

int TTS::speechUtterances()
{
    STARFISH_LOG_INFO("[TTS] speechUtterances()");

    String* curLang = m_defaultLanguage;
    String* lang = m_utterance->lang();
    if (!lang->isEmpty()) {
        if (lang->charAt(2) == '-') {
            curLang = lang->substring(0, 2)->concat('_')->concat(
                lang->substring(3, lang->length() - 3));
        } else {
            curLang = lang;
        }
    }

    String* utterText = m_utterance->text();
    int voiceType = m_defaultVoiceType;
    if (m_utterance->voice()) {
        voiceType = stringToVoiceType(m_utterance->voice()->name());
    }
    int textSpeed = voiceSpeed(m_handle, m_utterance->rate());
    int ret = tts_add_text(m_handle, CSTR(utterText), CSTR(curLang), voiceType,
                           textSpeed, &gUtteranceId);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_add_text failed : %d", ret);
        return ret;
    }

    // NOTE: In only case that an utterance is well inserted to TTS queue, set
    // it in our TTS instance.
    utterance()->setId(gUtteranceId);
    utteranceList().insert(std::make_pair(gUtteranceId, utterance()));

    tts_state_e curState;
    ret = tts_get_state(m_handle, &curState);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_get_state failed : %d", ret);
        return ret;
    }

    if (TTS_STATE_PLAYING != curState && TTS_STATE_PAUSED != curState) {
        ret = tts_play(m_handle);
    }

    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_play failed : %d", ret);
    }

    return ret;
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

                STARFISH_LOG_INFO("[TTS] speech TV : %s\n", text);
                if (tts->handle() != NULL) {
                    int ret = tts->speechElementText(text);
                    if (ret != TTS_ERROR_NONE) {
                        dispatchErrorEvent(tts, 0, errorToString(ret),
                                           errorToString(ret));
                    }
                } else {
                    STARFISH_LOG_ERROR("[TTS] handle is null");
                }

                free(text);
                delete d;
            },
            d);
    }
}

int TTS::speechElementText(const char* text)
{
    int ret = tts_stop(m_handle);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_stop failed : %d", ret);
        return ret;
    }

    if (utteranceList().size() > 0) {
        utteranceList().clear();
    }

    ret = tts_add_text(m_handle, text, NULL, TTS_VOICE_TYPE_AUTO,
                       TTS_SPEED_AUTO, &gUtteranceId);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_add_text failed : %d", ret);
        return ret;
    }

    ret = tts_play(m_handle);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_play failed : %d", ret);
        return ret;
    }
    STARFISH_LOG_INFO("[TTS] tts_play : %s ", text);

    return ret;
}

void TTS::changeDefaultVoice(String* language, const int voiceType)
{
    m_defaultLanguage = language;
    m_defaultVoiceType = voiceType;
}

int TTS::ttsState()
{
    tts_state_e curState;
    int ret = tts_get_state(m_handle, &curState);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_get_state failed : %d", ret);
        dispatchErrorEvent(this, currentUtterId(), errorToString(ret),
                           errorToString(ret));
        return -1;
    }
    return curState;
}

bool TTS::isPaused()
{
    return (TTS_STATE_PAUSED == ttsState());
}

void TTS::pause()
{
    int utterId = currentUtterId();
    if (utteranceList().size() == 0 || isPaused() || utterId == 0) {
        return;
    }

    STARFISH_LOG_INFO("[TTS] Pause Speaking! [ID:%d] \n", utterId);
    int ret = tts_pause(m_handle);
    if (ret != TTS_ERROR_NONE) {
        dispatchErrorEvent(this, utterId, errorToString(ret),
                           errorToString(ret));
    }
}

void TTS::resume()
{
    int utterId = currentUtterId();
    if (utteranceList().size() == 0 || !isPaused()) {
        return;
    }

    STARFISH_LOG_INFO("[TTS] Resume Speaking! [ID:%d] \n", utterId);
    int ret = tts_play(m_handle);
    if (ret != TTS_ERROR_NONE) {
        dispatchErrorEvent(this, utterId, errorToString(ret),
                           errorToString(ret));
    }
}

void TTS::cancel()
{
    int utterId = currentUtterId();
    if (utteranceList().size() == 0) {
        return;
    }

    STARFISH_LOG_INFO("[TTS] Cancel Speaking! [ID:%d] \n", utterId);
    int ret = tts_stop(m_handle);
    if (ret != TTS_ERROR_NONE) {
        dispatchErrorEvent(this, utterId, errorToString(ret),
                           errorToString(ret));
    }

    dispatchCompleteEvent(this, utterId);
    utteranceList().clear();
}
}
#endif
