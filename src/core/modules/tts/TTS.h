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

#ifdef STARFISH_ENABLE_TTS
#ifndef __StarfishTTS__
#define __StarfishTTS__

#include "PlatformIntegrationData.h"
#include "binding/StarfishHoldable.h"

#if defined(STARFISH_TIZEN)
#include <tts.h>
#endif

namespace Starfish {

class Element;
class TTS : public gc, public WebViewHoldable {
public:
    TTS(WebView* webView)
        : WebViewHoldable(webView)
        , m_element(nullptr)
        , m_isAccessibilityMode(false)
        , m_isCreatedVoiceList(false)
        , m_state(-1)
        , m_mode(LWE::TTSMode::Default)
        , m_utterance(nullptr)
        , m_readyState(false)
        , m_waitingState(false)
    {
        initialize();
    }

    ~TTS()
    {
    }

    bool isAccessibilityMode()
    {
        return m_isAccessibilityMode;
    }
    void setAccessibilityMode(bool value)
    {
        m_isAccessibilityMode = value;
    }

    void setMode(LWE::TTSMode mode)
    {
        m_mode = mode;
    }

    LWE::TTSMode mode() const
    {
        return m_mode;
    }

    Element* element()
    {
        return m_element;
    }

    SpeechSynthesisUtterance* utterance()
    {
        return m_utterance;
    }

    void setUtterance(SpeechSynthesisUtterance* u)
    {
        m_utterance = u;
    }

    GCUnorderedMap<uint32_t, SpeechSynthesisUtterance*>& utteranceList()
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

    void destroy();
    void speech(Element* element, String* text);
    void speak(SpeechSynthesisUtterance* utterance);
    void speakStoredUtterance();
    void pause();
    void resume();
    void cancel();
    // const char* state(tts_state_e s);

    bool addText(const char* text);
    void changeDefaultVoice(String* language, const int voiceType);
    void readyState();

private:
    void initialize();

    bool createHandle();
    bool startPlay(const char* text);
    bool stopPlay();

#if defined(STARFISH_TIZEN)
    tts_h m_handle;
#endif
    Element* m_element;
    bool m_isAccessibilityMode;
    bool m_isCreatedVoiceList;
    int m_state;
    LWE::TTSMode m_mode;
    SpeechSynthesisUtterance* m_utterance;
    GCUnorderedMap<uint32_t, SpeechSynthesisUtterance*> m_utteranceList;
    GCUnorderedMap<String*, int> m_supportedVoiceList;
    String* m_defaultLanguage;
    int m_defaultVoiceType;
    bool m_readyState;
    bool m_waitingState;
};
}
#endif
#endif
