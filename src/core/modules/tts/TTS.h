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

#ifdef STARFISH_ENABLE_TTS
#ifndef __StarfishTTS__
#define __StarfishTTS__

#include "PlatformIntegrationData.h"
#include "binding/WebViewHoldable.h"

#if defined(STARFISH_TIZEN)
#include <tts.h>
#endif

#include <set>

namespace Starfish {

class Element;
class TTS : public gc, public WebViewHoldable {
public:
    TTS(WebView* webView)
        : WebViewHoldable(webView)
        , m_isAccessibilityMode(false)
        , m_lweTTSMode(LWE::TTSMode::Default)
        , m_utterance(nullptr)
        , m_userLanguage()
        , m_currentUtterId(0)
    {
        initialize();
        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                TTS* self = static_cast<TTS*>(obj);
                self->destroy();
            },
            nullptr, nullptr, nullptr);
    }

    bool isAccessibilityMode()
    {
        return m_isAccessibilityMode;
    }

    void setAccessibilityMode(bool value)
    {
        m_isAccessibilityMode = value;
    }

    LWE::TTSMode mode() const
    {
        return m_lweTTSMode;
    }

    void setMode(LWE::TTSMode lweTTSMode);

    std::string userLanguage() const
    {
        return m_userLanguage;
    }

    void setUserLanguage(const std::string& language)
    {
        m_userLanguage = language;
    }

    SpeechSynthesisUtterance* utterance()
    {
        return m_utterance;
    }

    void setUtterance(SpeechSynthesisUtterance* u)
    {
        m_utterance = u;
    }

    GCUnorderedMap<int, SpeechSynthesisUtterance*>& utteranceList()
    {
        return m_utteranceList;
    }

    GCUnorderedMap<String*, int>& supportedVoiceList()
    {
        return m_supportedVoiceList;
    }

    String* defaultLanguage() const
    {
        return m_defaultLanguage;
    }

    int currentUtterId()
    {
        return m_currentUtterId;
    }

    void setCurrentUtterId(int id)
    {
        m_currentUtterId = id;
    }

    void addCallbackId(unsigned int id)
    {
        m_callbackIds.insert(id);
    }

    void removeCallbackId(unsigned int id)
    {
        m_callbackIds.erase(id);
    }

    void clearCallbackIds()
    {
        m_callbackIds.clear();
    }

    void destroy();
    int prepare();
    void unprepare();
    int ttsPlay();
    int ttsState();
    int ttsStop();

    void speech(Element* element, String* text);
    void speech(SpeechSynthesisUtterance* utterance);
    void changeDefaultVoice(String* language, const int voiceType);
    bool isPaused();
    void pause();
    void resume();
    void cancel();

#if defined(STARFISH_TIZEN)
    tts_h& handle()
    {
        return m_handle;
    }

    Optional<Element*> lastSpeechElement()
    {
        return m_lastSpeechElement;
    }
#endif

private:
    void initialize();
    int createHandle();
    int speechElementText();
    int speechUtterances();

#if defined(STARFISH_TIZEN)
    tts_h m_handle;
    Optional<Element*> m_lastSpeechElement;
    friend void utteranceCompletedCB(tts_h handle, int utteranceId, void* data);
#endif
    bool m_isAccessibilityMode;
    LWE::TTSMode m_lweTTSMode;
    SpeechSynthesisUtterance* m_utterance;
    std::pair<Element*, String*> m_pendingSpeech;
    GCUnorderedMap<int, SpeechSynthesisUtterance*> m_utteranceList;
    GCUnorderedMap<String*, int> m_supportedVoiceList;
    String* m_defaultLanguage;
    std::string m_userLanguage;
    int m_defaultVoiceType;
    int m_currentUtterId;
    std::set<unsigned int> m_callbackIds;
};
} // namespace Starfish
#endif
#endif
