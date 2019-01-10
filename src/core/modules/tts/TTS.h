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
        , m_isTTSEnabled(false)
        , m_state(-1)
        , m_mode(LWE::TTSMode::Default)
    {
        initialize();
    }

    ~TTS()
    {
    }

    bool isTTSEnabled()
    {
        return m_isTTSEnabled;
    }
    void setTTSEnabled(bool value)
    {
        m_isTTSEnabled = value;
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

    void destroy();
    void speech(Element* element, String* text);
    const char* state(int s);

private:
    void initialize();

    bool createHandle();
    bool startPlay(const char* text);
    bool stopPlay();
    bool addText(const char* text);

#if defined(STARFISH_TIZEN)
    tts_h m_handle;
#endif
    Element* m_element;
    bool m_isTTSEnabled;
    int m_state;
    LWE::TTSMode m_mode;
};
}
#endif
#endif
