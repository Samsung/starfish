/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"

// Tizen TTS implementation for non-TV profiles (mobile/common/etc.).
// TV profiles keep their own implementation in TTSTV.cpp. The three TTS
// backends are mutually exclusive by guard:
//   TTSTV.cpp    : TIZEN && TTS &&  PROD_TV && GLIB
//   TTSBase.cpp  : TTS && (!TIZEN || (PROD_TV && !GLIB))
//   TTSTizen.cpp : TIZEN && TTS && !PROD_TV     (this file)
#if defined(STARFISH_TIZEN) && defined(STARFISH_ENABLE_TTS) && \
    !defined(STARFISH_TIZEN_PROD_TV)

#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/ErrorEvent.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/tts/TTS.h"
#include "core/modules/tts/SpeechSynthesisEvent.h"
#include "core/modules/profiling/Profiling.h"
#if defined(PORT_EVENTLOOP_BACKEND_GLIB) || defined(STARFISH_ENABLE_WORKER)
#include "platform/message_loop/RunLoopGLib.h"
#else
namespace Starfish {
// No engine-owned GMainContext on non-glib event loops: fall back to the
// default context.
static inline void* glibMainContext()
{
    return nullptr;
}
} // namespace Starfish
#endif

#include <vconf/vconf.h>
#include <glib.h>

// The accessibility TTS key lives in vconf-internal-setting-keys.h, which is
// not always present in non-TV rootstraps. Fall back to the literal key.
#ifndef VCONFKEY_SETAPPL_ACCESSIBILITY_TTS
#define VCONFKEY_SETAPPL_ACCESSIBILITY_TTS "db/setting/accessibility/tts"
#endif
// Some settings apps (e.g. Family Hub) toggle only this preview twin of the
// key and never touch (or even delete) the base key, so both must be
// watched; accessibility is on when either reads true.
#define VCONFKEY_SETAPPL_ACCESSIBILITY_TTS_TEMPORARY \
    "db/setting/accessibility/tts/temporary"

#define TTS_REMOVED_INSTANCE_SIZE 5

namespace Starfish {

// False only when neither key is readable; enabled is the OR of both.
static bool readAccessibilityVconf(bool& enabled)
{
    int base = 0, temporary = 0;
    int baseOk = vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &base);
    int temporaryOk = vconf_get_bool(
        VCONFKEY_SETAPPL_ACCESSIBILITY_TTS_TEMPORARY, &temporary);
    if (baseOk != 0 && temporaryOk != 0) {
        return false;
    }
    enabled =
        (baseOk == 0 && base == 1) || (temporaryOk == 0 && temporary == 1);
    return true;
}

int gUtteranceId = 1;

static std::vector<TTS*> gRemovedInstance;

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
static bool isValidTTS(TTS* t)
{
    if (t != nullptr) {
        auto iter =
            std::find(gRemovedInstance.begin(), gRemovedInstance.end(), t);
        if (iter == gRemovedInstance.end()) {
            return true;
        }
    }
    return false;
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

static void dispatchStartEvent(TTS* t, int id)
{
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

        Optional<Element*> element = t->lastSpeechElement();
        if (!u && element) {
            String* eventName =
                element->starfish()->staticStrings()->m_ttsstart.localName();
            Event* e = new Event(element->executionContext(), eventName);
            element->window()->dispatchEventByUA(e);
        }
    }
}

static void dispatchCompleteEvent(TTS* t, int id)
{
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

        Optional<Element*> element = t->lastSpeechElement();
        if (!u && element) {
            String* eventName =
                element->starfish()->staticStrings()->m_ttsend.localName();
            Event* e = new Event(element->executionContext(), eventName);
            element->window()->dispatchEventByUA(e);
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
                Escargot::ValueRef::create(Escargot::StringRef::createFromASCII(
                    errorCode, strlen(errorCode))));
            Event* errorEvent = new ErrorEvent(
                u->executionContext(),
                u->webView()->starfish()->staticStrings()->m_error.localName(),
                errorInfo);
            u->dispatchEventByUA(errorEvent);
            t->utteranceList().erase(id);
        }

        // NOTE : Code below is required in case of accessibility.
        Optional<Element*> element = t->lastSpeechElement();
        if (u == nullptr && element) {
            ErrorEventInit errorInfo;
            errorInfo.setMessage(String::fromUTF8(errorMsg, strlen(errorMsg)));
            errorInfo.setError(
                Escargot::ValueRef::create(Escargot::StringRef::createFromASCII(
                    errorCode, strlen(errorCode))));
            element->window()->dispatchErrorEvent(errorInfo);
        }
    }
}

static void accessibilityChangedCB(keynode_t* keynodeName, void* data)
{
    // Do not free data
    TTS* t = (TTS*)data;
    bool enabled = false;
    if (!readAccessibilityVconf(enabled)) {
        return;
    }
    if (isValidTTS(t)) {
        t->setAccessibilityMode(enabled);
    }
}

static void stateChangedCB(tts_h handle, tts_state_e prev, tts_state_e cur,
                           void* data)
{
    TTS* t = (TTS*)data;
    if (!isValidTTS(t)) {
        return;
    }

    int utterId = t->currentUtterId();

    if (prev == TTS_STATE_PAUSED && cur == TTS_STATE_PLAYING && utterId) {
        SpeechSynthesisUtterance* u = findUtterance(t, utterId);
        if (u) {
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
        SpeechSynthesisUtterance* u = findUtterance(t, utterId);
        if (u) {
            String* eventName =
                u->webView()->starfish()->staticStrings()->m_pause.localName();
            SpeechSynthesisEvent* e = new SpeechSynthesisEvent(
                u->executionContext(), eventName,
                SpeechSynthesisEventInit(u, 0, timestamp() - u->startTime(),
                                         String::emptyString));
            u->dispatchEventByUA(e);
        }
    } else if (prev == TTS_STATE_CREATED && cur == TTS_STATE_READY) {
        t->ttsPlay();
    } else if (prev == TTS_STATE_READY && cur == TTS_STATE_CREATED) {
    }
}

static void ttsErrorCB(tts_h handle, int utteranceId, tts_error_e reason,
                       void* data)
{
    dispatchErrorEvent((TTS*)data, utteranceId, errorToString(reason),
                       errorToString(reason));
}

static void utteranceStartedCB(tts_h handle, int utteranceId, void* data)
{
    TTS* t = (TTS*)data;
    t->setCurrentUtterId(utteranceId);
    dispatchStartEvent(t, utteranceId);
}

void utteranceCompletedCB(tts_h handle, int utteranceId, void* data)
{
    TTS* t = (TTS*)data;
    dispatchCompleteEvent(t, utteranceId);
    t->setCurrentUtterId(0);

    if (t->m_pendingSpeech.first) {
        t->m_lastSpeechElement = t->m_pendingSpeech.first;
        String* text = t->m_pendingSpeech.second;
        int ret =
            tts_add_text(t->m_handle, CSTR(text), NULL, TTS_VOICE_TYPE_AUTO,
                         TTS_SPEED_AUTO, &gUtteranceId);
        if (ret != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("[TTS] tts_add_text failed : %d", ret);
        }
        t->m_pendingSpeech.first = nullptr;
        t->m_pendingSpeech.second = String::emptyString;
    } else if (gUtteranceId == utteranceId) {
        t->unprepare();
    }
}

static bool supportedVoiceCB(tts_h handle, const char* language, int voiceType,
                             void* data)
{
    TTS* t = (TTS*)data;
    if (isValidTTS(t)) {
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
    if (isValidTTS(t)) {
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
    if (std::abs(rate - rateNormal) < epsilon) {
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
        return (speedMin + (rate - rateMin) * ((speedNormal - speedMin) /
                                               (rateNormal - rateMin)));
    } else {
        return (speedNormal + (rate - rateNormal) * ((speedMax - speedNormal) /
                                                     (rateMax - rateNormal)));
    }
}

void TTS::initialize()
{
    gRemovedInstance.erase(
        std::remove(gRemovedInstance.begin(), gRemovedInstance.end(), this),
        gRemovedInstance.end());
    m_handle = nullptr;
    bool enabled = false;
    if (!readAccessibilityVconf(enabled)) {
        return;
    }
    setAccessibilityMode(enabled);

    // Add listener
    vconf_notify_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS,
                             accessibilityChangedCB, this);
    vconf_notify_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS_TEMPORARY,
                             accessibilityChangedCB, this);

    if (m_handle == NULL) {
        guint* idleIdPtr = new guint(0);

        // Attach to the engine's own GMainContext, not the default one: when
        // the engine runs on a dedicated thread, createHandle() allocates
        // GC-managed Strings, and doing that from the app's main thread
        // corrupts the collector (SIGSEGV inside GC_malloc). glibMainContext()
        // is null when the engine shares the app main loop, which restores the
        // default-context behaviour.
        GMainContext* context =
            reinterpret_cast<GMainContext*>(glibMainContext());
        GSource* source = g_idle_source_new();
        g_source_set_ready_time(source, -1);
        g_source_set_priority(source, G_PRIORITY_DEFAULT);
        g_source_set_callback(
            source,
            (GSourceFunc)[](gpointer data)->gboolean {
                std::pair<TTS*, guint*>* pair = (std::pair<TTS*, guint*>*)data;
                TTS* t = pair->first;
                guint* idleIdPtr = pair->second;
                t->removeCallbackId(*idleIdPtr);
                int ret = t->createHandle();
                if (ret != TTS_ERROR_NONE) {
                    dispatchErrorEvent(t, 0, errorToString(ret),
                                       errorToString(ret));
                }
                return G_SOURCE_REMOVE;
            },
            new std::pair<TTS*, guint*>(this, idleIdPtr),
            [](gpointer data) {
                std::pair<TTS*, guint*>* pair = (std::pair<TTS*, guint*>*)data;
                delete pair->second;
                delete pair;
            });
        guint callbackId = g_source_attach(source, context);
        g_source_set_ready_time(source, 0);
        g_source_unref(source);

        addCallbackId(callbackId);
        *idleIdPtr = callbackId;
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

    ret = tts_set_error_cb(m_handle, ttsErrorCB, this);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_set_error_cb failed : %d", ret);
        return ret;
    }

    ret = tts_set_utterance_started_cb(m_handle, utteranceStartedCB, this);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_set_utterance_started_cb failed : %d",
                           ret);
        return ret;
    }

    ret = tts_set_utterance_completed_cb(m_handle, utteranceCompletedCB, this);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_set_utterance_completed_cb failed : %d",
                           ret);
        return ret;
    }

    ret = tts_foreach_supported_voices(m_handle, supportedVoiceCB, this);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_foreach_supported_voices failed : %d",
                           ret);
        return ret;
    }

    ret =
        tts_set_default_voice_changed_cb(m_handle, defaultVoiceChangedCB, this);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_set_default_voice_changed_cb failed : %d",
                           ret);
        return ret;
    }

    // Set mode after callbacks are registered. SCREEN_READER mode gates
    // speech on the system accessibility setting; Forced mode speaks
    // regardless (developer/embedder override).
    tts_mode_e mode = TTS_MODE_SCREEN_READER;
    if (m_lweTTSMode == LWE::TTSMode::Forced) {
        mode = TTS_MODE_DEFAULT;
    }
    ret = tts_set_mode(m_handle, mode);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_set_mode failed : %d", ret);
        return ret;
    }

    return TTS_ERROR_NONE;
}

void TTS::destroy()
{
    STARFISH_LOG_ERROR("[TTS] TTS::destroy");

    for (guint id : m_callbackIds) {
        GMainContext* context =
            reinterpret_cast<GMainContext*>(glibMainContext());
        GSource* source = g_main_context_find_source_by_id(context, id);
        if (source) {
            g_source_destroy(source);
        }
    }
    clearCallbackIds();

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
            STARFISH_LOG_ERROR(
                "[TTS] tts_unset_utterance_completed_cb failed : %d", ret);
        }

        ret = tts_unset_default_voice_changed_cb(m_handle);
        if (ret != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR(
                "[TTS] tts_unset_default_voice_changed_cb failed : %d", ret);
        }

        ret = tts_destroy(m_handle);
        if (ret != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("[TTS] tts_destroy failed : %d", ret);
        }

        if (gRemovedInstance.size() >= TTS_REMOVED_INSTANCE_SIZE) {
            gRemovedInstance.erase(gRemovedInstance.begin());
        }
        gRemovedInstance.push_back(this);
        m_handle = NULL;
    } else {
        STARFISH_LOG_ERROR("[TTS] handle is null in destroyTTSHandle()");
    }

    vconf_ignore_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS,
                             accessibilityChangedCB);
    vconf_ignore_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS_TEMPORARY,
                             accessibilityChangedCB);
}

void TTS::setMode(LWE::TTSMode lweTTSMode)
{
    if (m_lweTTSMode != lweTTSMode) {
        unprepare();
        tts_mode_e mode = TTS_MODE_SCREEN_READER;
        if (lweTTSMode == LWE::TTSMode::Forced) {
            mode = TTS_MODE_DEFAULT;
        }

        int ret = tts_set_mode(m_handle, mode);
        if (ret != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("[TTS] tts_set_mode failed : %d", ret);
            return;
        }

        m_lweTTSMode = lweTTSMode;
    }
}

int TTS::prepare()
{
    int state = ttsState();
    int ret = TTS_ERROR_NONE;
    if (state == TTS_STATE_CREATED) {
        ret = tts_prepare(m_handle);
        if (ret != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("[TTS] tts_prepare failed : %d", ret);
            return ret;
        }
    }
    return ret;
}

void TTS::unprepare()
{
    STARFISH_LOG_INFO("[TTS] unprepare() start");
    m_lastSpeechElement = nullptr;
    int state = ttsState();
    if (state != TTS_STATE_CREATED) {
        int ret = TTS_ERROR_NONE;
        if (state == TTS_STATE_PLAYING || state == TTS_STATE_PAUSED) {
            ttsStop();
        }

        ret = tts_unprepare(m_handle);
        if (ret != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("[TTS] tts_unprepare failed : %d", ret);
        }
    }
}

void TTS::speech(Element* element, String* text)
{
    if (!text->isEmpty()) {
        m_pendingSpeech.first = element;
        m_pendingSpeech.second = text;
        TTS* t = (TTS*)this;
        if (t->handle() != NULL) {
            int ret = t->speechElementText();
            if (ret != TTS_ERROR_NONE) {
                dispatchErrorEvent(t, 0, errorToString(ret),
                                   errorToString(ret));
            }
        } else {
            STARFISH_LOG_ERROR("[TTS] handle is null");
        }
    }
}

int TTS::speechElementText()
{
    int state = ttsState();
    int ret = TTS_ERROR_NONE;
    if (state == TTS_STATE_CREATED) {
        return prepare();
    } else {
        ttsStop();
    }

    ttsPlay();

    return ret;
}

int TTS::ttsPlay()
{
    if (!m_pendingSpeech.first) {
        return TTS_ERROR_NONE;
    }

    int ret = TTS_ERROR_NONE;

    m_lastSpeechElement = m_pendingSpeech.first;
    String* text = m_pendingSpeech.second;

    const char* language = nullptr;
    if (m_userLanguage.size()) {
        language = m_userLanguage.c_str();
    }
    // If the variable language is still nullptr, TTS Engine will use
    // system-defined value.
    ret = tts_add_text(m_handle, CSTR(text), language, TTS_VOICE_TYPE_AUTO,
                       TTS_SPEED_AUTO, &gUtteranceId);
    m_pendingSpeech.first = nullptr;
    m_pendingSpeech.second = String::emptyString;
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_add_text failed : %d", ret);
        return ret;
    }

    ret = tts_play(m_handle);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_play failed : %d", ret);
        return ret;
    }

    return ret;
}

int TTS::ttsStop()
{
    bool needStop = false;
    int ret = 0;
    if (m_lweTTSMode != LWE::TTSMode::Forced) {
        bool enabled = false;
        if (!readAccessibilityVconf(enabled)) {
            STARFISH_LOG_ERROR("[TTS] accessibility vconf unreadable");
        }
        if (enabled) {
            needStop = true;
        }
    } else {
        needStop = true;
    }

    if (needStop) {
        // NOTE: tts_stop() interrupts playback WITHOUT firing
        // utterance_completed; only the state callback (PLAYING->READY) runs.
        // Queue-resume logic must not depend on the completed callback here.
        STARFISH_LOG_INFO("[TTS] tts_stop");
        ret = tts_stop(m_handle);
        if (ret != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("[TTS] tts_stop failed : %d", ret);
        }
    }
    return ret;
}

int TTS::ttsState()
{
    tts_state_e cur;
    int ret = tts_get_state(m_handle, &cur);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_get_state failed : %d", ret);
        dispatchErrorEvent(this, currentUtterId(), errorToString(ret),
                           errorToString(ret));
        return -1;
    }
    return cur;
}

void TTS::speech(SpeechSynthesisUtterance* utterance)
{
    if (!utterance || utterance->text()->isEmpty()) {
        return;
    }

    struct Dummy {
        TTS* t;
        SpeechSynthesisUtterance* u;
        guint* callbackId;
    };

    Dummy* d = new Dummy();
    d->t = this;
    d->u = utterance;
    d->callbackId = new guint(0);

    GMainContext* context = reinterpret_cast<GMainContext*>(glibMainContext());
    GSource* source = g_idle_source_new();
    g_source_set_ready_time(source, -1);
    g_source_set_priority(source, G_PRIORITY_DEFAULT);
    g_source_set_callback(
        source,
        (GSourceFunc)[](gpointer data)->gboolean {
            Dummy* d = (Dummy*)data;
            TTS* t = d->t;
            t->removeCallbackId(*d->callbackId);

            if (t->handle() != NULL) {
                t->setUtterance(d->u);

                int ret = t->speechUtterances();
                if (ret != TTS_ERROR_NONE) {
                    dispatchErrorEvent(t, t->utterance()->id(),
                                       errorToString(ret), errorToString(ret));
                }
            } else {
                STARFISH_LOG_ERROR("[TTS] handle is null");
            }
            return G_SOURCE_REMOVE;
        },
        d,
        [](gpointer data) {
            Dummy* d = (Dummy*)data;
            delete d->callbackId;
            delete d;
        });
    guint callbackId = g_source_attach(source, context);
    g_source_set_ready_time(source, 0);
    g_source_unref(source);

    addCallbackId(callbackId);
    *d->callbackId = callbackId;
}

int TTS::speechUtterances()
{
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

void TTS::changeDefaultVoice(String* language, const int voiceType)
{
    m_defaultLanguage = language;
    m_defaultVoiceType = voiceType;
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

    int ret = tts_play(m_handle);
    if (ret != TTS_ERROR_NONE) {
        dispatchErrorEvent(this, utterId, errorToString(ret),
                           errorToString(ret));
    }
}

void TTS::cancel()
{
    STARFISH_LOG_INFO("[TTS] Cancel speaking!");
    int utterId = currentUtterId();
    if (utteranceList().size() == 0) {
        // whatever we try to stop tts
        ttsStop();
        return;
    }

    STARFISH_LOG_INFO("[TTS] Cancel speaking! [ID:%d] ", utterId);
    int ret = ttsStop();
    if (ret != TTS_ERROR_NONE) {
        dispatchErrorEvent(this, utterId, errorToString(ret),
                           errorToString(ret));
    }

    dispatchCompleteEvent(this, utterId);
    utteranceList().clear();
}
} // namespace Starfish
#endif
