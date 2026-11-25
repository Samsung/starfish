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

#include <cstdio>
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

// True when `arg` is exactly the option `name`.
bool isFlag(const char* arg, const char* name)
{
    return strcmp(arg, name) == 0;
}

// For an option written as "--name=<value>" returns the value; nullptr when
// `arg` is some other option. `prefix` includes the trailing '='.
const char* valueOf(const char* arg, const char* prefix)
{
    size_t length = strlen(prefix);
    return strncmp(arg, prefix, length) == 0 ? arg + length : nullptr;
}
} // namespace

#if defined(SHELL_HOST_WEBVIEW)
// --headless runs a WebContainer instead of a windowed WebView; both expose
// the calls made through this.
#define LWE_CALL(call) (m_headlessLwe ? m_headlessLwe->call : m_lwe->call)
#else
#define LWE_CALL(call) (m_lwe->call)
#endif

namespace StarfishShell {

void MiniBrowser::parseArgs(int argc, char* argv[],
                            MiniBrowser::EnvironmentValues& env,
                            MiniBrowser::InitOption& init,
                            MiniBrowser::Settings& settings,
                            MiniBrowser::OtherOptions& others)
{
    // argv[1] is url.
    for (int i = 2; i < argc; i++) {
        const char* arg = argv[i];
        if (isFlag(arg, "--dump-computed-style")) {
            env.flag |= StarfishStartUpFlag::enableComputedStyleDump;
        } else if (isFlag(arg, "--dump-frame-tree")) {
            env.flag |= StarfishStartUpFlag::enableFrameTreeDump;
        } else if (isFlag(arg, "--dump-stacking-context")) {
            env.flag |= StarfishStartUpFlag::enableStackingContextDump;
        } else if (isFlag(arg, "--dump-hittest")) {
            env.flag |= StarfishStartUpFlag::enableHitTestDump;
        } else if (isFlag(arg, "--debug-graphics-layer")) {
            env.flag |= StarfishStartUpFlag::enableDebugGraphicsLayer;
        } else if (isFlag(arg, "--debug-repaint-region")) {
            env.flag |= StarfishStartUpFlag::enableDebugRepaintRegion;
        } else if (isFlag(arg, "--pixel-test")) {
#ifdef SHELL_ENABLE_TEST
            env.pixelTest = true;
#endif
        } else if (isFlag(arg, "--ref-test")) {
#ifdef SHELL_ENABLE_TEST
            env.referenceTestState = true;
#endif
        } else if (const char* value = valueOf(arg, "--width=")) {
            init.geometry.width = std::atoi(value);
        } else if (const char* value = valueOf(arg, "--height=")) {
            init.geometry.height = std::atoi(value);
        } else if (isFlag(arg, "--regression-test")) {
            env.flag |= StarfishStartUpFlag::enableRegressionTest;
        } else if (const char* value = valueOf(arg, "--screen-shot=")) {
            env.screenShot = value;
        } else if (const char* value = valueOf(arg, "--screen-shot-width=")) {
            env.screenShotWidth = value;
        } else if (const char* value = valueOf(arg, "--screen-shot-height=")) {
            env.screenShotHeight = value;
        } else if (isFlag(arg, "--hide-window")) {
            // Only hides the window. Regression mode (font substitution,
            // synchronous image loads) is opt-in via --regression-test so a
            // headless run can still exercise the page's own web fonts.
            env.hideWindow = true;
        } else if (isFlag(arg, "--network-log-verbose")) {
            env.networkLogVerbose = true;
        } else if (const char* value = valueOf(arg, "--posX=")) {
            init.geometry.x = std::atoi(value);
        } else if (const char* value = valueOf(arg, "--posY=")) {
            init.geometry.y = std::atoi(value);
        } else if (const char* value = valueOf(arg, "--device-pixel-ratio=")) {
            init.scaleFactor = std::atof(value);
        } else if (const char* value = valueOf(arg, "--useragent=")) {
            settings.customUserAgentString = value;
        } else if (isFlag(arg, "--disable-web-security")) {
            settings.enableSecurity = false;
        } else if (isFlag(arg, "--tts-forced")) {
            settings.ttsMode = LWE::TTSMode::Forced;
        } else if (isFlag(arg, "--crash-test")) {
            others.crashTest = true;
        } else if (isFlag(arg, "--needs-download-webfont-early")) {
            settings.needsDownloadWebFontsEarly = true;
        } else if (isFlag(arg, "--disable-console")) {
            others.disableConsole = true;
        } else if (const char* value = valueOf(
                       arg, "--needs-downscale-image-resource-larger-than=")) {
            settings.needsDownScaleImageResourceLargerThan = std::atoi(value);
        } else if (const char* value =
                       valueOf(arg, "--max-video-decode-size=")) {
            unsigned width = 0, height = 0;
            if (sscanf(value, "%ux%u", &width, &height) == 2) {
                settings.maxVideoDecodeWidth = width;
                settings.maxVideoDecodeHeight = height;
            }
        } else if (isFlag(arg, "--scrollbar-unvisible")) {
            settings.scrollbarVisible = false;
        } else if (isFlag(arg, "--use-external-popup")) {
            settings.useExternalPopup = true;
        } else if (isFlag(arg, "--use-spatial-navigation")) {
            settings.useSpatialNavigation = true;
        } else if (isFlag(arg, "--use-http2")) {
            settings.useHTTP2 = true;
        } else if (const char* value = valueOf(arg, "--tts-language=")) {
            settings.language = value;
        } else if (isFlag(arg, "--ignore-ssl-verify")) {
            env.starfishIgnoreSSLVerify = true;
        } else if (const char* value = valueOf(arg, "--gl-compositor-scale=")) {
            // this is secret feature for testing(working on gl + efl webview)
            env.glCompositorScale = value;
        } else if (isFlag(arg, "--show-fps")) {
            settings.showFps = true;
        } else if (isFlag(arg, "--enable-video-overlay")) {
            settings.videoOverlayEnabled = true;
        } else if (const char* value = valueOf(arg, "--timeout=")) {
            others.timeout = std::atof(value);
        } else if (const char* value = valueOf(arg, "--storage-dir=")) {
            others.storageDir = value;
        } else if (isFlag(arg, "--prefer-isolated-thread")) {
            others.preferSeparateThread = true;
        } else if (isFlag(arg, "--prefer-incremental-gc")) {
            others.preferIncrementalGC = true;
        } else if (isFlag(arg, "--headless")) {
#if defined(STARFISH_SHELL_EFL) || defined(STARFISH_SHELL_X11_WEBCONTAINER)
            // These shells need a display for their window and event loop
            // before the engine starts, so they cannot run headless.
            fprintf(stderr,
                    "--headless is not supported by this shell; "
                    "starting with a window\n");
#else
            init.headless = true;
#endif
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
    LWE_CALL(Blur());
    LWE_CALL(Destroy());

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
    LWE::Settings lweSettings = LWE_CALL(GetSettings());
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

    if (settings.maxVideoDecodeWidth && settings.maxVideoDecodeHeight) {
        lweSettings.SetMaxVideoDecodeSize(settings.maxVideoDecodeWidth,
                                          settings.maxVideoDecodeHeight);
    }

    if (!settings.scrollbarVisible) {
        lweSettings.SetScrollbarVisible(settings.scrollbarVisible);
    }

    if (settings.useExternalPopup) {
        lweSettings.SetUseExternalPopup(settings.useExternalPopup);
    }

    if (settings.showFps) {
        lweSettings.UpdateSetting("--show-fps", "True");
    }

    if (settings.videoOverlayEnabled) {
        lweSettings.UpdateSetting("videoOverlayEnabled", "True");
    }

    lweSettings.SetUseSpatialNavigation(settings.useSpatialNavigation);
    lweSettings.SetTTSMode(settings.ttsMode);
    lweSettings.SetTTSLanguage(settings.language);
    lweSettings.SetUseHttp2(settings.useHTTP2);

    LWE_CALL(SetSettings(lweSettings));
}

void MiniBrowser::loadURL(const std::string& url)
{
    LWE_CALL(LoadURL(url));
}

std::string MiniBrowser::evaluateJavaScript(const std::string& script)
{
    return LWE_CALL(EvaluateJavaScript(script));
}

void MiniBrowser::reload()
{
    LWE_CALL(Reload());
}

void MiniBrowser::focus()
{
    LWE_CALL(Focus());
}

void MiniBrowser::setRotate(int degree)
{
    m_window->setRotate(degree);
}

void MiniBrowser::setDevicePixelRatio(float dpr)
{
    LWE_CALL(SetDevicePixelRatio(dpr));
}

int MiniBrowser::runMainLoop()
{
    return m_window->appLoop()->start();
}

AppLoop* MiniBrowser::appLoop()
{
    return m_window->appLoop();
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
#if defined(SHELL_HOST_GL_WEBCONTAINER)
    if (!createGLWebContainer(initOption)) {
        return false;
    }
    connectWindowEvents();
#elif defined(SHELL_HOST_WEBVIEW)
    createWebView(initOption);
#elif defined(SHELL_HOST_HEADLESS_WEBCONTAINER)
    m_lwe = LWE::WebContainer::CreateHeadless(
        initOption.geometry.width, initOption.geometry.height,
        initOption.scaleFactor, "serif", "ko-KR", "Asia/Seoul");
#endif

#if defined(SHELL_CAN_REPLAY_API)
    startReplayIfRequested();
#endif

    return true;
}

#if defined(SHELL_HOST_GL_WEBCONTAINER)
bool MiniBrowser::createGLWebContainer(const InitOption& initOption)
{
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

    return true;
}

void MiniBrowser::connectWindowEvents()
{
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
}
#endif

#if defined(SHELL_HOST_WEBVIEW)
void MiniBrowser::createWebView(const InitOption& initOption)
{
    if (initOption.headless) {
        m_headlessLwe = LWE::WebContainer::CreateHeadless(
            initOption.geometry.width, initOption.geometry.height,
            initOption.scaleFactor, "serif", "ko-KR", "Asia/Seoul");
        return;
    }
    m_lwe = LWE::WebView::Create(
        m_window->getNativeWindowHandle(), initOption.geometry.x,
        initOption.geometry.y, initOption.geometry.width,
        initOption.geometry.height, initOption.scaleFactor, "serif", "ko-KR",
        "Asia/Seoul");
#if defined(STARFISH_SHELL_EFL)
    m_window->setFocusInHandler([this]() { m_lwe->Focus(); });
    m_window->addAutoFitChild(m_lwe->Unwrap());
#endif
}
#endif

#if defined(SHELL_CAN_REPLAY_API)
void MiniBrowser::startReplayIfRequested()
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

std::string MiniBrowser::storageDir()
{
    if (!m_storageDirOverride.empty()) {
        return m_storageDirOverride;
    }

    std::string cacheDir = "/tmp";
    const char* homeDir = getenv("HOME");
    if (homeDir && strlen(homeDir)) {
        cacheDir = homeDir;
    }
    cacheDir += "/Starfish-storage";
    return cacheDir;
}

} // namespace StarfishShell
