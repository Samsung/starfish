/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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
#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/page/BrowsingContext.h"
#include "core/modules/tts/SpeechSynthesis.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"

#include "core/modules/tts/TextAlternativeHelper.h"
#include "core/modules/tts/TTS.h"

namespace Starfish {

const char* voiceTypeToString(int type)
{
#ifdef STARFISH_TIZEN
    switch (type) {
    case TTS_VOICE_TYPE_MALE:
        return "Male";
    case TTS_VOICE_TYPE_FEMALE:
        return "Female";
    case TTS_VOICE_TYPE_CHILD:
        return "Child";
    }
#endif
    return "Auto";
}

DEFINE_EVENT_LISTENER(SpeechSynthesisUtterance, start);
DEFINE_EVENT_LISTENER(SpeechSynthesisUtterance, end);
DEFINE_EVENT_LISTENER(SpeechSynthesisUtterance, error);
DEFINE_EVENT_LISTENER(SpeechSynthesisUtterance, pause);
DEFINE_EVENT_LISTENER(SpeechSynthesisUtterance, resume);
// DEFINE_EVENT_LISTENER(SpeechSynthesisUtterance, mark);
// DEFINE_EVENT_LISTENER(SpeechSynthesisUtterance, boundary);

DEFINE_EVENT_LISTENER(SpeechSynthesis, voiceschanged);

void SpeechSynthesis::speak(SpeechSynthesisUtterance* u)
{
    webView()->tts()->speak(u);
}

void SpeechSynthesis::cancel()
{
    webView()->tts()->cancel();
}

void SpeechSynthesis::pause()
{
    webView()->tts()->pause();
}

void SpeechSynthesis::resume()
{
    webView()->tts()->resume();
}

bool SpeechSynthesis::paused()
{
    return webView()->tts()->isPaused();
}

bool SpeechSynthesis::speaking()
{
    return (webView()->tts()->utteranceList().size() >= 1);
}

bool SpeechSynthesis::pending()
{
    return (webView()->tts()->utteranceList().size() > 1);
}

GCVector<SpeechSynthesisVoice*>& SpeechSynthesis::getVoices()
{
    auto list = webView()->tts()->supportedVoiceList();
    size_t size = list.size();
    if (!m_isCreatedVoiceList && size > 0) {
        String* defaultLang = webView()->tts()->defaultLanguage();
        auto iter = list.begin();
        while (iter != list.end()) {
            String* lang = iter->first;
            String* voice = String::fromUTF8(voiceTypeToString(iter->second));

            SpeechSynthesisVoice* speechVoice = new SpeechSynthesisVoice(
                window(), voice, voice, lang, true, defaultLang->equals(lang));
            m_voiceList.push_back(speechVoice);
            iter++;
        }

        m_isCreatedVoiceList = true;
    }

    return m_voiceList;
}
}
#endif // STARFISH_ENABLE_TTS
