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

#if defined(STARFISH_X11_CAIRO_GL) || defined(STARFISH_GLFW_CAIRO_GL) || \
    defined(STARFISH_EFL_HEADLESS)
typedef LWE::WebContainer* LWEType;
#elif defined(STARFISH_EFL_CAIRO) || defined(STARFISH_EFL_CAIRO_GL)
typedef LWE::WebView* LWEType;
#endif

struct MiniBrowserGeometry {
    MiniBrowserGeometry() = default;
    MiniBrowserGeometry(int32_t x, int32_t y, int32_t width, int32_t height)
        : x(x)
        , y(y)
        , width(width)
        , height(height)
    {
    }

    int32_t x = 0, y = 0, width = 1920, height = 1080;
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
