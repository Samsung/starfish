/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishShellMiniBrowser__
#define __StarfishShellMiniBrowser__

#include <string>
#include <cstdint>

#include "LWEWebView.h"

namespace LWE {
class WebContainer;
class WebView;
} // namespace LWE

namespace StarfishShell {

class Window;

#if defined(STARFISH_SHELL_GLFW) || defined(STARFISH_SHELL_X11) || \
    defined(STARFISH_SHELL_EFL_HEADLESS)
typedef LWE::WebContainer* LWEType;
#elif defined(STARFISH_SHELL_EFL)
typedef LWE::WebView* LWEType;
#endif

struct MiniBrowserGeometry {
    MiniBrowserGeometry() = default;
    MiniBrowserGeometry(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
        : x(x)
        , y(y)
        , width(width)
        , height(height)
    {
    }

    uint32_t x = 0, y = 0, width = 0, height = 0;
};

struct MiniBrowserInitOption {
    MiniBrowserGeometry geometry;
    float scaleFactor = 1;
};

struct MiniBrowserSettings {
    bool enableSecurity = true;
    bool needsDownloadWebFontsEarly = false;
    bool scrollbarVisible = true;
    bool useExternalPopup = false;
    bool useSpatialNavigation = false;
    bool useHTTP2 = false;
    uint32_t needsDownScaleImageResourceLargerThan = 0;
    LWE::TTSMode ttsMode = LWE::TTSMode::Default;
    std::string customUserAgentString;
    std::string language;
};

class MiniBrowser {
public:
    MiniBrowser();
    ~MiniBrowser();

    bool init(const MiniBrowserInitOption& initOption);
    void setSettings(const MiniBrowserSettings& settings);

    void loadURL(const std::string& url);

    std::string evaluateJavaScript(const std::string& script);

    void reload();

    void focus();

    void setRotate(int degree);

private:
    bool createWindow();

    bool createLWE();

    std::string cacheDir();

    MiniBrowserInitOption m_initOption;
    MiniBrowserSettings m_settings;

    bool m_isMouseLbuttonDown = false;

    Window* m_window = nullptr;
    LWEType m_lwe = nullptr;
};
} // namespace StarfishShell

#endif
