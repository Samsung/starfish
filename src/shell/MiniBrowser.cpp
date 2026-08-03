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

#include "Console.h"
#if defined(STARFISH_ENABLE_TEST)
#include "APIReplayer.h"
#endif

#include <cstring>

namespace {
// Originally defined in core/page/WebView.h
enum StarfishStartUpFlag {
    enableComputedStyleDump = 1 << 1,
    enableFrameTreeDump = 1 << 2,
    enableStackingContextDump = 1 << 3,
    enableHitTestDump = 1 << 4,
    enableDebugGraphicsLayer = 1 << 5,
    enableDebugRepaintRegion = 1 << 6,
    enableRegressionTest = 1 << 7,
};
} // namespace

namespace StarfishShell {

void MiniBrowser::parseArgs(int argc, char* argv[],
                            MiniBrowser::EnvironmentValues& env,
                            MiniBrowser::InitOption& init,
                            MiniBrowser::Settings& settings,
                            MiniBrowser::OtherOptions& others)
{
    // argv[1] is url.
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--dump-computed-style") == 0) {
            env.flag |= StarfishStartUpFlag::enableComputedStyleDump;
        } else if (strcmp(argv[i], "--dump-frame-tree") == 0) {
            env.flag |= StarfishStartUpFlag::enableFrameTreeDump;
        } else if (strcmp(argv[i], "--dump-stacking-context") == 0) {
            env.flag |= StarfishStartUpFlag::enableStackingContextDump;
        } else if (strcmp(argv[i], "--dump-hittest") == 0) {
            env.flag |= StarfishStartUpFlag::enableHitTestDump;
        } else if (strcmp(argv[i], "--debug-graphics-layer") == 0) {
            env.flag |= StarfishStartUpFlag::enableDebugGraphicsLayer;
        } else if (strcmp(argv[i], "--debug-repaint-region") == 0) {
            env.flag |= StarfishStartUpFlag::enableDebugRepaintRegion;
        } else if (strcmp(argv[i], "--pixel-test") == 0) {
#ifdef SHELL_ENABLE_TEST
            env.pixelTest = true;
#endif
        } else if (strcmp(argv[i], "--ref-test") == 0) {
#ifdef SHELL_ENABLE_TEST
            env.referenceTestState = true;
#endif
        } else if (strstr(argv[i], "--width=") == argv[i]) {
            init.geometry.width = std::atoi(argv[i] + strlen("--width="));
        } else if (strstr(argv[i], "--height=") == argv[i]) {
            init.geometry.height = std::atoi(argv[i] + strlen("--height="));
        } else if (strcmp(argv[i], "--regression-test") == 0) {
            env.flag |= StarfishStartUpFlag::enableRegressionTest;
        } else if (strstr(argv[i], "--screen-shot=") == argv[i]) {
            env.screenShot = argv[i] + strlen("--screen-shot=");
        } else if (strstr(argv[i], "--screen-shot-width=") == argv[i]) {
            env.screenShotWidth = (argv[i] + strlen("--screen-shot-width="));
        } else if (strstr(argv[i], "--screen-shot-height=") == argv[i]) {
            env.screenShotHeight = argv[i] + strlen("--screen-shot-height=");
        } else if (strcmp(argv[i], "--hide-window") == 0) {
            // regression test, pixel test only
            env.hideWindow = true;
            env.flag |= StarfishStartUpFlag::enableRegressionTest;
        } else if (strcmp(argv[i], "--network-log-verbose") == 0) {
            env.networkLogVerbose = true;
        } else if (strstr(argv[i], "--posX=") == argv[i]) {
            init.geometry.x = std::atoi(argv[i] + strlen("--posX="));
        } else if (strstr(argv[i], "--posY=") == argv[i]) {
            init.geometry.y = std::atoi(argv[i] + strlen("--posY="));
        } else if (strstr(argv[i], "--device-pixel-ratio=") == argv[i]) {
            init.scaleFactor =
                std::atof(argv[i] + strlen("--device-pixel-ratio="));
        } else if (strstr(argv[i], "--useragent=") == argv[i]) {
            settings.customUserAgentString = argv[i] + strlen("--useragent=");
        } else if (strcmp(argv[i], "--disable-web-security") == 0) {
            settings.enableSecurity = false;
        } else if (strcmp(argv[i], "--tts-forced") == 0) {
            settings.ttsMode = LWE::TTSMode::Forced;
        } else if (strcmp(argv[i], "--crash-test") == 0) {
            others.crashTest = true;
        } else if (strstr(argv[i], "--needs-download-webfont-early") ==
                   argv[i]) {
            settings.needsDownloadWebFontsEarly = true;
        } else if (strcmp(argv[i], "--disable-console") == 0) {
            others.disableConsole = true;
        } else if (strstr(argv[i],
                          "--needs-downscale-image-resource-larger-than=") ==
                   argv[i]) {
            settings.needsDownScaleImageResourceLargerThan = std::atoi(
                argv[i] +
                strlen("--needs-downscale-image-resource-larger-than="));
        } else if (strstr(argv[i], "--scrollbar-unvisible")) {
            settings.scrollbarVisible = false;
        } else if (strstr(argv[i], "--use-external-popup")) {
            settings.useExternalPopup = true;
        } else if (strstr(argv[i], "--use-spatial-navigation")) {
            settings.useSpatialNavigation = true;
        } else if (strcmp(argv[i], "--use-http2") == 0) {
            settings.useHTTP2 = true;
        } else if (strstr(argv[i], "--tts-language=") == argv[i]) {
            settings.language = argv[i] + strlen("--tts-language=");
        } else if (strstr(argv[i], "--ignore-ssl-verify")) {
            env.starfishIgnoreSSLVerify = true;
        } else if (strstr(argv[i], "--gl-compositor-scale=") == argv[i]) {
            // this is secret feature for testing(working on gl + efl webview)
            env.glCompositorScale = argv[i] + strlen("--gl-compositor-scale=");
        } else if (strstr(argv[i], "--show-fps") == argv[i]) {
            settings.showFps = true;
        } else if (strcmp(argv[i], "--enable-video-overlay") == 0) {
            settings.videoOverlayEnabled = true;
        } else if (strstr(argv[i], "--timeout=") == argv[i]) {
            others.timeout = std::atoi(argv[i] + strlen("--timeout="));
        } else if (strcmp(argv[i], "--prefer-isolated-thread") == 0) {
            others.preferIsolatedThread = false;
        } else if (strcmp(argv[i], "--prefer-incremental-gc") == 0) {
            others.preferIncrementalGC = true;
        }
    }
}

void MiniBrowser::setEnvironmentValues(const EnvironmentValues& env)
{
    if (env.screenShot.length()) {
        setenv("SCREEN_SHOT", env.screenShot.data(), 1);
        setenv("SCREEN_SHOT_FILE", env.screenShot.c_str(), 1);
        setenv("EXIT_AFTER_SCREEN_SHOT", "1", 1);
    }

    if (env.screenShotWidth.length()) {
        setenv("SCREEN_SHOT_WIDTH", env.screenShotWidth.c_str(), 1);
    }

    if (env.screenShotHeight.length()) {
        setenv("SCREEN_SHOT_HEIGHT", env.screenShotHeight.c_str(), 1);
    }

    if (env.hideWindow) {
        setenv("HIDE_WINDOW", "1", 1);
    }

    if (env.networkLogVerbose) {
        setenv("NETWORK_LOG_VERBOSE", "1", 1);
    }

    if (env.starfishIgnoreSSLVerify) {
        setenv("IGNORE_SSL_VERIFY", "1", 1);
    }

    if (env.glCompositorScale.length()) {
        setenv("LWE_GL_COMPOSITOR_SCALE", env.glCompositorScale.c_str(), 1);
    }

    if (env.pixelTest) {
        setenv("PIXEL_TEST", "1", 1);
    }

    if (env.referenceTestState) {
        setenv("REF_TEST_STATE", "1", 1);
        setenv("HIDE_WINDOW", "1", 1);
    }

    std::string startUpFlag = std::to_string(env.flag);
    setenv("START_UP_FLAG", startUpFlag.c_str(), 1);
    setenv("SHELL_DONE_FLAG", "0", 1);
    setenv("EXIT_CODE", "0", 1);
}

MiniBrowser::MiniBrowser()
{
}

MiniBrowser::~MiniBrowser()
{
    if (m_console) {
        delete m_console;
    }
    m_lwe->Blur();
    m_lwe->Destroy();

    m_window->terminate();
    delete m_window;

    LWE::LWE::Finalize();
}

bool MiniBrowser::init(const InitOption& initOption,
                       LWE::InitializeOption initializeOption)
{
    // on EFL, createWindow must be called first
    // since createWindow does elm_init();
    if (!createWindow(initOption)) {
        return false;
    }

    LWE::LWE::SetVersionPreference(true);
    LWE::LWE::Initialize(storageDir().c_str(), initializeOption);

    int major, minor, patch;
    LWE::LWE::GetVersion(&major, &minor, &patch);
    printf("LWE Version: %d.%d.%d\n", major, minor, patch);

    const char* gcFrequency = getenv("GC_FREQUENCY");
    if (gcFrequency && strlen(gcFrequency)) {
        LWE::LWE::SetGCFrequency(std::atoi(gcFrequency));
    }

    if (!createLWE(initOption)) {
        return false;
    }

    return true;
}

void MiniBrowser::setSettings(const Settings& settings)
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

    if (!settings.scrollbarVisible) {
        lweSettings.SetScrollbarVisible(settings.scrollbarVisible);
    }

    if (settings.useExternalPopup) {
        lweSettings.SetUseExternalPopup(settings.useExternalPopup);
    }

    if (settings.showFps) {
        lweSettings.UpdateSetting("--show-fps", "true");
    }

    if (settings.videoOverlayEnabled) {
        lweSettings.UpdateSetting("videoOverlayEnabled", "True");
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

void MiniBrowser::setDevicePixelRatio(float dpr)
{
    m_lwe->SetDevicePixelRatio(dpr);
}

int MiniBrowser::runMainLoop()
{
    return m_window->appLoop()->start();
}

int MiniBrowser::runMainLoopWithTimeout(double timeoutInSec)
{
    return m_window->appLoop()->start(timeoutInSec);
}

void MiniBrowser::runConsole()
{
    m_console = Console::create(this);
    m_console->run();
}

bool MiniBrowser::createWindow(const InitOption& initOption)
{
    m_window = Window::create();
#if defined(STARFISH_ENABLE_TEST)
    if (getenv("SCREEN_SHOT") || getenv("HIDE_WINDOW")) {
        m_window->setInitHint(HINT_VISIBLE, 0);
    }
#endif

    if (!m_window->init("Starfish", initOption.geometry.width,
                        initOption.geometry.height)) {
        return false;
    }

    return true;
}

bool MiniBrowser::createLWE(const InitOption& initOption)
{
#if defined(STARFISH_SHELL_X11_WEBCONTAINER)
    LWE::WebContainer::WebContainerArguments args{
        .width = initOption.geometry.width,
        .height = initOption.geometry.height,
        .devicePixelRatio = initOption.scaleFactor,
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

    m_window->setExitEventHandler([this]() {
        printf("Exit\n");
        setenv("SHELL_DONE_FLAG", "1", 1);
        m_window->appLoop()->stop();
    });

    m_lwe->RegisterOnShowSoftwareKeyboardIfPossibleHandler(
        [this](LWE::WebContainer*) {
            m_window->ShowSoftwareKeyboardIfPossible();
        });

    m_lwe->RegisterOnHideSoftwareKeyboardIfPossibleHandler(
        [this](LWE::WebContainer*) {
            m_window->HideSoftwareKeyboardIfPossible();
        });

    m_window->setCompositionEventHandler([this](const char* text, bool isEnd) {
        if (isEnd) {
            m_lwe->DispatchCompositionEndEvent(text);
        } else {
            m_lwe->DispatchCompositionUpdateEvent(text);
        }
    });

#elif defined(STARFISH_SHELL_EFL) || defined(STARFISH_SHELL_X11) ||         \
    defined(STARFISH_SHELL_ECORE_X) || defined(STARFISH_SHELL_ECORE_WL2) || \
    defined(STARFISH_SHELL_TCORE_WL)
    m_lwe = LWE::WebView::Create(
        m_window->getNativeWindowHandle(), initOption.geometry.x,
        initOption.geometry.y, initOption.geometry.width,
        initOption.geometry.height, initOption.scaleFactor, "serif", "ko-KR",
        "Asia/Seoul");
#if defined(STARFISH_SHELL_EFL)
    m_window->setFocusInHandler([this]() { m_lwe->Focus(); });
    m_window->addAutoFitChild(m_lwe->Unwrap());
#endif
#elif defined(STARFISH_SHELL_EFL_HEADLESS) || \
    defined(STARFISH_SHELL_TCORE_HEADLESS) || \
    defined(STARFISH_SHELL_GLIB_HEADLESS)
    m_lwe = LWE::WebContainer::CreateHeadless(
        initOption.geometry.width, initOption.geometry.height,
        initOption.scaleFactor, "serif", "ko-KR", "Asia/Seoul");
#endif

#if defined(STARFISH_ENABLE_TEST) &&             \
    (defined(STARFISH_SHELL_X11_WEBCONTAINER) || \
     defined(STARFISH_SHELL_EFL_HEADLESS) ||     \
     defined(STARFISH_SHELL_TCORE_HEADLESS) ||   \
     defined(STARFISH_SHELL_GLIB_HEADLESS))
    {
        const char* replayPath = getenv("STARFISH_API_REPLAY");
        if (replayPath && replayPath[0] != '\0') {
            float speedFactor = 1.0f;
            const char* speedStr = getenv("STARFISH_API_REPLAY_SPEED");
            if (speedStr && speedStr[0] != '\0')
                speedFactor = std::atof(speedStr);

            struct ReplayStartCtx {
                LWE::WebContainer* wc;
                std::string path;
                float speed;
            };
            auto* ctx = new ReplayStartCtx{ m_lwe, replayPath, speedFactor };
            m_lwe->AddTimeout(
                [](void* data) {
                    auto* ctx = static_cast<ReplayStartCtx*>(data);
                    StarfishShell::APIReplayer replayer;
                    if (replayer.load(ctx->path.c_str()))
                        replayer.startReplay(ctx->wc, ctx->speed);
                    delete ctx;
                },
                ctx, 100);
        }
    }
#endif

    return true;
}

std::string MiniBrowser::storageDir()
{
    std::string cacheDir = "/tmp";
    const char* homeDir = getenv("HOME");
    if (homeDir && strlen(homeDir)) {
        cacheDir = homeDir;
    }
    cacheDir += "/Starfish-storage";
    return cacheDir;
}

} // namespace StarfishShell
