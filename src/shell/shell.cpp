/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
*/
#include "StarFishConfig.h"
#include "core/dom/Document.h"
#include "StarFish.h"

#include "binding/ScriptBindingInstance.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "platform/multimedia/Demuxer.h"
#include "StarFishPublic.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include <pthread.h>

#if defined(STARFISH_DALI)

#if defined(STARFISH_TIZEN)
#include <tbm_surface.h>
#endif

#include <dali-toolkit/dali-toolkit.h>
#include "platform/window/PlatformWindow.h"

#include "core/dom/MouseEvent.h"
#include "core/dom/TouchEvent.h"
#include "core/dom/KeyboardEvent.h"
#include <uv.h>
#endif

#include <Elementary.h>

using namespace StarFish;

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

uv_signal_t sigterm;
uv_signal_t sigint;

void uv_term_cb(uv_signal_t* handle, int signum);
bool needToInitMainThread();
void initMainThread();

class DaliShellController : public ConnectionTracker {
public:
    DaliShellController(Application& application, int width, int height)
        : m_isInit(false)
        , m_isMouseLbuttonDown(false)
        , m_width(width)
        , m_height(height)
        , m_sf(nullptr)
        , mApplication(application)
        , m_InitMutex(new StarFish::Mutex())
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

#if defined(STARFISH_TIZEN)
    bool updateTick()
    {
        if (m_sf) {
            int bufferIdx = m_sf->frameBufferUpdate();
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
    bool updateTick()
    {
        if (m_sf && m_sf->frameBufferUpdate()) {
            m_daliImg.Update();
        }
        return true;
    }
#endif
    void Create(Application& application)
    {
        if (needToInitMainThread()) {
            initMainThread();
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
        m_daliImg =
            Dali::BufferImage::New(width, height, Dali::Pixel::BGRA8888);
#endif
        m_mainView = Dali::Toolkit::ImageView::New(m_daliImg);
        m_mainView.SetParentOrigin(Dali::ParentOrigin::TOP_LEFT);
        m_mainView.SetAnchorPoint(Dali::AnchorPoint::TOP_LEFT);
        m_mainView.SetPosition(0, 0);
        Dali::Stage::GetCurrent().Add(m_mainView);

        // TODO: Need to get screen info from X11.
        // Temporally, rect's width and height are set to window size.
        m_InitMutex->lock();

        uv_async_init(uv_default_loop(), &m_uv_handle, [](uv_async_t* handle) {
            int flag = 0;
            DaliShellController* app = (DaliShellController*)handle->data;

            ScreenInfo info;
            info.rect.setWidth(app->m_width);
            info.rect.setHeight(app->m_height);
            info.availableRect.setWidth(app->m_width);
            info.availableRect.setHeight(app->m_height);

            GC_stack_base tmp;
            tmp.mem_base = (void*)&flag;
            GC_allow_register_threads();
            GC_register_my_thread(&tmp);

            app->m_sf = new StarFish::StarFish(
                (StarFish::StarFishStartUpFlag)flag, "ko-KR", "Asia/Seoul", app,
                app->m_width, app->m_height, 1, info, "", "");

#if defined(STARFISH_TIZEN)
            app->m_sf->registerFrameBuffer(app->m_surface_info1.planes[0].ptr,
                                           app->m_surface_info2.planes[0].ptr);
#else
            app->m_sf->registerFrameBuffer(
                (void*)app->m_daliImg.GetBuffer(),nullptr);
#endif
            app->m_isInit = true;
            app->m_InitMutex->unlock();

            // app->m_sf->loadHTMLDocument(
            //     StarFish::String::fromUTF8("about:blank"));

            uv_close((uv_handle_t*)handle, nullptr);
        });
        m_uv_handle.data = this;
        uv_async_send(&m_uv_handle);

        Dali::Stage::GetCurrent().GetRootLayer().TouchSignal().Connect(
            this, &DaliShellController::TouchEventHandler);
        Dali::Stage::GetCurrent().GetRootLayer().HoveredSignal().Connect(
            this, &DaliShellController::HoverEventHandler);

        m_timer = Dali::Timer::New(20);
        m_timer.TickSignal().Connect(this, &DaliShellController::updateTick);

        m_timer.Start();
        {
            StarFish::Locker<Mutex> l(*m_InitMutex);
        }

        struct dummy {
            StarFish::StarFish* starfish;
            StarFish::String* data;
        };
        dummy* d = new dummy;
        d->starfish = m_sf;
        d->data = StarFish::String::fromUTF8(url);
        m_sf->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
            nullptr,
            [](size_t, void* data) {
                dummy* d = (dummy*)data;
                StarFish::StarFish* m_sf = d->starfish;
                StarFishEnterer enter(m_sf);

                m_sf->loadHTMLDocument(d->data);
                delete d;
            },
            d);
    }

    bool TouchEventHandler(Dali::Actor actor, const Dali::TouchData& data)
    {
        if (!m_isInit)
            return true;

        size_t pointCount = data.GetPointCount();
        if (pointCount == 1) {
            // Single touch event

            struct dummy {
                StarFish::StarFish* starfish;
                StarFish::MouseData data;
            };
            dummy* d = new dummy;
            d->starfish = m_sf;

            Dali::PointState::Type pointState = data.GetState(0);
            const Dali::Vector2& screen = data.GetScreenPosition(0);
            if (pointState == Dali::PointState::DOWN) {
                StarFishEnterer enter(m_sf);
                MouseData data(MouseData::MouseButtonValue::LeftButton,
                               MouseData::MouseButtonsValue::LeftButtonDown,
                               screen.x, screen.y);
                d->data = data;
                m_sf->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
                    m_sf->platformWindow()->webView()->mainBrowsingContext(),
                    [](size_t, void* data) {
                        dummy* d = (dummy*)data;
                        StarFish::StarFish* m_sf = d->starfish;
                        StarFish::MouseData mouseData = d->data;
                        m_sf->platformWindow()->dispatchMouseEvent(
                            PlatformWindow::MouseEventDown, mouseData);
                        delete d;
                    },
                    d);
                m_isMouseLbuttonDown = true;
            } else if (pointState == Dali::PointState::UP) {
                StarFishEnterer enter(m_sf);
                StarFish::MouseData data(
                    MouseData::MouseButtonValue::NoButton,
                    MouseData::MouseButtonsValue::NoButtonDown, screen.x,
                    screen.y);
                d->data = data;
                m_sf->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
                    m_sf->platformWindow()->webView()->mainBrowsingContext(),
                    [](size_t, void* data) {
                        dummy* d = (dummy*)data;
                        StarFish::StarFish* m_sf = d->starfish;
                        StarFish::MouseData mouseData = d->data;
                        m_sf->platformWindow()->dispatchMouseEvent(
                            PlatformWindow::MouseEventUp, mouseData);
                        delete d;
                    },
                    d);
                m_isMouseLbuttonDown = false;
            } else {
                StarFishEnterer enter(m_sf);
                unsigned char buttons =
                    m_isMouseLbuttonDown
                        ? MouseData::MouseButtonsValue::LeftButtonDown
                        : 0;
                StarFish::MouseData data(0, buttons, screen.x, screen.y);

                d->data = data;
                m_sf->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
                    m_sf->platformWindow()->webView()->mainBrowsingContext(),
                    [](size_t, void* data) {
                        dummy* d = (dummy*)data;
                        StarFish::StarFish* m_sf = d->starfish;
                        StarFish::MouseData mouseData = d->data;
                        m_sf->platformWindow()->dispatchMouseEvent(
                            PlatformWindow::MouseEventMove, mouseData);
                        delete d;
                    },
                    d);
            }
        }
        return true;
    }
    bool HoverEventHandler(Dali::Actor actor, const Dali::HoverEvent& event)
    {
        if (!m_isInit)
            return true;

        const Dali::Vector2& point = event.GetPoint(0).screen;
        StarFishEnterer enter(m_sf);
        unsigned char buttons =
            m_isMouseLbuttonDown ? MouseData::MouseButtonsValue::LeftButtonDown
                                 : 0;
        StarFish::MouseData data(0, buttons, point.x, point.y);

        struct dummy {
            StarFish::StarFish* starfish;
            StarFish::MouseData data;
        };
        dummy* d = new dummy;
        d->starfish = m_sf;
        d->data = data;
        m_sf->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
            m_sf->platformWindow()->webView()->mainBrowsingContext(),
            [](size_t, void* data) {
                dummy* d = (dummy*)data;
                StarFish::StarFish* m_sf = d->starfish;
                StarFish::MouseData mouseData = d->data;
                m_sf->platformWindow()->dispatchMouseEvent(
                    PlatformWindow::MouseEventMove, mouseData);
                delete d;
            },
            d);

        return true;
    }

    bool m_isInit;
    bool m_isMouseLbuttonDown;
    int m_width;
    int m_height;
    StarFish::StarFish* m_sf;
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
    StarFish::Mutex* m_InitMutex;
    uv_async_t m_uv_handle;
};
#endif

int main(int argc, char* argv[])
{
#ifndef NDEBUG
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
#endif

    // setenv("ELM_ENGINE", "gl", 1);
    elm_init(0, 0);
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

    // GC_disable();
    int flag = 0;

    if (argc == 1) {
        puts("please specify file path");
        return -1;
    }
    // STARFISH_LOG_INFO("%d", (int)sizeof (StarFish::ComputedStyle));

    std::string screenShot;
    int width = 1280, height = 720;
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
            g_enablePixelTest = true;
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
            g_memLogDump = true;
#endif
        }
    }

    if (screenShot.length()) {
        // screenShot = std::string("shot:delay=0.5:file=") + screenShot;
        // setenv("ELM_ENGINE", screenShot.data(), 1);
        setenv("SCREEN_SHOT", screenShot.data(), 1);
        setenv("EXIT_AFTER_SCREEN_SHOT", "1", 1);
    }

#if defined(PORT_GRAPHIC_BACKEND_GENERAL_BUFFER)

    url = argv[1];
    Application application = Application::New(&argc, &argv);
    DaliShellController shell(application, width, height);
    application.MainLoop();

#elif defined(PORT_GRAPHIC_BACKEND_EFL)

    elm_init(0, 0);
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

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

    // TODO: Need to get screen info from X11.
    // Temporally, rect's width and height are set to window size.
    ScreenInfo info;
    info.rect.setWidth(width);
    info.rect.setHeight(height);
    info.availableRect.setWidth(width);
    info.availableRect.setHeight(height);
    StarFish::StarFish* sf = new StarFish::StarFish(
        (StarFish::StarFishStartUpFlag)flag, "ko-KR", "Asia/Seoul", nullptr,
        width, height, 1, info, "", "/tmp/StarFish_Cookies.txt");

#if defined(STARFISH_ENABLE_INSPECTOR)
    sf->setupInspector();
#endif
    sf->loadHTMLDocument(String::createASCIIString(argv[1]));

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

                                   StarFishEnterer enter(p->sf);
                                   String* str = p->sf->evaluate(
                                       String::fromUTF8(p->buf));
                                   puts(str->utf8Data());

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

    // sf->messageLoop()->addIdler(test, sf);
    sf->run();
    delete sf;
    sf = nullptr;
#endif

    elm_shutdown();

#ifndef NDEBUG
    clearStack<102400>();
#endif

    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();

    return 0;
}
