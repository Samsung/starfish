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

#include <pthread.h>

#if defined(PORT_GRAPHIC_BACKEND_GENERAL_BUFFER)
#include <dali-toolkit/dali-toolkit.h>
#include "platform/window/PlatformWindow.h"

#include "core/dom/MouseEvent.h"
#include "core/dom/TouchEvent.h"
#include "core/dom/KeyboardEvent.h"
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

#ifdef PORT_GRAPHIC_BACKEND_GENERAL_BUFFER
using namespace Dali;

char* url = nullptr;

class DaliShellController : public ConnectionTracker {
public:
    DaliShellController(Application& application, int width, int height)
        : m_isMouseLbuttonDown(false)
        , m_width(width)
        , m_height(height)
        , mApplication(application)

    {
        mApplication.InitSignal().Connect(this, &DaliShellController::Create);
    }
    ~DaliShellController()
    {
    }

    bool updateTick()
    {
        m_daliBuffer.Update();
        return true;
    }
    void Create(Application& application)
    {
        int width = m_width, height = m_height;

        m_daliBuffer =
            Dali::BufferImage::New(width, height, Dali::Pixel::BGRA8888);
        m_mainView = Dali::Toolkit::ImageView::New(m_daliBuffer);
        m_mainView.SetParentOrigin(Dali::ParentOrigin::TOP_LEFT);
        m_mainView.SetAnchorPoint(Dali::AnchorPoint::TOP_LEFT);
        m_mainView.SetPosition(0, 0);
        Dali::Stage::GetCurrent().Add(m_mainView);

        // TODO: Need to get screen info from X11.
        // Temporally, rect's width and height are set to window size.

        pthread_t t2;
        pthread_attr_t attr2;
        pthread_attr_init(&attr2);
        pthread_create(
            &t2, &attr2,
            [](void* data) -> void* {
                int flag = 0;
                DaliShellController* app = (DaliShellController*)data;

                ScreenInfo info;
                info.rect.setWidth(app->m_width);
                info.rect.setHeight(app->m_height);
                info.availableRect.setWidth(app->m_width);
                info.availableRect.setHeight(app->m_height);

                app->m_sf = new StarFish::StarFish(
                    (StarFish::StarFishStartUpFlag)flag, "ko-KR", "Asia/Seoul",
                    app, app->m_width, app->m_height, 1, info, "",
                    "/tmp/StarFish_Cookies.txt");
                app->m_sf->registerFrameBuffer(
                    (void*)app->m_daliBuffer.GetBuffer());
                app->m_sf->loadHTMLDocument(String::createASCIIString(url));
                app->m_sf->run();

                return NULL;
            },
            this);

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
                       m_sf);

        Dali::Stage::GetCurrent().GetRootLayer().TouchSignal().Connect(
            this, &DaliShellController::TouchEventHandler);
        Dali::Stage::GetCurrent().GetRootLayer().HoveredSignal().Connect(
            this, &DaliShellController::HoverEventHandler);

        m_timer = Dali::Timer::New(20);
        m_timer.TickSignal().Connect(this, &DaliShellController::updateTick);

        m_timer.Start();
    }

    bool TouchEventHandler(Dali::Actor actor, const Dali::TouchData& data)
    {
        // TODO
        return true;

        size_t pointCount = data.GetPointCount();
        if (pointCount == 1) {
            // Single touch event

            Dali::PointState::Type pointState = data.GetState(0);
            const Dali::Vector2& screen = data.GetScreenPosition(0);
            if (pointState == Dali::PointState::DOWN) {
                StarFishEnterer enter(m_sf);
                MouseData data(MouseData::MouseButtonValue::LeftButton,
                               MouseData::MouseButtonsValue::LeftButtonDown,
                               screen.x, screen.y);

                m_sf->platformWindow()->dispatchMouseEvent(
                    PlatformWindow::MouseEventDown, data);
                m_isMouseLbuttonDown = true;
            } else if (pointState == Dali::PointState::UP) {
                StarFishEnterer enter(m_sf);
                StarFish::MouseData data(
                    MouseData::MouseButtonValue::NoButton,
                    MouseData::MouseButtonsValue::NoButtonDown, screen.x,
                    screen.y);

                m_sf->platformWindow()->dispatchMouseEvent(
                    PlatformWindow::MouseEventUp, data);
                m_isMouseLbuttonDown = false;
            } else {
                StarFishEnterer enter(m_sf);
                unsigned char buttons =
                    m_isMouseLbuttonDown
                        ? MouseData::MouseButtonsValue::LeftButtonDown
                        : 0;
                StarFish::MouseData data(0, buttons, screen.x, screen.y);
                m_sf->platformWindow()->dispatchMouseEvent(
                    PlatformWindow::MouseEventMove, data);
            }
        }
        return true;
    }
    bool HoverEventHandler(Dali::Actor actor, const Dali::HoverEvent& event)
    {
        // TODO
        return true;

        const Dali::Vector2& point = event.GetPoint(0).screen;
        StarFishEnterer enter(m_sf);
        unsigned char buttons =
            m_isMouseLbuttonDown ? MouseData::MouseButtonsValue::LeftButtonDown
                                 : 0;
        StarFish::MouseData data(0, buttons, point.x, point.y);

        m_sf->platformWindow()->dispatchMouseEvent(
            PlatformWindow::MouseEventMove, data);
        return true;
    }

    bool m_isMouseLbuttonDown;
    int m_width;
    int m_height;
    StarFish::StarFish* m_sf;
    Application& mApplication;
    Dali::BufferImage m_daliBuffer;
    Dali::Toolkit::ImageView m_mainView;
    Dali::Timer m_timer;
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
