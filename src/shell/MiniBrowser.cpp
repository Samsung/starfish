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

#include "ShellConfig.h"

#include "MiniBrowser.h"
#include "Window.h"
#include <cstring>

namespace StarfishShell {

#if defined(STARFISH_SHELL_GLFW) || defined(STARFISH_SHELL_X11)

class EventPoller {
public:
    void start(Window* window, LWE::WebContainer* webContainer)
    {
        m_lwe = webContainer;
        m_window = window;

        m_timeout = m_lwe->AddTimeout(onTimeout, this, 10);
    }

    void stop()
    {
        if (m_timeout) {
            m_lwe->ClearTimeout(m_timeout);
            m_timeout = 0;
        }
    }

private:
    static void onTimeout(void* data)
    {
        EventPoller* self = reinterpret_cast<EventPoller*>(data);
        self->m_window->pollEvent();
        self->m_timeout = self->m_lwe->AddTimeout(onTimeout, data, 10);
    }

    LWE::WebContainer* m_lwe = nullptr;
    Window* m_window = nullptr;
    size_t m_timeout = 0;

} g_eventPoller;
#endif

MiniBrowser::MiniBrowser()
{
}

MiniBrowser::~MiniBrowser()
{
    m_lwe->Blur();
#if defined(STARFISH_SHELL_GLFW) || defined(STARFISH_SHELL_X11)
    g_eventPoller.stop();
#endif
    m_lwe->Destroy();

    m_window->terminate();
    delete m_window;

    LWE::LWE::Finalize();
}

bool MiniBrowser::init(const MiniBrowserInitOption& initOption)
{
    m_initOption = initOption;

    // on EFL, createWindow must be called first
    // since createWindow does elm_init();
    if (!createWindow()) {
        return false;
    }

    LWE::LWE::Initialize("/tmp/Starfish_localStorage.txt",
                         "/tmp/Starfish_Cookies.txt", cacheDir().c_str());

    const char* gcFrequency = getenv("GC_FREQUENCY");
    if (gcFrequency && strlen(gcFrequency)) {
        LWE::LWE::SetGCFrequency(std::atoi(gcFrequency));
    }

    if (!createLWE()) {
        return false;
    }

    return true;
}

void MiniBrowser::setSettings(const MiniBrowserSettings& settings)
{
    LWE::Settings lweSettings = m_lwe->GetSettings();
    if (settings.customUserAgentString.length()) {
        lweSettings.SetUserAgentString(settings.customUserAgentString);
    }

    if (!settings.enableSecurity) {
        lweSettings.SetWebSecurityMode(LWE::WebSecurityMode::Disable);
    }

    if (settings.needsDownloadWebFontsEarly) {
        lweSettings.SetNeedsDownloadWebFontsEarly(true);
    }

    if (settings.needsDownScaleImageResourceLargerThan) {
        lweSettings.SetNeedsDownScaleImageResourceLargerThan(
            settings.needsDownScaleImageResourceLargerThan);
    }
#ifndef TIZEN_COMPAT_HEADER_5_0
    if (!settings.scrollbarVisible) {
        lweSettings.SetScrollbarVisible(settings.scrollbarVisible);
    }
#endif
    if (settings.useExternalPopup) {
        lweSettings.SetUseExternalPopup(settings.useExternalPopup);
    }

    if (settings.showFps) {
        lweSettings.UpdateSetting("--show-fps", "true");
    }

    lweSettings.SetUseSpatialNavigation(settings.useSpatialNavigation);
    lweSettings.SetTTSMode(settings.ttsMode);
    lweSettings.SetTTSLanguage(settings.language);
    lweSettings.SetUseHttp2(settings.useHTTP2);

    m_lwe->SetSettings(lweSettings);
}

void MiniBrowser::loadURL(const std::string& url)
{
    m_lwe->LoadURL(url);
}

std::string MiniBrowser::evaluateJavaScript(const std::string& script)
{
    return m_lwe->EvaluateJavaScript(script);
}

void MiniBrowser::reload()
{
    m_lwe->Reload();
}

void MiniBrowser::focus()
{
    m_lwe->Focus();
}

void MiniBrowser::setRotate(int degree)
{
    m_window->setRotate(degree);
}

bool MiniBrowser::createWindow()
{
    m_window = Window::create();
#if defined(STARFISH_ENABLE_TEST)
    if (getenv("SCREEN_SHOT") || getenv("HIDE_WINDOW")) {
        m_window->setInitHint(HINT_VISIBLE, 0);
    }
#endif

    if (!m_window->init("Starfish", m_initOption.geometry.width,
                        m_initOption.geometry.height)) {
        return false;
    }

    return true;
}

bool MiniBrowser::createLWE()
{
#if defined(STARFISH_SHELL_GLFW) || defined(STARFISH_SHELL_X11)
    LWE::WebContainer::WebContainerArguments args{
        .width = m_initOption.geometry.width,
        .height = m_initOption.geometry.height,
        .devicePixelRatio = m_initOption.scaleFactor,
        .defaultFontName = "serif",
        .locale = "ko-KR",
        .timezoneID = "Asia/Seoul",
    };
    LWE::WebContainer::RendererGLConfiguration config;
    config.onMakeCurrent = [this](LWE::WebContainer* wc) {
        m_window->renderer()->makeCurrent();
    };
    config.onSwapBuffers = [this](LWE::WebContainer* wc, bool mayNeedsSync) {
        m_window->renderer()->swapBuffers();
    };
    config.onCreateSharedContext = [this](LWE::WebContainer* wc) -> uintptr_t {
        return m_window->renderer()->createSharedContext();
    };
    config.onDestroyContext = [this](LWE::WebContainer* wc,
                                     uintptr_t context) -> bool {
        return m_window->renderer()->destroyContext(context);
    };
    config.onClearCurrentContext = [this](LWE::WebContainer* wc) -> bool {
        return m_window->renderer()->clearCurrentContext();
    };
    config.onMakeCurrentWithContext = [this](LWE::WebContainer* wc,
                                             uintptr_t context) -> bool {
        return m_window->renderer()->makeCurrentWithContext(context);
    };
    config.onGetProcAddress = [this](LWE::WebContainer* wc,
                                     const char* name) -> void* {
        return m_window->renderer()->getProcAddress(name);
    };
    config.onIsSupportedExtension = [this](LWE::WebContainer* wc,
                                           const char* extension) -> bool {
        return m_window->renderer()->isSupportedExtension(extension);
    };

    m_lwe = LWE::WebContainer::CreateGL(args, config);

    if (!m_lwe) {
        return false;
    }

    m_window->setWindowSizeEventHandler(
        [this](int width, int height) { m_lwe->ResizeTo(width, height); });

    m_window->setMotionEventHandler([this](int xpos, int ypos) {
        LWE::MouseButtonsValue buttons =
            m_isMouseLbuttonDown ? LWE::MouseButtonsValue::LeftButtonDown
                                 : LWE::MouseButtonsValue::NoButtonDown;
        m_lwe->DispatchMouseMoveEvent(LWE::MouseButtonValue::NoButton, buttons,
                                      xpos, ypos);
    });

    m_window->setButtonEventHandler([this](INPUT type, INPUT action) {
        if (type == INPUT::MOUSE_LBUTTON) {
            double xpos, ypos;
            m_window->getCursorPos(xpos, ypos);
            if (action == INPUT::PRESS) {
                m_isMouseLbuttonDown = true;
                m_lwe->DispatchMouseDownEvent(
                    LWE::MouseButtonValue::NoButton,
                    LWE::MouseButtonsValue::LeftButtonDown, xpos, ypos);
            } else {
                m_isMouseLbuttonDown = false;
                m_lwe->DispatchMouseUpEvent(
                    LWE::MouseButtonValue::NoButton,
                    LWE::MouseButtonsValue::NoButtonDown, xpos, ypos);
            }
        }
    });

    m_window->setScrollEventHandler([this](double x, double y, int delta) {
        m_lwe->DispatchMouseWheelEvent(x, y, delta);
    });

    m_window->setKeyEventHandler(
        [this](unsigned long code, INPUT action, unsigned mods) {
            LWE::KeyValue keyValue = Window::convertKeyCode(code, action, mods);
            if (action == INPUT::PRESS) {
                m_lwe->DispatchKeyDownEvent(keyValue);
                m_lwe->DispatchKeyPressEvent(keyValue);
            } else {
                m_lwe->DispatchKeyUpEvent(keyValue);
            }
        });

    m_window->setExitEventHandler([]() {
        printf("Exit\n");
        setenv("SHELL_DONE_FLAG", "1", 1);
    });

    g_eventPoller.start(m_window, m_lwe);
#elif defined(STARFISH_SHELL_EFL)
    m_lwe = LWE::WebView::Create(
        m_window->getNativeWindowHandle(), m_initOption.geometry.x,
        m_initOption.geometry.y, m_initOption.geometry.width,
        m_initOption.geometry.height, m_initOption.scaleFactor, "serif",
        "ko-KR", "Asia/Seoul");
    m_window->setFocusInHandler([this]() { m_lwe->Focus(); });
    m_window->addAutoFitChild(m_lwe->Unwrap());
#elif defined(STARFISH_SHELL_EFL_HEADLESS)
    m_lwe = LWE::WebContainer::CreateHeadless(
        m_initOption.geometry.width, m_initOption.geometry.height,
        m_initOption.scaleFactor, "serif", "ko-KR", "Asia/Seoul");
#endif
    return true;
}

std::string MiniBrowser::cacheDir()
{
    std::string cacheDir = "/tmp";
    const char* homeDir = getenv("HOME");
    if (homeDir && strlen(homeDir)) {
        cacheDir = homeDir;
    }
    cacheDir += "/Starfish-cache";
    return cacheDir;
}

} // namespace StarfishShell
