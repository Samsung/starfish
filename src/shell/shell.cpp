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
#include "StarFishConfig.h"
#include "core/dom/Document.h"
#include "StarFish.h"

#include "binding/ScriptBindingInstance.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "platform/multimedia/Demuxer.h"
#include "StarFishPublic.h"
#include "LWEWebView.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include <pthread.h>

#if !defined(STARFISH_WINDOWS)

#if defined(STARFISH_DALI)

#if defined(STARFISH_TIZEN)
#include <tbm_surface.h>
#endif

#include <dali-toolkit/dali-toolkit.h>
#include "platform/window/PlatformWindow.h"

#include "core/dom/MouseEvent.h"
#include "core/dom/TouchEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "platform/event/PlatformKeyEventData.h"
#include <uv.h>
extern bool g_MainLoopAlive;
#endif

#if defined(PORT_WINDOW_BACKEND_EFL)
#include <Elementary.h>
#elif defined(PORT_WINDOW_BACKEND_EFL_HEADLESS)
#include <Ecore.h>
#endif

bool hasEnding(std::string const& fullString, std::string const& ending)
{
    if (fullString.length() >= ending.length()) {
        return (0 ==
                fullString.compare(fullString.length() - ending.length(),
                                   ending.length(), ending));
    } else {
        return false;
    }
}

// #define STARFISH_ENABLE_TV_MEMPS

#ifdef STARFISH_ENABLE_TV_MEMPS
#ifndef STARFISH_ENABLE_MULTIMEDIA
#undef STARFISH_ENABLE_TV_MEMPS
#endif
#endif

#ifdef STARFISH_ENABLE_TV_MEMPS
#ifndef STARFISH_TIZEN_TV
#undef STARFISH_ENABLE_TV_MEMPS
#endif
#endif

#ifdef STARFISH_ENABLE_TV_MEMPS
#include <chrono>

static void printMemps(
    std::chrono::time_point<std::chrono::system_clock>& startTime)
{
    std::chrono::time_point<std::chrono::system_clock> currentTime =
        std::chrono::system_clock::now();
    std::chrono::duration<double> diff = currentTime - startTime;
    char command[512];
#ifdef STARFISH_TIZEN_TV_EMULATOR
    snprintf(command, sizeof(command),
             "memps -v 2> /dev/null | sed 's/^[ \\t]*//' | sed 's/,//g' | grep "
             "-E %d | grep -v grep | egrep -o '[0-9]+ '",
             getpid());
    FILE* file = popen(command, "r");
    char line[512];
    int tmp, pss, gempss, gemrss;
    fscanf(file, "%d%d%d%d%d%d%d%d%d%d%d", &tmp, &tmp, &tmp, &tmp, &tmp, &tmp,
           &pss, &tmp, &gempss, &gemrss, &tmp);
    STARFISH_LOG_INFO("[MEMPS] PSS: %d, GEM_PSS: %d, GEM_RSS: %d\n", pss,
                      gempss, gemrss);
#else
    snprintf(command, sizeof(command),
             "vd_memps -x 1 2> /dev/null  | sed 's/^[ \\t]*//' | sed 's/,//g' "
             "| grep -E %d | grep -v grep | egrep -o '[0-9]+ '",
             getpid());
    FILE* file = popen(command, "r");
    char line[512];
    int tmp, pss, gem, maliprocess, malidevice;
    fscanf(file, "%d%d%d%d%d%d%d%d%d%d%d%d", &tmp, &tmp, &tmp, &tmp, &tmp, &tmp,
           &pss, &tmp, &tmp, &gem, &maliprocess, &malidevice);
    STARFISH_LOG_INFO(
        "[VD_MEMPS][%lf sec] PSS: %d, GEM: %d, MALI(PROCESS): %d, "
        "MALI(DEVICE): %d\n",
        diff.count(), pss, gem, maliprocess, malidevice);
#endif
    fclose(file);
}
#endif

#ifdef STARFISH_DALI
using namespace Dali;

char* url = nullptr;
extern uv_async_t g_launcher_handle;
extern pthread_mutex_t* g_initMutex;

void uv_term_cb(uv_signal_t* handle, int signum);
bool needToInitMainThread();
void initMainThread(void* (*f)(void*));
StarFish::PlatformKeyEventData DaliEventKeyToPlatformKeyEventData(
    const char* DALIKeyString, bool isShiftPressed);

class DaliShellController : public ConnectionTracker {
public:
    DaliShellController(Application& application, int width, int height)
        : m_isInit(false)
        , m_isMouseLbuttonDown(false)
        , m_width(width)
        , m_height(height)
        , m_starFish(nullptr)
        , mApplication(application)
    {
        mApplication.InitSignal().Connect(this, &DaliShellController::Create);
    }
    ~DaliShellController()
    {
#if defined(STARFISH_TIZEN)
        if (tbm_surface_unmap(m_surface1) != TBM_SURFACE_ERROR_NONE) {
            printf("Failed to unmap tbm_surface\n");
        }
        if (tbm_surface_unmap(m_surface2) != TBM_SURFACE_ERROR_NONE) {
            printf("Failed to unmap tbm_surface\n");
        }
        if (tbm_surface_destroy(m_surface1) != TBM_SURFACE_ERROR_NONE) {
            printf("Failed to destroy tbm_surface\n");
        }
        if (tbm_surface_destroy(m_surface2) != TBM_SURFACE_ERROR_NONE) {
            printf("Failed to destroy tbm_surface\n");
        }
#endif
    }

    bool updateTick();
    void Create(Application& application);
    bool TouchEventHandler(Dali::Actor actor, const Dali::TouchData& data);
    bool HoverEventHandler(Dali::Actor actor, const Dali::HoverEvent& event);
    void KeyEventHandler(const Dali::KeyEvent& event);

    bool m_isInit;
    bool m_isMouseLbuttonDown;
    int m_width;
    int m_height;
    StarFish::StarFish* m_starFish;
    Application& mApplication;
#if defined(STARFISH_TIZEN)
    tbm_surface_h m_surface1;
    tbm_surface_h m_surface2;
    tbm_surface_info_s m_surface_info1;
    tbm_surface_info_s m_surface_info2;

    Dali::NativeImageSourcePtr m_daliImg_src;
    Dali::NativeImage m_daliImg;
#else
    Dali::BufferImage m_daliImg;
#endif
    Dali::Toolkit::ImageView m_mainView;
    Dali::Timer m_timer;
};

void* mainShellThread(void* data)
{
    uv_async_init(
        uv_default_loop(), &g_launcher_handle, [](uv_async_t* handle) {

            int flag = 0;
            int x = 0;
            int y = 0;
            DaliShellController* app = (DaliShellController*)handle->data;

            StarFish::ScreenInfo info;
            info.rect.setWidth(app->m_width);
            info.rect.setHeight(app->m_height);
            info.availableRect.setWidth(app->m_width);
            info.availableRect.setHeight(app->m_height);

            std::string cacheDir(getenv("HOME"));
            cacheDir += "/Starfish-cache";

            app->m_starFish = new StarFish::StarFish(
                (StarFish::StarFishStartUpFlag)flag, "ko-KR", "Asia/Seoul", app,
                app->m_width, app->m_height, x, y, 1,
                StarFish::String::createASCIIString("samsungOne"), info, "",
                "/tmp/StarFish_Cookies.txt", cacheDir.data(),
                StarFish::String::emptyString, StarFish::String::emptyString);

#if defined(STARFISH_TIZEN)
            app->m_starFish->registerFrameBuffer(
                app->m_surface_info1.planes[0].ptr,
                app->m_surface_info2.planes[0].ptr);
#else
        app->m_starFish->registerFrameBuffer(
            (void*)app->m_daliImg.GetBuffer(),nullptr);
#endif
            app->m_isInit = true;
            pthread_mutex_unlock(g_initMutex);
        });
    g_MainLoopAlive = true;
    pthread_mutex_unlock(g_initMutex);
    while (true) {
        uv_run(uv_default_loop(), UV_RUN_ONCE);
    }
    return NULL;
}

#if defined(STARFISH_TIZEN)
bool DaliShellController::updateTick()
{
    if (m_starFish) {
        int bufferIdx = m_starFish->updateFrameBuffer();
        if (bufferIdx == 1) {
            Any source(m_surface1);
            m_daliImg_src->SetSource(source);
            Dali::Stage::GetCurrent().KeepRendering(0.0f);
        } else if (bufferIdx == 2) {
            Any source(m_surface2);
            m_daliImg_src->SetSource(source);
            Dali::Stage::GetCurrent().KeepRendering(0.0f);
        }
    }

    return true;
}
#else
bool DaliShellController::updateTick()
{
    if (m_starFish && m_starFish->updateFrameBuffer()) {
        m_daliImg.Update();
    }
    return true;
}
#endif
void DaliShellController::Create(Application& application)
{
    if (needToInitMainThread()) {
        initMainThread(&mainShellThread);
    }

    int width = m_width, height = m_height;

#if defined(STARFISH_TIZEN)
    m_surface1 = tbm_surface_create(m_width, m_height, TBM_FORMAT_ARGB8888);
    m_surface2 = tbm_surface_create(m_width, m_height, TBM_FORMAT_ARGB8888);

    if (tbm_surface_map(m_surface1,
                        TBM_SURF_OPTION_READ | TBM_SURF_OPTION_WRITE,
                        &m_surface_info1) != TBM_SURFACE_ERROR_NONE) {
        printf("Fail to map tbm_surface\n");
    }
    if (tbm_surface_map(m_surface2,
                        TBM_SURF_OPTION_READ | TBM_SURF_OPTION_WRITE,
                        &m_surface_info2) != TBM_SURFACE_ERROR_NONE) {
        printf("Fail to map tbm_surface\n");
    }

    Any source(m_surface1);
    m_daliImg_src = Dali::NativeImageSource::New(source);
    m_daliImg = Dali::NativeImage::New(*m_daliImg_src);
#else
    m_daliImg = Dali::BufferImage::New(width, height, Dali::Pixel::BGRA8888);
#endif
    m_mainView = Dali::Toolkit::ImageView::New(m_daliImg);
    m_mainView.SetParentOrigin(Dali::ParentOrigin::TOP_LEFT);
    m_mainView.SetAnchorPoint(Dali::AnchorPoint::TOP_LEFT);
    m_mainView.SetPosition(0, 0);
    Dali::Stage::GetCurrent().Add(m_mainView);

    // TODO: Need to get screen info from X11.
    // Temporally, rect's width and height are set to window size.
    pthread_mutex_lock(g_initMutex);

    g_launcher_handle.data = this;
    uv_async_send(&g_launcher_handle);

    Dali::Stage::GetCurrent().GetRootLayer().TouchSignal().Connect(
        this, &DaliShellController::TouchEventHandler);
    Dali::Stage::GetCurrent().GetRootLayer().HoveredSignal().Connect(
        this, &DaliShellController::HoverEventHandler);
    Dali::Stage::GetCurrent().KeyEventSignal().Connect(
        this, &DaliShellController::KeyEventHandler);

    m_timer = Dali::Timer::New(20);
    m_timer.TickSignal().Connect(this, &DaliShellController::updateTick);

    m_timer.Start();
    pthread_mutex_lock(g_initMutex);
    pthread_mutex_unlock(g_initMutex);

    struct dummy {
        StarFish::StarFish* starfish;
        char data[128];
    };
    dummy* d = new dummy;
    d->starfish = m_starFish;
    strcpy(d->data, url);
    m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
        nullptr,
        [](size_t, void* data) {
            dummy* d = (dummy*)data;
            StarFish::StarFish* m_sf = d->starfish;
            StarFish::StarFishEnterer enter(m_sf);
            m_sf->loadHTMLDocument(StarFish::String::fromUTF8(&(d->data)[0]));
            delete d;
        },
        d);
}

bool DaliShellController::TouchEventHandler(Dali::Actor actor,
                                            const Dali::TouchData& data)
{
    if (m_starFish == nullptr || !m_isInit) {
        return true;
    }
    if (!(m_starFish->platformWindow() &&
          m_starFish->platformWindow()->webView() &&
          m_starFish->platformWindow()->webView()->mainBrowsingContext())) {
        return true;
    }
    size_t pointCount = data.GetPointCount();
    if (pointCount == 1) {
        // Single touch event

        struct dummy {
            StarFish::StarFish* starfish;
            StarFish::MouseData data;
        };
        dummy* d = new dummy;
        d->starfish = m_starFish;

        Dali::PointState::Type pointState = data.GetState(0);
        const Dali::Vector2& screen = data.GetLocalPosition(0);
        if (pointState == Dali::PointState::DOWN) {
            StarFish::StarFishEnterer enter(m_starFish);
            StarFish::MouseData data(
                StarFish::MouseData::MouseButtonValue::LeftButton,
                StarFish::MouseData::MouseButtonsValue::LeftButtonDown,
                screen.x * m_starFish->screenInfo().deviceScaleFactor,
                screen.y * m_starFish->screenInfo().deviceScaleFactor, 0);
            d->data = data;
            m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
                m_starFish->platformWindow()->webView()->mainBrowsingContext(),
                [](size_t, void* data) {
                    dummy* d = (dummy*)data;
                    StarFish::StarFish* m_sf = d->starfish;
                    StarFish::MouseData mouseData = d->data;
                    m_sf->platformWindow()->dispatchMouseEvent(
                        StarFish::MouseEventKind::MouseEventDown, mouseData);
                    delete d;
                },
                d);
            m_isMouseLbuttonDown = true;
        } else if (pointState == Dali::PointState::UP) {
            StarFish::StarFishEnterer enter(m_starFish);
            StarFish::MouseData data(
                StarFish::MouseData::MouseButtonValue::NoButton,
                StarFish::MouseData::MouseButtonsValue::NoButtonDown,
                screen.x * m_starFish->screenInfo().deviceScaleFactor,
                screen.y * m_starFish->screenInfo().deviceScaleFactor, 0);
            d->data = data;
            m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
                m_starFish->platformWindow()->webView()->mainBrowsingContext(),
                [](size_t, void* data) {
                    dummy* d = (dummy*)data;
                    StarFish::StarFish* m_sf = d->starfish;
                    StarFish::MouseData mouseData = d->data;
                    m_sf->platformWindow()->dispatchMouseEvent(
                        StarFish::MouseEventKind::MouseEventUp, mouseData);
                    delete d;
                },
                d);
            m_isMouseLbuttonDown = false;
        } else {
            StarFish::StarFishEnterer enter(m_starFish);
            unsigned char buttons =
                m_isMouseLbuttonDown
                    ? StarFish::MouseData::MouseButtonsValue::LeftButtonDown
                    : 0;
            StarFish::MouseData data(
                0, buttons,
                screen.x * m_starFish->screenInfo().deviceScaleFactor,
                screen.y * m_starFish->screenInfo().deviceScaleFactor, 0);

            d->data = data;
            m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
                m_starFish->platformWindow()->webView()->mainBrowsingContext(),
                [](size_t, void* data) {
                    dummy* d = (dummy*)data;
                    StarFish::StarFish* m_sf = d->starfish;
                    StarFish::MouseData mouseData = d->data;
                    m_sf->platformWindow()->dispatchMouseEvent(
                        StarFish::MouseEventKind::MouseEventMove, mouseData);
                    delete d;
                },
                d);
        }
    }
    return true;
}

bool DaliShellController::HoverEventHandler(Dali::Actor actor,
                                            const Dali::HoverEvent& event)
{
    if (!m_isInit)
        return true;

    const Dali::Vector2& point = event.GetPoint(0).screen;
    StarFish::StarFishEnterer enter(m_starFish);
    unsigned char buttons =
        m_isMouseLbuttonDown
            ? StarFish::MouseData::MouseButtonsValue::LeftButtonDown
            : 0;
    StarFish::MouseData data(
        0, buttons, point.x * m_starFish->screenInfo().deviceScaleFactor,
        point.y * m_starFish->screenInfo().deviceScaleFactor, 0);

    struct dummy {
        StarFish::StarFish* starfish;
        StarFish::MouseData data;
    };
    dummy* d = new dummy;
    d->starfish = m_starFish;
    d->data = data;
    m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
        m_starFish->platformWindow()->webView()->mainBrowsingContext(),
        [](size_t, void* data) {
            dummy* d = (dummy*)data;
            StarFish::StarFish* m_sf = d->starfish;
            StarFish::MouseData mouseData = d->data;
            m_sf->platformWindow()->dispatchMouseEvent(
                StarFish::MouseEventKind::MouseEventMove, mouseData);
            delete d;
        },
        d);

    return true;
}
void DaliShellController::KeyEventHandler(const Dali::KeyEvent& event)
{
    StarFish::PlatformKeyEventData kdata(StarFish::KeyValue::UnidentifiedKey);
    if (32 < event.keyPressed.c_str()[0] && 127 > event.keyPressed.c_str()[0]) {
        kdata = StarFish::PlatformKeyEventData(
            (StarFish::KeyValue)event.keyPressed.c_str()[0]);
    } else {
        kdata = DaliEventKeyToPlatformKeyEventData(event.keyPressedName.c_str(),
                                                   event.keyModifier & 1);
    }
    struct dummy {
        StarFish::StarFish* starfish;
        StarFish::PlatformKeyEventData data;
    };
    dummy* d = new dummy;
    d->starfish = m_starFish;
    d->data = kdata;
    if (event.state == Dali::KeyEvent::Down) {
        m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
            m_starFish->platformWindow()->webView()->mainBrowsingContext(),
            [](size_t, void* data) {
                dummy* d = (dummy*)data;
                StarFish::StarFish* m_sf = d->starfish;
                StarFish::PlatformKeyEventData keyData = d->data;
                StarFish::StarFishEnterer enter(m_sf);
                m_sf->platformWindow()->dispatchKeyEvent(
                    StarFish::KeyEventKind::KeyEventDown, keyData);
                m_sf->platformWindow()->dispatchKeyEvent(
                    StarFish::KeyEventKind::KeyEventPress, keyData);
                delete d;
            },
            d);
    } else if (event.state == Dali::KeyEvent::Up) {
        m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
            m_starFish->platformWindow()->webView()->mainBrowsingContext(),
            [](size_t, void* data) {
                dummy* d = (dummy*)data;
                StarFish::StarFish* m_sf = d->starfish;
                StarFish::PlatformKeyEventData keyData = d->data;
                StarFish::StarFishEnterer enter(m_sf);
                m_sf->platformWindow()->dispatchKeyEvent(
                    StarFish::KeyEventKind::KeyEventUp, keyData);
                delete d;
            },
            d);
    }
}

#endif

#if defined(STARFISH_ENABLE_TEST) && defined(STARFISH_64)
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
        sprintf(syscom, "addr2line %p -e StarFish",
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

int main(int argc, char* argv[])
{
#if defined(STARFISH_ENABLE_TEST)
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

#if defined(STARFISH_TIZEN)
    setenv("ELM_ENGINE", engine, 1);
#endif

#if defined(PORT_WINDOW_BACKEND_EFL)
    elm_init(0, 0);
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

#if defined(PORT_GRAPHIC_BACKEND_EFL_CAIRO) && defined(STARFISH_TIZEN)
    elm_config_accel_preference_set(config);
#endif
#elif defined(PORT_WINDOW_BACKEND_EFL_HEADLESS)
    ecore_init();
    ecore_app_args_set(argc, (const char**)argv);
#endif

#ifdef STARFISH_ENABLE_TEST
    StarFish::StarFishTestCompatibleMode testCompatibleMode =
        StarFish::StarFishTestCompatibleMode::Normal;
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

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--dump-computed-style") == 0) {
            flag |= StarFish::enableComputedStyleDump;
        } else if (strcmp(argv[i], "--dump-frame-tree") == 0) {
            flag |= StarFish::enableFrameTreeDump;
        } else if (strcmp(argv[i], "--dump-stacking-context") == 0) {
            flag |= StarFish::enableStackingContextDump;
        } else if (strcmp(argv[i], "--dump-hittest") == 0) {
            flag |= StarFish::enableHitTestDump;
        } else if (strcmp(argv[i], "--pixel-test") == 0) {
#ifdef STARFISH_ENABLE_TEST
            StarFish::g_enablePixelTest = true;
            setenv("PIXEL_TEST", "1", 1);
#endif
        } else if (strstr(argv[i], "--width=") == argv[i]) {
            width = std::atoi(argv[i] + strlen("--width="));
        } else if (strstr(argv[i], "--height=") == argv[i]) {
            height = std::atoi(argv[i] + strlen("--height="));
        } else if (strcmp(argv[i], "--regression-test") == 0) {
            flag |= StarFish::enableRegressionTest;
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
        } else if (strcmp(argv[i], "--mem-log-dump") == 0) {
#ifdef STARFISH_ENABLE_TEST
            StarFish::g_memLogDump = true;
#endif
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
                StarFish::StarFishTestCompatibleMode::ChromiumLayout;
#endif
        }
    }

    if (screenShot.length()) {
        // screenShot = std::string("shot:delay=0.5:file=") + screenShot;
        // setenv("ELM_ENGINE", screenShot.data(), 1);
        setenv("SCREEN_SHOT", screenShot.data(), 1);
        setenv("EXIT_AFTER_SCREEN_SHOT", "1", 1);
    }

#ifdef STARFISH_ENABLE_TV_MEMPS
    pthread_t vdm;
    pthread_attr_t attrAttr;
    pthread_attr_init(&attrAttr);
    pthread_create(
        &vdm, &attrAttr,
        [](void* data) -> void* {
            std::chrono::time_point<std::chrono::system_clock> startTime =
                std::chrono::system_clock::now();
            while (1) {
                // Print result of memps (or vd_memps) every 5 seconds
                sleep(5);
                printMemps(startTime);
            }
            return NULL;
        },
        NULL);
#endif

#if defined(PORT_GRAPHIC_BACKEND_GENERAL_BUFFER)
    url = argv[1];
    Application application = Application::New(&argc, &argv);
    DaliShellController shell(application, width, height);
    application.MainLoop();
#else

    // TODO: Need to get screen info from X11.
    // Temporally, rect's width and height are set to window size.
    StarFish::ScreenInfo info;
    info.rect.setWidth(width);
    info.rect.setHeight(height);
    info.availableRect.setWidth(width);
    info.availableRect.setHeight(height);
#if defined(STARFISH_EFL_CAIRO)
    info.deviceScaleFactor = scaleFactor;
#endif

    std::string cacheDir(getenv("HOME"));
    cacheDir += "/Starfish-cache";
    StarFish::StarFish* sf = new StarFish::StarFish(
        (StarFish::StarFishStartUpFlag)flag, "ko-KR", "Asia/Seoul", nullptr,
        width, height, x, y, 1,
        StarFish::String::createASCIIString("samsungOne"), info,
        "/tmp/StarFish_localStorage.txt", "/tmp/StarFish_Cookies.txt",
        cacheDir.data(),
        StarFish::String::fromUTF8(customUserAgentString.data()),
        StarFish::String::fromUTF8(builtinPolyfillPathString.data()));

    LWE::WebView* webView = LWE::WebView::Create(sf);

#ifdef STARFISH_ENABLE_TEST
    sf->setTestCompatibleMode(testCompatibleMode);
#endif

#if defined(STARFISH_ENABLE_INSPECTOR)
    sf->setupInspector();
#endif

    try {
        webView->LoadURL(std::string(argv[1]));
    } catch (...) {
        fprintf(stderr, "Exception: WebView");
        return -1;
    }
#if defined(STARFISH_ENABLE_TEST) || defined(STARFISH_ENABLE_SHELL)
    pthread_t t;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&t, &attr,
                   [](void* data) -> void* {
                       char buf[1024];
                       sleep(1);
                       while (1) {
                           fgets(buf, 1024, stdin);
                           struct Pass {
                               StarFish::StarFish* sf;
                               char* buf;
                           };
                           char* b = new char[1024];
                           Pass* pass = new Pass;
                           pass->buf = b;
                           pass->sf = (StarFish::StarFish*)data;
                           memcpy(b, buf, sizeof buf);
                           ecore_thread_main_loop_begin();
                           ecore_animator_add(
                               [](void* data) -> Eina_Bool {
                                   Pass* p = (Pass*)data;

                                   if (strncmp(p->buf, "!exit", 5) == 0) {
                                       delete p->sf;

                                       GC_gcollect_and_unmap();
                                       GC_gcollect_and_unmap();
                                       GC_gcollect_and_unmap();
                                       GC_gcollect_and_unmap();
                                       exit(-1);
                                   }

                                   StarFish::StarFishEnterer enter(p->sf);
                                   StarFish::String* str = p->sf->evaluate(
                                       StarFish::String::fromUTF8(p->buf));
                                   auto s = str->toUTF8NonGCString();
                                   puts(s.data());

                                   delete[] p->buf;
                                   delete p;
                                   return ECORE_CALLBACK_CANCEL;
                               },
                               pass);
                           ecore_thread_main_loop_end();
                       }
                       return NULL;
                   },
                   sf);
#endif

    sf->run();
    webView->Destroy();
    delete webView;
    webView = nullptr;
    sf = nullptr;
#endif

#if defined(PORT_WINDOW_BACKEND_EFL)
    elm_shutdown();
#elif defined(PORT_WINDOW_BACKEND_EFL_HEADLESS)
    ecore_shutdown();
#endif

#ifndef NDEBUG
    clearStack<102400>();
#endif

    GC_gcollect_and_unmap();

    return 0;
}

#endif
