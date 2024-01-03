/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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
#include "LWEWebView.h"

#if defined(SHELL_ENABLE_BACKTRACE)
static void bt_sighandler(int sig, struct sigcontext ctx)
{
    void* trace[128];
    char** messages = (char**)NULL;
    int i, trace_size = 0;

    // `[STARFISH_TEST] Got signal` string is used by test case runner
    // don't change!
    if (sig == SIGSEGV) {
        printf(
            "[STARFISH_TEST] Got signal %d, pid %d, faulty address is %p, from "
            "%p\n",
            sig, (int)getpid(), (void*)ctx.cr2, (void*)ctx.rip);
    } else {
        printf("[STARFISH_TEST] Got signal %d, pid %d\n", sig, (int)getpid());
    }

    trace_size = backtrace(trace, 128);
    /* overwrite sigaction with caller's address */
    trace[1] = (void*)ctx.rip;
    messages = backtrace_symbols(trace, trace_size);
    /* skip first stack frame (points here) */
    printf("[bt] Execution path:\n");
    for (i = 1; i < trace_size; ++i) {
        printf("[bt] #%d %s ", i, messages[i]);

        char syscom[256];
        std::string temp = messages[i];
        auto moduleEnd = temp.find("(");
        auto addrStart = temp.find("+");
        auto addrEnd = temp.find(")");
        if (moduleEnd != std::string::npos && addrStart != std::string::npos &&
            addrEnd != std::string::npos) {
            std::string modulePath = temp.substr(0, moduleEnd);
            std::string addr = temp.substr(addrStart, addrEnd - addrStart);
            sprintf(syscom, "addr2line %s -e %s", addr.c_str(),
                    modulePath.c_str());
            system(syscom);
        } else {
            printf("\n");
        }
    }

    fflush(stdout);

    // this is the trick: it will trigger the core dump
    signal(sig, SIG_DFL);
    kill(getpid(), sig);
}
#endif

#if defined(SHELL_ENABLE_WINDOWLESS)
static volatile sig_atomic_t doneFlag = 0;

static void updateDoneFlagFromENV()
{
    if (getenv("SHELL_DONE_FLAG") && (atoi(getenv("SHELL_DONE_FLAG")) == 1)) {
        doneFlag = 1;
    }
}

static void setDoneFlag(int sig, siginfo_t* siginfo, void* context)
{
    doneFlag = 1;
}
#endif

static void handleShellCommand(const std::string input, LWE::WebView* webView)
{
    static const std::string prefix = "\\";

    std::string command =
        input.substr(0, input.find_last_not_of(" \t\n\v\f\r") + 1);
    if (command.find(prefix, 0) == 0) {
        command.erase(0, prefix.size());
        if (command == "reload") {
            webView->Reload();
        }
        return;
    }

    puts(webView->EvaluateJavaScript(input).c_str());
}

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

struct LWECreationOptions {
    int x = 0;
    int y = 0;
    float scaleFactor = 1;
    int width = 1920;
    int height = 1080;
};

struct LWEOptions {
    std::string customUserAgentString;
    bool enableSecurity = true;
    LWE::TTSMode ttsMode = LWE::TTSMode::Default;
    bool needsDownloadWebFontsEarly = false;
    uint32_t needsDownScaleImageResourceLargerThan = 0;
    bool scrollbarVisible = true;
    bool useExternalPopup = false;
    bool useSpatialNavigation = false;
    bool useHTTP2 = false;
    std::string language;

    struct EnvOptions {
        int flag = 0;
        bool pixelTest = false;
        bool referenceTestState = false;
        bool hideWindow = false;
        std::string screenShot;
        std::string screenShotWidth;
        std::string screenShotHeight;
        bool networkLogVerbose = false;
        bool starfishIgnoreSSLVerify = false;
        std::string glCompositorScale;
    } envOptions;
};

struct ShellOptions {
    bool crashTest = false;
    bool disableConsole = false;
    int timeout = 0;
};

static void parseArg(int argc, char* argv[],
                     LWECreationOptions& lweCreationOption,
                     LWEOptions& lweOptions, ShellOptions& shellOptions)
{
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--dump-computed-style") == 0) {
            lweOptions.envOptions.flag |=
                StarfishStartUpFlag::enableComputedStyleDump;
        } else if (strcmp(argv[i], "--dump-frame-tree") == 0) {
            lweOptions.envOptions.flag |=
                StarfishStartUpFlag::enableFrameTreeDump;
        } else if (strcmp(argv[i], "--dump-stacking-context") == 0) {
            lweOptions.envOptions.flag |=
                StarfishStartUpFlag::enableStackingContextDump;
        } else if (strcmp(argv[i], "--dump-hittest") == 0) {
            lweOptions.envOptions.flag |=
                StarfishStartUpFlag::enableHitTestDump;
        } else if (strcmp(argv[i], "--debug-graphics-layer") == 0) {
            lweOptions.envOptions.flag |=
                StarfishStartUpFlag::enableDebugGraphicsLayer;
        } else if (strcmp(argv[i], "--debug-repaint-region") == 0) {
            lweOptions.envOptions.flag |=
                StarfishStartUpFlag::enableDebugRepaintRegion;
        } else if (strcmp(argv[i], "--pixel-test") == 0) {
#ifdef SHELL_ENABLE_TEST
            lweOptions.envOptions.pixelTest = true;
#endif
        } else if (strcmp(argv[i], "--ref-test") == 0) {
#ifdef SHELL_ENABLE_TEST
            lweOptions.envOptions.referenceTestState = true;
#endif
        } else if (strstr(argv[i], "--width=") == argv[i]) {
            lweCreationOption.width = std::atoi(argv[i] + strlen("--width="));
        } else if (strstr(argv[i], "--height=") == argv[i]) {
            lweCreationOption.height = std::atoi(argv[i] + strlen("--height="));
        } else if (strcmp(argv[i], "--regression-test") == 0) {
            lweOptions.envOptions.flag |=
                StarfishStartUpFlag::enableRegressionTest;
        } else if (strstr(argv[i], "--screen-shot=") == argv[i]) {
            lweOptions.envOptions.screenShot =
                argv[i] + strlen("--screen-shot=");
        } else if (strstr(argv[i], "--screen-shot-width=") == argv[i]) {
            lweOptions.envOptions.screenShotWidth =
                (argv[i] + strlen("--screen-shot-width="));
        } else if (strstr(argv[i], "--screen-shot-height=") == argv[i]) {
            lweOptions.envOptions.screenShotHeight =
                argv[i] + strlen("--screen-shot-height=");
        } else if (strcmp(argv[i], "--hide-window") == 0) {
            // regression test, pixel test only
            lweOptions.envOptions.hideWindow = true;
            lweOptions.envOptions.flag |=
                StarfishStartUpFlag::enableRegressionTest;
        } else if (strcmp(argv[i], "--network-log-verbose") == 0) {
            lweOptions.envOptions.networkLogVerbose = true;
        } else if (strstr(argv[i], "--posX=") == argv[i]) {
            lweCreationOption.x = std::atoi(argv[i] + strlen("--posX="));
        } else if (strstr(argv[i], "--posY=") == argv[i]) {
            lweCreationOption.y = std::atoi(argv[i] + strlen("--posY="));
        } else if (strstr(argv[i], "--device-pixel-ratio=") == argv[i]) {
            lweCreationOption.scaleFactor =
                std::atof(argv[i] + strlen("--device-pixel-ratio="));
        } else if (strstr(argv[i], "--useragent=") == argv[i]) {
            lweOptions.customUserAgentString = argv[i] + strlen("--useragent=");
        } else if (strcmp(argv[i], "--disable-web-security") == 0) {
            lweOptions.enableSecurity = false;
        } else if (strcmp(argv[i], "--tts-forced") == 0) {
            lweOptions.ttsMode = LWE::TTSMode::Forced;
        } else if (strcmp(argv[i], "--crash-test") == 0) {
            shellOptions.crashTest = true;
        } else if (strstr(argv[i], "--needs-download-webfont-early") ==
                   argv[i]) {
            lweOptions.needsDownloadWebFontsEarly = true;
        } else if (strcmp(argv[i], "--disable-console") == 0) {
            shellOptions.disableConsole = true;
        } else if (strstr(argv[i],
                          "--needs-downscale-image-resource-larger-than=") ==
                   argv[i]) {
            lweOptions.needsDownScaleImageResourceLargerThan = std::atoi(
                argv[i] +
                strlen("--needs-downscale-image-resource-larger-than="));
        } else if (strstr(argv[i], "--scrollbar-unvisible")) {
            lweOptions.scrollbarVisible = false;
        } else if (strstr(argv[i], "--use-external-popup")) {
            lweOptions.useExternalPopup = true;
        } else if (strstr(argv[i], "--use-spatial-navigation")) {
            lweOptions.useSpatialNavigation = true;
        } else if (strcmp(argv[i], "--use-http2") == 0) {
            lweOptions.useHTTP2 = true;
        } else if (strstr(argv[i], "--tts-language=") == argv[i]) {
            lweOptions.language = argv[i] + strlen("--tts-language=");
        } else if (strstr(argv[i], "--timeout=") == argv[i]) {
            shellOptions.timeout = std::atoi(argv[i] + strlen("--timeout="));
        } else if (strstr(argv[i], "--ignore-ssl-verify")) {
            lweOptions.envOptions.starfishIgnoreSSLVerify = true;
        } else if (strstr(argv[i], "--gl-compositor-scale=") == argv[i]) {
            // this is secret feature for testing(working on gl + efl webview)
            lweOptions.envOptions.glCompositorScale =
                argv[i] + strlen("--gl-compositor-scale=");
        }
    }
}

static void applyLWEOptions(const LWEOptions& lweOptions, LWE::WebView* webView)
{
    auto settings = webView->GetSettings();
    if (lweOptions.customUserAgentString.length()) {
        settings.SetUserAgentString(lweOptions.customUserAgentString);
    }

    if (!lweOptions.enableSecurity) {
        settings.SetWebSecurityMode(LWE::WebSecurityMode::Disable);
    }

    if (lweOptions.needsDownloadWebFontsEarly) {
        settings.SetNeedsDownloadWebFontsEarly(true);
    }

    if (lweOptions.needsDownScaleImageResourceLargerThan) {
        settings.SetNeedsDownScaleImageResourceLargerThan(
            lweOptions.needsDownScaleImageResourceLargerThan);
    }
#ifndef TIZEN_COMPAT_HEADER_5_0
    if (!lweOptions.scrollbarVisible) {
        settings.SetScrollbarVisible(lweOptions.scrollbarVisible);
    }
#endif
    if (lweOptions.useExternalPopup) {
        settings.SetUseExternalPopup(lweOptions.useExternalPopup);
    }

    settings.SetUseSpatialNavigation(lweOptions.useSpatialNavigation);
    settings.SetTTSMode(lweOptions.ttsMode);
    settings.SetTTSLanguage(lweOptions.language);
    settings.SetUseHttp2(lweOptions.useHTTP2);
    webView->SetSettings(settings);
}

static void installBacktraceHandlerIfNeeds()
{
#if defined(SHELL_ENABLE_BACKTRACE)
    /* Install our signal handler */
    struct sigaction sa;

    sa.sa_handler = (void (*)(int))bt_sighandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
#endif
}

static void setupConfig()
{
#ifndef NDEBUG
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
#endif

#if !defined(SHELL_ANDROID) && !defined(SHELL_WINDOWS)
    // Changing these options can reducing {malloc, free} internal memory pool
    // usage
    // for big chunk ex) packets for MSE
    mallopt(M_MMAP_THRESHOLD, 2048);
    mallopt(M_MMAP_MAX, 1024 * 1024);
#endif

#if defined(SHELL_ENABLE_ELEMENTARY)
    elm_init(0, 0);
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

    const char* defaultEngine = "gl";
    const char* engine = getenv("STARFISH_ELM_ENGINE");
    if (!engine || strlen(engine) == 0) {
        engine = defaultEngine;
    }
    const char* defaultConfig = "opengl";
    const char* elmConfig = getenv("STARFISH_ELM_CONFIG");
    if (!elmConfig || strlen(elmConfig) == 0) {
        elmConfig = defaultConfig;
    }
    setenv("ELM_ENGINE", engine, 1);
#if defined(SHELL_ENABLE_ELEMENTARY_GL)
    elm_config_accel_preference_set(elmConfig);
#else
    elm_config_accel_preference_set(elmConfig);
#if !defined(SHELL_TIZEN)
    elm_config_preferred_engine_set("software_x11");
#endif
#endif
#elif defined(SHELL_ENABLE_ECORE)
    ecore_init();
#endif
}

static void printUsage()
{
    puts("please specify url");
}

static std::string cacheDir()
{
    std::string cacheDir = "/tmp";
    const char* homeDir = getenv("HOME");
    if (homeDir && strlen(homeDir)) {
        cacheDir = homeDir;
    }
    cacheDir += "/Starfish-cache";
    return cacheDir;
}

static void* createWindowIfNeeds(const LWECreationOptions& creationOption)
{
#if defined(SHELL_ENABLE_ELEMENTARY)
    Evas_Object* wndObj = nullptr;
    wndObj = elm_win_add(NULL, "Starfish", ELM_WIN_BASIC);
    elm_win_title_set(wndObj, "Starfish Shell");
    elm_win_autodel_set(wndObj, EINA_TRUE);
    evas_object_resize(wndObj, creationOption.width, creationOption.height);
    evas_object_move(wndObj, creationOption.x, creationOption.y);
    evas_object_show(wndObj);

#if defined(SHELL_TIZEN)
    int rots[4] = { 0, 90, 180, 270 };
    elm_win_wm_rotation_available_rotations_set(wndObj, (const int*)(&rots), 4);
#endif

#ifdef SHELL_TIZEN
#ifdef SHELL_ENABLE_TRANSPARENT_WINDOW
    // Set efl configuration for resizing window (Without this, Window'll be
    // full-screen only )
    elm_win_aux_hint_add(wndObj, "wm.policy.win.user.geometry", "1");

    elm_win_alpha_set(wndObj, EINA_TRUE);
    Evas_Object* bg = elm_bg_add(wndObj);
    evas_object_color_set(bg, 0x00, 0x00, 0x00, 0x00);

    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(wndObj, bg);
    evas_object_show(bg);
#else
    Evas_Object* bg = elm_bg_add(wndObj);
    // set background color to transparent for draw video correctly in tizen tv
    evas_object_color_set(bg, 0x00, 0x00, 0x00, 0x00);

    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(wndObj, bg);
    evas_object_show(bg);
#endif
#endif
    const char* path = getenv("SCREEN_SHOT");
    const char* hide = getenv("HIDE_WINDOW");
    if ((path && strlen(path)) || (hide && strlen(hide))) {
        evas_object_hide(wndObj);
    } else {
        evas_object_show(wndObj);
    }

    return wndObj;
#else
    return nullptr;
#endif
}

static void addMainBoxToWindowIfNeeds(void* wndObj, LWE::WebView* webView)
{
#if defined(SHELL_ENABLE_ELEMENTARY)
    Evas_Object* obj = static_cast<Evas_Object*>(webView->Unwrap());
    evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(static_cast<Evas_Object*>(wndObj), obj);
#endif
}

static void installConsoleThreadIfNeeds(LWE::WebView* webView,
                                        const ShellOptions& shellOptions)
{
    if (!shellOptions.disableConsole) {
#if defined(SHELL_ENABLE_ELEMENTARY) && (defined(SHELL_ENABLE_TEST))
        pthread_t t;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(
            &t, &attr,
            [](void* data) -> void* {
                char buf[1024];
                sleep(1);
                while (1) {
                    fgets(buf, 1024, stdin);
                    struct Pass {
                        LWE::WebView* webView;
                        char* buf;
                    };
                    char* b = new char[1024];
                    Pass* pass = new Pass;
                    pass->buf = b;
                    pass->webView = (LWE::WebView*)data;
                    memcpy(b, buf, sizeof buf);
                    ecore_thread_main_loop_begin();
                    ecore_animator_add(
                        [](void* data) -> Eina_Bool {
                            Pass* p = (Pass*)data;
                            handleShellCommand(p->buf, p->webView);
                            delete[] p->buf;
                            delete p;

                            return ECORE_CALLBACK_CANCEL;
                        },
                        pass);
                    ecore_thread_main_loop_end();
                }
                return NULL;
            },
            webView);
#elif defined(SHELL_ENABLE_UV) && defined(SHELL_ENABLE_TEST)
        struct Pass {
            LWE::WebView* webView;
            char* buf;
        };
        idlerThreadAsyncHandle = (uv_async_t*)malloc(sizeof(uv_async_t));
        uv_async_init(uv_default_loop(), idlerThreadAsyncHandle,
                      [](uv_async_t* handle) {
                          Pass* p = (Pass*)handle->data;
                          handleShellCommand(p->buf, p->webView);
                          delete[] p->buf;
                          delete p;
                      });
        pthread_t t;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(
            &t, &attr,
            [](void* data) -> void* {
                char buf[1024] = {
                    0,
                };
                sleep(1);
                while (1) {
                    auto ret = fgets(buf, 1024, stdin);
                    if (ret != nullptr) {
                        char* b = new char[1024];
                        Pass* pass = new Pass;
                        pass->buf = b;
                        pass->webView = (LWE::WebView*)data;
                        idlerThreadAsyncHandle->data = pass;
                        memcpy(b, buf, sizeof buf);

                        uv_async_send(idlerThreadAsyncHandle);
                    }
                }
                return NULL;
            },
            webView);
#endif
    }
}

static void cleanupConsoleThreadIfNeeds(const ShellOptions& shellOptions)
{
#if defined(SHELL_ENABLE_UV) && defined(SHELL_ENABLE_TEST)
    if (!shellOptions.disableConsole && idlerThreadAsyncHandle) {
        uv_close((uv_handle_t*)idlerThreadAsyncHandle,
                 [](uv_handle_t* handle) { free(handle); });
    }
#endif
}

static void installCrashTesThreadIfNeeds(LWE::WebView* webView,
                                         const ShellOptions& shellOptions)
{
    if (shellOptions.crashTest) {
        pthread_t t;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(
            &t, &attr,
            [](void* data) -> void* {
                sleep(5);
                puts("raise SIGINT for crash test");
                puts(
                    "if there is no crash until process exit, there "
                    "is no problem");
                raise(SIGINT);
                return NULL;
            },
            nullptr);
    }
}

static void installTimeoutThreadIfNeeds(LWE::WebView* webView,
                                        const ShellOptions& shellOptions)
{
#if defined(SHELL_ENABLE_WINDOWLESS)
    std::future<int> future;
    if (shellOptions.timeout > 0) {
        future = std::async(std::launch::async, [shellOptions]() {
            std::this_thread::sleep_for(
                std::chrono::seconds(shellOptions.timeout));
            return 1;
        });

        pthread_t t;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(
            &t, &attr,
            [](void* data) -> void* {
                std::future<int>* future = (std::future<int>*)(data);
                std::future_status status;
                do {
                    status = future->wait_for(std::chrono::seconds(1));
                    switch (status) {
                    case std::future_status::deferred:
                        puts("deferred");
                        break;
                    case std::future_status::timeout:
                        puts("timeout");
                        break;
                    case std::future_status::ready:
                        puts("ready!");
                        break;
                    default:
                        puts("default!");
                        break;
                    }
                } while (status != std::future_status::ready);
                doneFlag = 1;
                return NULL;
            },
            (void*)(&future));
    }
#endif
}

static void installFocusInHandlerIfNeeds(void* wndObj, LWE::WebView* webView)
{
#if defined(SHELL_ENABLE_ELEMENTARY)
    /// example code about diligently getting focus in EFL
    auto focusInHandler = [](void* data, Evas* e, Evas_Object* obj,
                             void* event_info) {
        LWE::WebView* wv = (LWE::WebView*)data;
        wv->Focus();
    };
    evas_object_event_callback_add(static_cast<Evas_Object*>(wndObj),
                                   EVAS_CALLBACK_FOCUS_IN, focusInHandler,
                                   webView);
#endif
}

static int runMainLoop()
{
#if defined(SHELL_ENABLE_ELEMENTARY)
    elm_run();
#elif defined(SHELL_ENABLE_HEADLESS)
    ecore_main_loop_begin();
#elif defined(SHELL_ENABLE_WINDOWLESS)
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_sigaction = setDoneFlag;
    act.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    while (!doneFlag) {
        usleep(100);
        updateDoneFlagFromENV();
    }
#endif
    return 0;
}

static void stopMainLoop()
{
#if defined(SHELL_ENABLE_ELEMENTARY)
    elm_shutdown();
#elif defined(SHELL_ENABLE_ECORE)
    ecore_shutdown();
#endif
}

static int getExitCode()
{
    int exitCode = 0;
#if defined(SHELL_ENABLE_TEST)
    if (getenv("EXIT_CODE")) {
        exitCode = std::atoi(getenv("EXIT_CODE"));
    }
#endif
    return exitCode;
}

static void applyEnvironmentVariable(const LWEOptions& lweOptions)
{
    if (lweOptions.envOptions.screenShot.length()) {
        setenv("SCREEN_SHOT", lweOptions.envOptions.screenShot.data(), 1);
        setenv("SCREEN_SHOT_FILE", lweOptions.envOptions.screenShot.c_str(), 1);
        setenv("EXIT_AFTER_SCREEN_SHOT", "1", 1);
    }

    if (lweOptions.envOptions.screenShotWidth.length()) {
        setenv("SCREEN_SHOT_WIDTH",
               lweOptions.envOptions.screenShotWidth.c_str(), 1);
    }

    if (lweOptions.envOptions.screenShotHeight.length()) {
        setenv("SCREEN_SHOT_HEIGHT",
               lweOptions.envOptions.screenShotHeight.c_str(), 1);
    }

    if (lweOptions.envOptions.hideWindow) {
        setenv("HIDE_WINDOW", "1", 1);
    }

    if (lweOptions.envOptions.networkLogVerbose) {
        setenv("NETWORK_LOG_VERBOSE", "1", 1);
    }

    if (lweOptions.envOptions.starfishIgnoreSSLVerify) {
        setenv("IGNORE_SSL_VERIFY", "1", 1);
    }

    if (lweOptions.envOptions.glCompositorScale.length()) {
        setenv("LWE_GL_COMPOSITOR_SCALE",
               lweOptions.envOptions.glCompositorScale.c_str(), 1);
    }

#ifdef SHELL_ENABLE_TEST
    if (lweOptions.envOptions.pixelTest) {
        setenv("PIXEL_TEST", "1", 1);
    }

    if (lweOptions.envOptions.referenceTestState) {
        setenv("REF_TEST_STATE", "1", 1);
        setenv("HIDE_WINDOW", "1", 1);
    }

    std::string startUpFlag = std::to_string(lweOptions.envOptions.flag);
    setenv("START_UP_FLAG", startUpFlag.c_str(), 1);
    setenv("SHELL_DONE_FLAG", "0", 1);
    setenv("EXIT_CODE", "0", 1);
#endif
}

int main(int argc, char* argv[])
{
    if (argc == 1) {
        printUsage();
        return -1;
    }

    installBacktraceHandlerIfNeeds();
    setupConfig();

    LWECreationOptions creationOption;
    LWEOptions lweOptions;
    ShellOptions shellOptions;
    parseArg(argc, argv, creationOption, lweOptions, shellOptions);

    applyEnvironmentVariable(lweOptions);

    LWE::LWE::Initialize("/tmp/Starfish_localStorage.txt",
                         "/tmp/Starfish_Cookies.txt", cacheDir().c_str());

    // TODO: remove test code.
    if (LWE::CookieManager::GetInstance()->HasCookies()) {
        // Test for cookie manager.
        printf("Cookie Manager has cookies.\n");
    }

    // Test for ResourceError.
    LWE::ResourceError error(1, "testcode", "testurl\n");
    printf("%s %s", error.GetDescription().c_str(), error.GetUrl().c_str());

    const char* gcFrequency = getenv("GC_FREQUENCY");
    if (gcFrequency && strlen(gcFrequency)) {
        LWE::LWE::SetGCFrequency(std::atoi(gcFrequency));
    }

    void* wndObj = createWindowIfNeeds(creationOption);

#if defined(SHELL_ENABLE_HEADLESS)
    LWE::WebContainer* webView = LWE::WebContainer::CreateHeadless(
        creationOption.width, creationOption.height, creationOption.scaleFactor,
        "serif", "ko-KR", "Asia/Seoul");
#else
    LWE::WebView* webView = LWE::WebView::Create(
        wndObj, creationOption.x, creationOption.y, creationOption.width,
        creationOption.height, creationOption.scaleFactor, "serif", "ko-KR",
        "Asia/Seoul");
    applyLWEOptions(lweOptions, webView);
#endif

#if !defined(SHELL_ENABLE_HEADLESS)
    addMainBoxToWindowIfNeeds(wndObj, webView);
#endif
    webView->LoadURL(std::string(argv[1]));
    webView->Focus();

#if !defined(SHELL_ENABLE_HEADLESS)
    installFocusInHandlerIfNeeds(wndObj, webView);

    installConsoleThreadIfNeeds(webView, shellOptions);
    installCrashTesThreadIfNeeds(webView, shellOptions);
    installTimeoutThreadIfNeeds(webView, shellOptions);
#endif

    int ret = runMainLoop();
    if (ret != 0) {
        return ret;
    }

    cleanupConsoleThreadIfNeeds(shellOptions);

    webView->Destroy();
    webView = nullptr;

    LWE::LWE::Finalize();

    stopMainLoop();

    return getExitCode();
}
