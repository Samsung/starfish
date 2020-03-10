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

#if defined(STARFISH_ENABLE_TTS) && !defined(STARFISH_TIZEN_PROD_TV)
#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Element.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/tts/TTS.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/WebView.h"

namespace Starfish {

void TTS::initialize()
{
#ifdef STARFISH_ENABLE_TEST
    m_isAccessibilityMode = true;
    if (m_isAccessibilityMode) {
        String* text = String::createASCIIString("Hi, I am Starfish");
        speech(nullptr, text);
    }
#endif
}

int TTS::createHandle()
{
    return 0;
}

void TTS::destroy()
{
}

void TTS::setMode(LWE::TTSMode lweTTSMode)
{
}

int TTS::prepare()
{
    return 0;
}

void TTS::unprepare()
{
}

void TTS::speech(Element* element, String* text)
{
    if (text && text != String::emptyString) {
        webView()->messageLoop()->addIdler(
            nullptr,
            [](size_t, void* data) {
                String* text = (String*)data;
                STARFISH_LOG_INFO("TTS speech : %s\n",
                                  text->toUTF8NonGCString().data());
            },
            text);
    }
}

int TTS::speechElementText()
{
    return 0;
}

void TTS::speech(SpeechSynthesisUtterance* utterance)
{
}

int TTS::speechUtterances()
{
    return 0;
}

void TTS::pause()
{
}

bool TTS::isPaused()
{
    return false;
}

void TTS::resume()
{
}

void TTS::cancel()
{
}

void TTS::changeDefaultVoice(String* language, const int voiceType)
{
}

int TTS::ttsState()
{
    return 0;
}

int TTS::ttsPlay()
{
    return 0;
}
}

#endif
