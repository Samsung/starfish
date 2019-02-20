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
#include "core/dom/Event.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/tts/TTS.h"

#include <Elementary.h>
#include <vconf/vconf.h>

namespace Starfish {

uint32_t gUtteranceId = 1;

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

const char* errorToString(tts_error_e error)
{
    switch (error) {
    case TTS_ERROR_NONE:
        return "Successful";
    case TTS_ERROR_OUT_OF_MEMORY:
        return "Out of Memory";
    case TTS_ERROR_IO_ERROR:
        return "I/O error";
    case TTS_ERROR_INVALID_PARAMETER:
        return "Invalid parameter";
    case TTS_ERROR_OUT_OF_NETWORK:
        return "Out of network";
    case TTS_ERROR_INVALID_STATE:
        return "Invalid state";
    case TTS_ERROR_INVALID_VOICE:
        return "Invalid voice";
    case TTS_ERROR_ENGINE_NOT_FOUND:
        return "No available engine";
    case TTS_ERROR_TIMED_OUT:
        return "No answer from the daemon";
    case TTS_ERROR_OPERATION_FAILED:
        return "Operation failed";
    case TTS_ERROR_AUDIO_POLICY_BLOCKED:
        return "Audio policy blocked";
    default:
        return "Unknown Error";
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

static SpeechSynthesisUtterance* findUtterance(TTS* t, uint32_t id)
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
    TTS* t = (TTS*)data;
    if (!t) {
        return;
    }

    uint32_t id = 0;
    if (t->utterance()) {
        id = t->utterance()->id();
    }

    STARFISH_LOG_INFO("[TTS] State changed Callback [ID:%u]: [%s] to [%s]\n",
                      id, stateToString(prev), stateToString(cur));

    if (prev == TTS_STATE_CREATED && cur == TTS_STATE_READY) {
        t->readyState();
    } else if (prev == TTS_STATE_PAUSED && cur == TTS_STATE_PLAYING && id) {
        SpeechSynthesisUtterance* u = findUtterance(t, id);
        if (u) {
            STARFISH_LOG_INFO("[TTS] TTS_STATE_RESUME !!!!");
            t->setPaused(false);
            String* eventName =
                u->webView()->starfish()->staticStrings()->m_resume.localName();
            Event* e = new Event(
                u->webView()->mainBrowsingContext()->document(), eventName);
            u->dispatchEventByUA(e);
        }
    } else if (prev == TTS_STATE_PLAYING && cur == TTS_STATE_PAUSED && id) {
        SpeechSynthesisUtterance* u = findUtterance(t, id);
        if (u) {
            STARFISH_LOG_INFO("[TTS] TTS_STATE_PAUSED !!!!");
            t->setPaused(true);
            String* eventName =
                u->webView()->starfish()->staticStrings()->m_pause.localName();
            Event* e = new Event(
                u->webView()->mainBrowsingContext()->document(), eventName);
            u->dispatchEventByUA(e);
        }
    }
}

static void dispatchErrorEvent(TTS* t, uint32_t id, const char* errorMsg)
{
    STARFISH_LOG_ERROR("[TTS] Occurred Error [ID:%u] : %s", id, errorMsg);
    SpeechSynthesisUtterance* u = findUtterance(t, id);
    if (u) {
        String* eventName =
            u->webView()->starfish()->staticStrings()->m_error.localName();
        Event* e = new Event(u->webView()->mainBrowsingContext()->document(),
                             eventName);
        u->dispatchEventByUA(e);
        t->utteranceList().erase(id);
    }
}

static void errorCB(tts_h handle, int utteranceId, tts_error_e reason,
                    void* data)
{
    TTS* t = (TTS*)data;
    if (t && t->utterance()) {
        dispatchErrorEvent(t, utteranceId, errorToString(reason));
    }
}

static void dispatchStartEvent(TTS* t, uint32_t id)
{
    if (t) {
        // NOTE : Hold this code according to VD requirement for now.
        Element* element = t->element();
        if (element) {
            String* eventName =
                element->starfish()->staticStrings()->m_ttsstart.localName();
            Event* e = new Event(element->document(), eventName);
            element->window()->dispatchEventByUA(e);
        }

        SpeechSynthesisUtterance* u = findUtterance(t, id);
        if (u) {
            t->setUtterance(u);
            String* eventName =
                u->webView()->starfish()->staticStrings()->m_start.localName();
            Event* e = new Event(
                u->webView()->mainBrowsingContext()->document(), eventName);
            u->dispatchEventByUA(e);
        }
    }
}

static void startedCB(tts_h handle, int utteranceId, void* data)
{
    STARFISH_LOG_INFO("[TTS] Started Speaking! [ID:%d]\n", utteranceId);
    dispatchStartEvent((TTS*)data, utteranceId);
}

static void dispatchCompleteEvent(TTS* t, uint32_t id)
{
    if (t) {
        // NOTE : Hold this code according to VD requirement for now.
        Element* element = t->element();
        if (element) {
            String* eventName =
                element->starfish()->staticStrings()->m_ttsend.localName();
            Event* e = new Event(element->document(), eventName);
            element->window()->dispatchEventByUA(e);
        }

        SpeechSynthesisUtterance* u = findUtterance(t, id);
        if (u) {
            String* eventName =
                u->webView()->starfish()->staticStrings()->m_end.localName();
            Event* e = new Event(
                u->webView()->mainBrowsingContext()->document(), eventName);
            u->dispatchEventByUA(e);

            t->utteranceList().erase(id);
        }
    }
}

static void completedCB(tts_h handle, int utteranceId, void* data)
{
    STARFISH_LOG_INFO("[TTS] Completed Speaking! \n");
    dispatchCompleteEvent((TTS*)data, utteranceId);
}

static bool supportedVoiceCB(tts_h handle, const char* language, int voiceType,
                             void* data)
{
    TTS* t = (TTS*)data;
    if (t) {
        t->supportedVoiceList().insert(
            std::make_pair(String::fromUTF8(language), voiceType));
        return true;
    }
    return false;
}

static void defaultVoiceChangedCB(tts_h handle, const char* prevLang,
                                  int prevVoiceType, const char* curLang,
                                  int curVoiceType, void* data)
{
    TTS* t = (TTS*)data;
    if (t) {
        t->changeDefaultVoice(String::fromUTF8(curLang), curVoiceType);
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
                tts->createHandle();
            },
            this);
    }
}

void TTS::destroy()
{
    if (m_handle) {
        int ret = tts_stop(m_handle);
        if (ret != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("[TTS] tts_stop failed : %d", ret);
        }

        ret = tts_unprepare(m_handle);
        if (ret != TTS_ERROR_NONE) {
            STARFISH_LOG_ERROR("[TTS] tts_unprepare failed : %d", ret);
        }

        ret = tts_unset_state_changed_cb(m_handle);
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
}

void TTS::readyState()
{
    m_readyState = true;
    if (m_waitingState) {
        ecore_main_loop_thread_safe_call_async(
            [](void* data) -> void {
                STARFISH_LOG_INFO("[TTS] readyState()");
                ((TTS*)data)->speakUtterances();
            },
            this);
    }
}

void TTS::speak(SpeechSynthesisUtterance* utterance)
{
    if (!utterance || utterance->text()->length() == 0) {
        return;
    }

    if (!m_readyState) {
        m_waitingState = true;
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
            Dummy* d = (Dummy*)data;

            TTS* t = d->t;
            SpeechSynthesisUtterance* utter = d->u;

            uint32_t id = gUtteranceId++;
            utter->setId(id);

            t->setUtterance(utter);
            t->utteranceList().insert(std::make_pair(id, utter));

            STARFISH_LOG_INFO("[TTS] speak(SpeechSynthesisUtterance*)");
            t->setPaused(false);
            t->speakUtterances();

            delete (d);
        },
        d);
}

void TTS::speakUtterances()
{
    STARFISH_LOG_INFO("[TTS] speakUtterances()");
    String* utterText = m_utterance->text();
    if (utterText->isEmpty()) {
        return;
    }

    String* curLang = m_defaultLanguage;
    String* lang = m_utterance->lang();

    if (!lang->isEmpty() && lang->charAt(2) == '-') {
        curLang = lang->substring(0, 2)->concat('_')->concat(
            lang->substring(3, lang->length() - 3));
    }

    int voiceType = m_defaultVoiceType;
    if (m_utterance->voice()) {
        voiceType = stringToVoiceType(m_utterance->voice()->name());
    }
    int textSpeed = voiceSpeed(m_handle, m_utterance->rate());
    int utteranceId = (int)m_utterance->id();

    int ret = tts_add_text(m_handle, CSTR(utterText), CSTR(curLang), voiceType,
                           textSpeed, &utteranceId);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_add_text failed : %d", ret);
        dispatchErrorEvent(this, m_utterance->id(),
                           errorToString((tts_error_e)ret));
        return;
    }

    tts_state_e curState;
    tts_get_state(m_handle, &curState);
    if (TTS_STATE_PLAYING != curState) {
        ret = tts_play(m_handle);
    }

    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_play failed : %d", ret);
        dispatchErrorEvent(this, m_utterance->id(),
                           errorToString((tts_error_e)ret));
        return;
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

                STARFISH_LOG_INFO("[TTS] speech TV : %s\n", text);
                tts->startPlay(text);

                free(text);
                delete d;
            },
            d);
    }
}

bool TTS::createHandle()
{
    // Create TTS Handle
    int ret = tts_create(&m_handle);
    if (ret != TTS_ERROR_NONE || m_handle == NULL) {
        STARFISH_LOG_ERROR("[TTS] tts_create failed : %d", ret);
        return false;
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
        m_defaultLanguage = String::fromUTF8(lang);
        m_defaultVoiceType = voiceType;
        free(lang);
    }

    // Set callback functions
    ret = tts_set_state_changed_cb(m_handle, stateChangedCB, this);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_set_state_changed_cb failed : %d", ret);
        return false;
    }

    ret = tts_set_error_cb(m_handle, errorCB, this);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_set_error_cb failed : %d", ret);
        return false;
    }

    ret = tts_set_utterance_started_cb(m_handle, startedCB, this);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_set_utterance_started_cb failed : %d",
                           ret);
        return false;
    }

    ret = tts_set_utterance_completed_cb(m_handle, completedCB, this);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_INFO("[TTS] tts_set_utterance_completed_cb failed : %d",
                          ret);
        return false;
    }

    ret = tts_foreach_supported_voices(m_handle, supportedVoiceCB, this);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_INFO("[TTS] tts_foreach_supported_voices failed : %d",
                          ret);
        return false;
    }

    ret =
        tts_set_default_voice_changed_cb(m_handle, defaultVoiceChangedCB, this);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_INFO("[TTS] tts_set_default_voice_changed_cb failed : %d",
                          ret);
        return false;
    }

    // Initialize engine
    ret = tts_set_mode(m_handle, TTS_MODE_DEFAULT);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_set_mode failed : %d", ret);
        return false;
    }

    ret = tts_prepare(m_handle);
    if (ret != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_prepare failed : %d", ret);
        return false;
    }

    return true;
}

bool TTS::startPlay(const char* text)
{
    if (m_handle == NULL) {
        STARFISH_LOG_ERROR("[TTS] handle is null in startTTSPlay()");
        return false;
    }

    addText(text);
    return true;
}

bool TTS::stopPlay()
{
    if (m_handle == NULL) {
        STARFISH_LOG_ERROR("[TTS] handle is null in stopTTSPlay()");
        return false;
    }

    int ret = 0;
    if ((ret = tts_stop(m_handle)) != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_stop failed : %d", ret);
        return false;
    }
    return true;
}

bool TTS::addText(const char* text)
{
    if (m_handle == NULL) {
        STARFISH_LOG_ERROR("[TTS] handle is null in addTTSText()");
        return false;
    }

    int ret = 0;
    int utt_id = 0;

    if ((ret = tts_stop(m_handle)) != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_stop failed : %d", ret);
        return false;
    }
    if ((ret = tts_add_text(m_handle, text, NULL, TTS_VOICE_TYPE_AUTO,
                            TTS_SPEED_AUTO, &utt_id)) != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_add_text failed : %d", ret);
        return false;
    }
    if ((ret = tts_play(m_handle)) != TTS_ERROR_NONE) {
        STARFISH_LOG_ERROR("[TTS] tts_play failed : %d", ret);
        return false;
    }
    STARFISH_LOG_INFO("[TTS] tts_play : %s ", text);

    return true;
}

void TTS::changeDefaultVoice(String* language, const int voiceType)
{
    m_defaultLanguage = language;
    m_defaultVoiceType = voiceType;
}

void TTS::pause()
{
    if (!utteranceList().size() || !utterance()->id() || m_isPaused) {
        return;
    }

    STARFISH_LOG_INFO("[TTS] Pause Speaking! [ID:%u] \n", utterance()->id());

    int ret = tts_pause(m_handle);
    if (ret != TTS_ERROR_NONE) {
        dispatchErrorEvent(this, utterance()->id(),
                           errorToString((tts_error_e)ret));
    }
}

void TTS::resume()
{
    if (!utteranceList().size() || !utterance()->id()) {
        return;
    }

    STARFISH_LOG_INFO("[TTS] Resume Speaking! [ID:%u] \n", utterance()->id());

    int ret = tts_play(m_handle);
    if (ret != TTS_ERROR_NONE) {
        dispatchErrorEvent(this, utterance()->id(),
                           errorToString((tts_error_e)ret));
    }
}

void TTS::cancel()
{
    if (!utteranceList().size() || !utterance()->id()) {
        return;
    }

    STARFISH_LOG_INFO("[TTS] Cancel Speaking! [ID:%u] \n", utterance()->id());

    int ret = tts_stop(m_handle);
    if (ret != TTS_ERROR_NONE) {
        dispatchErrorEvent(this, utterance()->id(),
                           errorToString((tts_error_e)ret));
    }
    dispatchCompleteEvent(this, utterance()->id());
}
}
#endif
