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
#include "binding/WebViewHoldable.h"

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
        , m_isPaused(false)
        , m_state(-1)
        , m_lweTTSMode(LWE::TTSMode::Default)
        , m_utterance(nullptr)
        , m_ttsText(String::emptyString)
        , m_currentUtterId(0)
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

    LWE::TTSMode mode() const
    {
        return m_lweTTSMode;
    }

    void setMode(LWE::TTSMode lweTTSMode);

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

    String* ttsText()
    {
        return m_ttsText;
    }

    void clearTTSText()
    {
        m_ttsText = String::emptyString;
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

    void destroy();
    int prepare();
    void unprepare();
    int ttsPlay();
    int ttsState();

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
#endif

private:
    void initialize();
    int createHandle();
    int speechElementText();
    int speechUtterances();

#if defined(STARFISH_TIZEN)
    tts_h m_handle;
#endif
    Element* m_element;
    bool m_isAccessibilityMode;
    bool m_isCreatedVoiceList;
    bool m_isPaused;
    int m_state;
    LWE::TTSMode m_lweTTSMode;
    SpeechSynthesisUtterance* m_utterance;
    String* m_ttsText;
    GCUnorderedMap<int, SpeechSynthesisUtterance*> m_utteranceList;
    GCUnorderedMap<String*, int> m_supportedVoiceList;
    String* m_defaultLanguage;
    int m_defaultVoiceType;
    int m_currentUtterId;
};
}
#endif
#endif
