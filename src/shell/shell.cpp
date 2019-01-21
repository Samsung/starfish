/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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
#include "StarfishConfig.h"
#include "LWEWebView.h"

#include "Starfish.h"
#include "core/page/WebView.h"

#if defined(PORT_WEBVIEW_BRIDGE_EFL)
#include <Elementary.h>
#elif defined(PORT_WEBVIEW_BRIDGE_ECORE_WAYLAND2)
#include <Ecore.h>
#if defined(PORT_WEBVIEW_BRIDGE_ECORE_WAYLAND2_HANDLE_FROM_ELM_WIN)
#include <Elementary.h>
#endif
#elif defined(PORT_EVENTLOOP_BACKEND_EFL)
#include <Ecore.h>
#endif

#if defined(PORT_WEBVIEW_BRIDGE_ECORE_WAYLAND2)
#define EFL_BETA_API_SUPPORT
#include <Ecore_Wl2.h>
#undef EFL_BETA_API_SUPPORT
#include <wayland-client.h>
#endif

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#ifdef STARFISH_ENABLE_TEST
extern int g_testCompatibleMode;
extern int g_startUpFlag;
#endif

#if defined(PORT_WEBVIEW_BRIDGE_GLFW)
#include <signal.h>
#endif

#if defined(STARFISH_ENABLE_TEST) && defined(STARFISH_X86_64)
#include <stdio.h>
#include <signal.h>
#include <execinfo.h>

void bt_sighandler(int sig, struct sigcontext ctx)
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
        printf("[bt] #%d %s\n", i, messages[i]);

        char syscom[256];
        sprintf(syscom, "addr2line %p -e Starfish",
                trace[i]); // last parameter is the name of this app
        system(syscom);
    }

    fflush(stdout);

    // this is the trick: it will trigger the core dump
    signal(sig, SIG_DFL);
    kill(getpid(), sig);
}

// crash test functions
int func_a(int a, char b)
{
    char* p = (char*)0xdeadbeef;
    a = a + b;
    *p = 10; /* CRASH here!! */
    return 2 * a;
}

int func_b()
{
    int res, a = 5;
    res = 5 + func_a(a, 't');
    return res;
}

#endif

#if defined(STARFISH_ENABLE_TEST)
struct smaps_sizes {
    int KernelPageSize;
    int MMUPageSize;
    int Private_Clean;
    int Private_Dirty;
    int Pss;
    int Referenced;
    int Rss;
    int Shared_Clean;
    int Shared_Dirty;
    int Size;
    int Swap;
};

smaps_sizes getSmapsStats()
{
    // Setup our pipe for reading and execute our command.
    char command[512];
    snprintf(command, sizeof(command), "cat /proc/%d/smaps", getpid());
    FILE* file = popen(command, "r");

    struct smaps_sizes sizes;
    memset(&sizes, 0, sizeof sizes);

    char line[BUFSIZ];
    while (fgets(line, sizeof line, file)) {
        // puts(line);
        char substr[32];
        int n;
        if (sscanf(line, "%31[^:]: %d", substr, &n) == 2) {
            if (strcmp(substr, "KernelPageSize") == 0) {
                sizes.KernelPageSize += n;
            } else if (strcmp(substr, "MMUPageSize") == 0) {
                sizes.MMUPageSize += n;
            } else if (strcmp(substr, "Private_Clean") == 0) {
                sizes.Private_Clean += n;
            } else if (strcmp(substr, "Private_Dirty") == 0) {
                sizes.Private_Dirty += n;
            } else if (strcmp(substr, "Pss") == 0) {
                sizes.Pss += n;
            } else if (strcmp(substr, "Referenced") == 0) {
                sizes.Referenced += n;
            } else if (strcmp(substr, "Rss") == 0) {
                sizes.Rss += n;
            } else if (strcmp(substr, "Shared_Clean") == 0) {
                sizes.Shared_Clean += n;
            } else if (strcmp(substr, "Shared_Dirty") == 0) {
                sizes.Shared_Dirty += n;
            } else if (strcmp(substr, "Size") == 0) {
                sizes.Size += n;
            } else if (strcmp(substr, "Swap") == 0) {
                sizes.Swap += n;
            }
        }
    }
    fclose(file);
    return sizes;
}
#endif

#if defined(PORT_WEBVIEW_BRIDGE_GLFW)
static volatile sig_atomic_t g_doneFlag = 0;

static void setDoneFlag(int sig, siginfo_t* siginfo, void* context)
{
    g_doneFlag = 1;
}
#endif

int main(int argc, char* argv[])
{
#if defined(STARFISH_ENABLE_TEST) && defined(STARFISH_X86_64)
    /* Install our signal handler */
    struct sigaction sa;

    sa.sa_handler = (void (*)(int))bt_sighandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
#endif

#ifndef NDEBUG
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
#endif

#if !defined(STARFISH_ANDROID) && !defined(STARFISH_WINDOWS)
    // Changing these options can reducing {malloc, free} internal memory pool
    // usage
    // for big chunk ex) packets for MSE
    mallopt(M_MMAP_THRESHOLD, 2048);
    mallopt(M_MMAP_MAX, 1024 * 1024);
#endif

#if defined(PORT_WEBVIEW_BRIDGE_EFL)
    const char* defaultEngine = "gl";
    const char* engine = getenv("STARFISH_ELM_ENGINE");
    if (!engine || strlen(engine) == 0) {
        engine = defaultEngine;
    }
    const char* defaultConfig = "opengl";
    const char* config = getenv("STARFISH_ELM_CONFIG");
    if (!config || strlen(config) == 0) {
        config = defaultConfig;
    }

    // printf("engine-> %s\n", engine);
    // printf("config-> %s\n", config);

    setenv("ELM_ENGINE", engine, 1);
#endif

#if defined(PORT_WEBVIEW_BRIDGE_EFL)
    elm_init(0, 0);
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
#if defined(PORT_COMPOSITOR_BACKEND_GL)
    elm_config_accel_preference_set("opengl");
#else
    elm_config_accel_preference_set(config);
#endif
#elif defined(PORT_EVENTLOOP_BACKEND_EFL)
    ecore_init();
#endif

#ifdef STARFISH_ENABLE_TEST
    Starfish::StarfishTestCompatibleMode testCompatibleMode =
        Starfish::StarfishTestCompatibleMode::Normal;
#endif

    int flag = 0;

    if (argc == 1) {
        puts("please specify url");
        return -1;
    }

    // sig handling tester
    // printf("%d\n", func_b());

    std::string screenShot;
    std::string customUserAgentString;
    std::string builtinPolyfillPathString;
    int width = 1280, height = 720;
#ifdef STARFISH_TIZEN_TV
    width = 1920;
    height = 1080;
#endif
    int x = 0, y = 0;
    float scaleFactor = 1;
    bool enableSecurity = true;
    LWE::TTSMode ttsMode = LWE::TTSMode::Default;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--dump-computed-style") == 0) {
            flag |= Starfish::enableComputedStyleDump;
        } else if (strcmp(argv[i], "--dump-frame-tree") == 0) {
            flag |= Starfish::enableFrameTreeDump;
        } else if (strcmp(argv[i], "--dump-stacking-context") == 0) {
            flag |= Starfish::enableStackingContextDump;
        } else if (strcmp(argv[i], "--dump-hittest") == 0) {
            flag |= Starfish::enableHitTestDump;
        } else if (strcmp(argv[i], "--debug-graphics-layer") == 0) {
            flag |= Starfish::enableDebugGraphicsLayer;
        } else if (strcmp(argv[i], "--debug-repaint-region") == 0) {
            flag |= Starfish::enableDebugRepaintRegion;
        } else if (strcmp(argv[i], "--pixel-test") == 0) {
#ifdef STARFISH_ENABLE_TEST
            Starfish::g_enablePixelTest = true;
            setenv("PIXEL_TEST", "1", 1);
#endif
        } else if (strcmp(argv[i], "--ref-test") == 0) {
#ifdef STARFISH_ENABLE_TEST
            Starfish::g_referenceTestState = 1;
            setenv("HIDE_WINDOW", "1", 1);
#endif
        } else if (strstr(argv[i], "--width=") == argv[i]) {
            width = std::atoi(argv[i] + strlen("--width="));
        } else if (strstr(argv[i], "--height=") == argv[i]) {
            height = std::atoi(argv[i] + strlen("--height="));
        } else if (strcmp(argv[i], "--regression-test") == 0) {
            flag |= Starfish::enableRegressionTest;
        } else if (strstr(argv[i], "--screen-shot=") == argv[i]) {
            screenShot = argv[i] + strlen("--screen-shot=");
            setenv("SCREEN_SHOT_FILE", screenShot.c_str(), 1);
        } else if (strstr(argv[i], "--screen-shot-width=") == argv[i]) {
            setenv("SCREEN_SHOT_WIDTH",
                   argv[i] + strlen("--screen-shot-width="), 1);
        } else if (strstr(argv[i], "--screen-shot-height=") == argv[i]) {
            setenv("SCREEN_SHOT_HEIGHT",
                   argv[i] + strlen("--screen-shot-height="), 1);
        } else if (strcmp(argv[i], "--hide-window") == 0) {
            // regression test, pixel test only
            setenv("HIDE_WINDOW", "1", 1);
            flag |= Starfish::enableRegressionTest;
        } else if (strcmp(argv[i], "--network-log-verbose") == 0) {
            setenv("NETWORK_LOG_VERBOSE", "1", 1);
        } else if (strstr(argv[i], "--posX=") == argv[i]) {
            x = std::atoi(argv[i] + strlen("--posX="));
        } else if (strstr(argv[i], "--posY=") == argv[i]) {
            y = std::atoi(argv[i] + strlen("--posY="));
        } else if (strstr(argv[i], "--device-pixel-ratio=") == argv[i]) {
            scaleFactor = std::atof(argv[i] + strlen("--device-pixel-ratio="));
        } else if (strstr(argv[i], "--useragent=") == argv[i]) {
            customUserAgentString = argv[i] + strlen("--useragent=");
        } else if (strstr(argv[i], "--polyfill=") == argv[i]) {
            builtinPolyfillPathString = argv[i] + strlen("--polyfill=");
        } else if (strstr(argv[i], "--enable-chromium-test") == argv[i]) {
#ifdef STARFISH_ENABLE_TEST
            testCompatibleMode =
                Starfish::StarfishTestCompatibleMode::ChromiumLayout;
#endif
        } else if (strcmp(argv[i], "--disable-web-security") == 0) {
            enableSecurity = false;
        } else if (strcmp(argv[i], "--tts-forced") == 0) {
            ttsMode = LWE::TTSMode::Forced;
        }
    }

    if (screenShot.length()) {
        // screenShot = std::string("shot:delay=0.5:file=") + screenShot;
        // setenv("ELM_ENGINE", screenShot.data(), 1);
        setenv("SCREEN_SHOT", screenShot.data(), 1);
        setenv("EXIT_AFTER_SCREEN_SHOT", "1", 1);
    }

    std::string cacheDir;
    const char* homeDir = getenv("HOME");
    if (!homeDir || strlen(homeDir) == 0) {
        cacheDir = "/tmp";
    } else {
        cacheDir = homeDir;
    }
    cacheDir += "/Starfish-cache";

#ifdef STARFISH_DALI
    char buf[128];
    snprintf(buf, sizeof buf, "%d", width);
    setenv("DALI_WINDOW_WIDTH", buf, 1);
    snprintf(buf, sizeof buf, "%d", height);
    setenv("DALI_WINDOW_HEIGHT", buf, 1);
#endif

#if defined(PORT_WEBVIEW_BRIDGE_EFL)
    Evas_Object* wndObj = nullptr;
    wndObj = elm_win_add(NULL, "Starfish", ELM_WIN_BASIC);
    elm_win_title_set(wndObj, STARFISH_NAME);
    elm_win_autodel_set(wndObj, EINA_TRUE);
    evas_object_resize(wndObj, width, height);
    evas_object_move(wndObj, x, y);
    evas_object_show(wndObj);
#ifdef STARFISH_TIZEN
#ifdef STARFISH_ENABLE_TRANSPARENT_WINDOW
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
    evas_object_color_set(bg, 0xff, 0xff, 0xff, 0xff);

    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(wndObj, bg);
    evas_object_show(bg);
#endif
#endif

#elif defined(PORT_WEBVIEW_BRIDGE_ECORE_WAYLAND2)

#if defined(PORT_WEBVIEW_BRIDGE_ECORE_WAYLAND2_HANDLE_FROM_ELM_WIN)
    elm_init(0, 0);
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
    elm_config_accel_preference_set("opengl");

    Evas_Object* wndObj = nullptr;
    wndObj = elm_win_add(NULL, "Starfish", ELM_WIN_BASIC);
    elm_win_title_set(wndObj, STARFISH_NAME);
    elm_win_autodel_set(wndObj, EINA_TRUE);
    evas_object_resize(wndObj, width, height);
    evas_object_move(wndObj, x, y);
    evas_object_show(wndObj);
#else
    Ecore_Wl2_Display* _ecore_wl2_display = NULL;
    if (!ecore_wl2_init())
        return -1;
    if (!_ecore_wl2_display)
        _ecore_wl2_display = ecore_wl2_display_connect(NULL);

    struct wl_display* display = ecore_wl2_display_get(_ecore_wl2_display);

    wl_display_dispatch(display);
    wl_display_roundtrip(display);

    auto wndObj =
        ecore_wl2_window_new(_ecore_wl2_display, NULL, x, y, width, height);
    ecore_wl2_window_type_set(wndObj, ECORE_WL2_WINDOW_TYPE_TOPLEVEL);
    auto wlSurface = ecore_wl2_window_surface_get(wndObj);
    ecore_wl2_window_alpha_set(wndObj, EINA_FALSE);
    ecore_wl2_window_show(wndObj);

    size_t count = 0;
    while (count < 3) {
        if (wl_display_dispatch_pending(display) > 0) {
            wl_display_dispatch(display);
        }
        count++;
    }
#endif

#endif

#ifdef STARFISH_ENABLE_TEST
    g_testCompatibleMode = testCompatibleMode;
    g_startUpFlag = flag;

#ifdef PORT_WEBVIEW_BRIDGE_EFL
    {
        const char* path = getenv("SCREEN_SHOT");
        const char* hide = getenv("HIDE_WINDOW");
        if ((path && strlen(path)) || (hide && strlen(hide))) {
            evas_object_hide(wndObj);
        } else {
            evas_object_show(wndObj);
        }
    }
#endif
#endif

    LWE::LWE::Initialize("/tmp/Starfish_localStorage.txt",
                         "/tmp/Starfish_Cookies.txt", cacheDir.data());

#if defined(PORT_WEBVIEW_BRIDGE_EFL)
    LWE::WebView* webView =
        LWE::WebView::Create(wndObj, x, y, width, height, scaleFactor, "serif",
                             "ko-KR", "Asia/Seoul");
#elif defined(PORT_WEBVIEW_BRIDGE_ECORE_WAYLAND2)
    // Ecore_wayland2 WebView only provides full-window control
    LWE::WebView* webView =
        LWE::WebView::Create(wndObj, 0, 0, width, height, scaleFactor, "serif",
                             "ko-KR", "Asia/Seoul");
#else
    LWE::WebView* webView =
        LWE::WebView::Create(nullptr, x, y, width, height, scaleFactor, "serif",
                             "ko-KR", "Asia/Seoul");
#endif

    {
        auto settings = webView->GetSettings();
        if (customUserAgentString.length()) {
            settings.SetUserAgentString(customUserAgentString);
        }

        if (!enableSecurity) {
            settings.SetWebSecurityMode(LWE::WebSecurityMode::Disable);
        }
        settings.SetTTSMode(ttsMode);
        webView->SetSettings(settings);
    }

#if defined(PORT_WEBVIEW_BRIDGE_EFL)
    Evas_Object* obj = (Evas_Object*)webView->Unwrap();
    evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(wndObj, obj);

// evas_object_move(obj, 100, 100);
// evas_object_resize(obj, 300, 400);
#endif

    try {
        webView->LoadURL(std::string(argv[1]));
    } catch (...) {
        fprintf(stderr, "Exception: WebView");
        return -1;
    }

    webView->Focus();

#if defined(PORT_WEBVIEW_BRIDGE_EFL)
    /// example code about diligently getting focus in EFL
    auto focusInHandler = [](void* data, Evas* e, Evas_Object* obj,
                             void* event_info) {
        LWE::WebView* wv = (LWE::WebView*)data;
        wv->Focus();
    };
    evas_object_event_callback_add(wndObj, EVAS_CALLBACK_FOCUS_IN,
                                   focusInHandler, webView);

    auto destroyHandler = [](void* data, Evas* e, Evas_Object* obj,
                             void* event_info) {
        LWE::WebView* wv = (LWE::WebView*)data;
        wv->Destroy();
    };
    evas_object_event_callback_add(wndObj, EVAS_CALLBACK_DEL, destroyHandler,
                                   webView);
#endif

#if defined(PORT_EVENTLOOP_BACKEND_EFL) && \
    (defined(STARFISH_ENABLE_TEST) || defined(STARFISH_ENABLE_SHELL))
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
                        puts(p->webView->EvaluateJavaScript(p->buf).data());
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
#endif

#if defined(PORT_WEBVIEW_BRIDGE_EFL)
    elm_run();
#elif defined(PORT_EVENTLOOP_BACKEND_EFL) || \
    defined(PORT_WEBVIEW_BRIDGE_ECORE_WAYLAND2)
    ecore_main_loop_begin();
#elif defined(PORT_WEBVIEW_BRIDGE_GLFW)
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_sigaction = setDoneFlag;
    act.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    while (!g_doneFlag) {
        usleep(100);
    }
#endif

#if !defined(PORT_WEBVIEW_BRIDGE_EFL) // evas object delete callback will call
                                      // destroy function
    webView->Destroy();
#endif

#if defined(PORT_WEBVIEW_BRIDGE_EFL)
    elm_shutdown();
#elif defined(PORT_EVENTLOOP_BACKEND_EFL) || \
    defined(PORT_WEBVIEW_BRIDGE_ECORE_WAYLAND2)
    ecore_shutdown();
#endif

    webView = nullptr;
    LWE::LWE::Finalize();

#if defined(STARFISH_ENABLE_TEST)
    auto stat = getSmapsStats();
    STARFISH_LOG_INFO(
        "PSS[%.1f MB] "
        "RSS[%.1f MB] "
        "Private_Dirty[%.1fMB]\n",
        stat.Pss / 1024.f, stat.Rss / 1024.f, stat.Private_Dirty / 1024.f);
#endif

    return 0;
}
