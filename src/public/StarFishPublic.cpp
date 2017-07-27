/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "StarFish.h"

#include "StarFishPublic.h"

#include <cstdlib>

#define TO_STARFISH(instance) ((StarFish::StarFish*)instance->m_starfish)
#if defined(STARFISH_DALI)
#include "core/dom/Document.h"

#include "core/modules/threading/Locker.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include <pthread.h>

#include <dali-toolkit/dali-toolkit.h>
#include "platform/window/PlatformWindow.h"

#include "core/dom/MouseEvent.h"
#include "core/dom/TouchEvent.h"
#include "core/dom/KeyboardEvent.h"

#include <uv.h>

uv_signal_t g_sigterm;
uv_signal_t g_sigint;

void uv_term_cb(uv_signal_t* handle, int signum)
{
    exit(0);
}
bool needToInitMainThread()
{
    return !uv_loop_alive(uv_default_loop());
}

void* mainThread(void* data)
{
    uv_signal_init(uv_default_loop(), &g_sigterm);
    uv_signal_start(&g_sigterm, &uv_term_cb, SIGTERM);

    uv_signal_init(uv_default_loop(), &g_sigint);
    uv_signal_start(&g_sigint, &uv_term_cb, SIGINT);

    uv_idle_t idler;
    idler.data = data;
    uv_idle_init(uv_default_loop(), &idler);
    uv_idle_start(&idler, [](uv_idle_t* handle) {
        StarFish::Mutex* initMutext = (StarFish::Mutex*)handle->data;
        initMutext->unlock();
    });
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    return NULL;
}

void initMainThread()
{
    StarFish::Mutex* initMutex = new StarFish::Mutex();

    initMutex->lock();
    pthread_t t;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&t, &attr, mainThread, initMutex);

    {
        StarFish::Locker<StarFish::Mutex> l(*initMutex);
    }
}

class StarFishController : public Dali::ConnectionTracker {
public:
    StarFishController(StarFishInstance* instance)
        : m_isInit(false)
        , m_InitMutex(new StarFish::Mutex())
    {
        m_instance = instance;
    }
    bool updateBuffer()
    {
        if (TO_STARFISH(m_instance) != nullptr && m_isInit) {
            if (((StarFish::StarFish*)m_instance->m_starfish)->needsUpdate()) {
                m_daliBuffer.Update();
            }
        }
        return true;
    }

    bool TouchEventHandler(Dali::Actor actor, const Dali::TouchData& data)
    {
        if (TO_STARFISH(m_instance) == nullptr || !m_isInit)
            return true;

        size_t pointCount = data.GetPointCount();
        if (pointCount == 1) {
            // Single touch event

            struct dummy {
                StarFish::StarFish* starfish;
                StarFish::MouseData data;
            };
            dummy* d = new dummy;
            d->starfish = TO_STARFISH(m_instance);

            Dali::PointState::Type pointState = data.GetState(0);
            const Dali::Vector2& screen = data.GetScreenPosition(0);
            if (pointState == Dali::PointState::DOWN) {
                StarFish::StarFishEnterer enter(TO_STARFISH(m_instance));
                StarFish::MouseData data(
                    StarFish::MouseData::MouseButtonValue::LeftButton,
                    StarFish::MouseData::MouseButtonsValue::LeftButtonDown,
                    screen.x, screen.y);
                d->data = data;
                TO_STARFISH(m_instance)
                    ->messageLoop()
                    ->addIdlerWithNoGCRootingInOtherThread(
                        nullptr,
                        [](size_t, void* data) {
                            dummy* d = (dummy*)data;
                            StarFish::StarFish* m_sf = d->starfish;
                            StarFish::MouseData mouseData = d->data;
                            m_sf->platformWindow()->dispatchMouseEvent(
                                StarFish::PlatformWindow::MouseEventDown,
                                mouseData);
                            delete d;
                        },
                        d);
                m_isMouseLbuttonDown = true;
            } else if (pointState == Dali::PointState::UP) {
                StarFish::StarFishEnterer enter(TO_STARFISH(m_instance));
                StarFish::MouseData data(
                    StarFish::MouseData::MouseButtonValue::NoButton,
                    StarFish::MouseData::MouseButtonsValue::NoButtonDown,
                    screen.x, screen.y);
                d->data = data;
                TO_STARFISH(m_instance)
                    ->messageLoop()
                    ->addIdlerWithNoGCRootingInOtherThread(
                        nullptr,
                        [](size_t, void* data) {
                            dummy* d = (dummy*)data;
                            StarFish::StarFish* m_sf = d->starfish;
                            StarFish::MouseData mouseData = d->data;
                            m_sf->platformWindow()->dispatchMouseEvent(
                                StarFish::PlatformWindow::MouseEventUp,
                                mouseData);
                            delete d;
                        },
                        d);
                m_isMouseLbuttonDown = false;
            } else {
                StarFish::StarFishEnterer enter(TO_STARFISH(m_instance));
                unsigned char buttons =
                    m_isMouseLbuttonDown
                        ? StarFish::MouseData::MouseButtonsValue::LeftButtonDown
                        : 0;
                StarFish::MouseData data(0, buttons, screen.x, screen.y);

                d->data = data;
                TO_STARFISH(m_instance)
                    ->messageLoop()
                    ->addIdlerWithNoGCRootingInOtherThread(
                        nullptr,
                        [](size_t, void* data) {
                            dummy* d = (dummy*)data;
                            StarFish::StarFish* m_sf = d->starfish;
                            StarFish::MouseData mouseData = d->data;
                            m_sf->platformWindow()->dispatchMouseEvent(
                                StarFish::PlatformWindow::MouseEventMove,
                                mouseData);
                            delete d;
                        },
                        d);
            }
        }
        return true;
    }
    bool HoverEventHandler(Dali::Actor actor, const Dali::HoverEvent& event)
    {
        if (TO_STARFISH(m_instance) == nullptr || !m_isInit)
            return true;

        const Dali::Vector2& point = event.GetPoint(0).screen;
        StarFish::StarFishEnterer enter(TO_STARFISH(m_instance));
        unsigned char buttons =
            m_isMouseLbuttonDown
                ? StarFish::MouseData::MouseButtonsValue::LeftButtonDown
                : 0;
        StarFish::MouseData data(0, buttons, point.x, point.y);

        struct dummy {
            StarFish::StarFish* starfish;
            StarFish::MouseData data;
        };
        dummy* d = new dummy;
        d->starfish = TO_STARFISH(m_instance);
        d->data = data;
        TO_STARFISH(m_instance)
            ->messageLoop()
            ->addIdlerWithNoGCRootingInOtherThread(
                nullptr,
                [](size_t, void* data) {
                    dummy* d = (dummy*)data;
                    StarFish::StarFish* m_sf = d->starfish;
                    StarFish::MouseData mouseData = d->data;
                    m_sf->platformWindow()->dispatchMouseEvent(
                        StarFish::PlatformWindow::MouseEventMove, mouseData);
                    delete d;
                },
                d);

        return true;
    }

    bool m_isInit;
    bool m_isMouseLbuttonDown;
    int m_width;
    int m_height;
    StarFishInstance* m_instance;
    Dali::BufferImage m_daliBuffer;
    Dali::Toolkit::ImageView m_mainView;
    Dali::Timer m_timer;
    StarFish::Mutex* m_InitMutex;
    uv_async_t m_uv_handle;
};
#define TO_CONTROLLER(instance) ((StarFishController*)instance->m_data)

void starfishCreate_internal(uv_async_t* handle)
{
    volatile int flag = 0;
    StarFishController* app = (StarFishController*)handle->data;

    StarFish::ScreenInfo info;
    info.rect.setWidth(app->m_width);
    info.rect.setHeight(app->m_height);
    info.availableRect.setWidth(app->m_width);
    info.availableRect.setHeight(app->m_height);

    GC_stack_base tmp;
    tmp.mem_base = (void*)&flag;
    GC_allow_register_threads();
    GC_register_my_thread(&tmp);

    StarFish::StarFish* starFish = new (NoGC) StarFish::StarFish(
        (StarFish::StarFishStartUpFlag)flag, "ko-KR", "Asia/Seoul", nullptr,
        app->m_width, app->m_height, 1, info, "", "");
    starFish->registerFrameBuffer((void*)app->m_daliBuffer.GetBuffer());
    app->m_instance->m_starfish = starFish;
    app->m_isInit = true;
    app->m_InitMutex->unlock();

    starFish->run();

    uv_close((uv_handle_t*)handle, nullptr);
}

#endif

using namespace StarFish;

namespace StarFish {

typedef FILE* (*sfopen_cb)(const char* filename);
typedef long int (*sflength_cb)(FILE* fp);
typedef size_t (*sfread_cb)(void* buf, size_t size, size_t count, FILE* fp);
typedef int (*sfclose_cb)(FILE* fp);
typedef const char* (*sfmatchLocation_cb)(const char* filename);

sfopen_cb open_cb = nullptr;
sflength_cb length_cb = nullptr;
sfread_cb read_cb = nullptr;
sfclose_cb close_cb = nullptr;
sfmatchLocation_cb matchLocation_cb = nullptr;
}

extern "C" STARFISH_EXPORT StarFishInstance* starfishCreate(
    void* window, int windowWidth, int windowHeight, const char* locale,
    const char* timezoneID, float defaultFontSizeMultiplier)
{
#if defined(STARFISH_DALI)
    if (needToInitMainThread()) {
        initMainThread();
    }

    int width = windowWidth, height = windowHeight;

    StarFishInstance* instance = new StarFishInstance;
    instance->m_starfish = nullptr;

    StarFishController* starFishControl = new StarFishController(instance);
    instance->m_data = (void*)starFishControl;

    starFishControl->m_daliBuffer =
        Dali::BufferImage::New(width, height, Dali::Pixel::BGRA8888);
    starFishControl->m_mainView =
        Dali::Toolkit::ImageView::New(starFishControl->m_daliBuffer);
    starFishControl->m_mainView.SetParentOrigin(Dali::ParentOrigin::TOP_LEFT);
    starFishControl->m_mainView.SetAnchorPoint(Dali::AnchorPoint::TOP_LEFT);
    starFishControl->m_mainView.SetPosition(0, 0);
    Dali::Stage::GetCurrent().Add(starFishControl->m_mainView);

    starFishControl->m_width = width;
    starFishControl->m_height = height;

    starFishControl->m_InitMutex->lock();

    uv_async_init(uv_default_loop(), &starFishControl->m_uv_handle,
                  starfishCreate_internal);
    starFishControl->m_uv_handle.data = starFishControl;
    uv_async_send(&starFishControl->m_uv_handle);

    Dali::Stage::GetCurrent().GetRootLayer().TouchSignal().Connect(
        starFishControl, &StarFishController::TouchEventHandler);
    Dali::Stage::GetCurrent().GetRootLayer().HoveredSignal().Connect(
        starFishControl, &StarFishController::HoverEventHandler);

    starFishControl->m_timer = Dali::Timer::New(20);
    starFishControl->m_timer.TickSignal().Connect(
        starFishControl, &StarFishController::updateBuffer);
    starFishControl->m_timer.Start();
    {
        StarFish::Locker<Mutex> l(*TO_CONTROLLER(instance)->m_InitMutex);
    }

    return instance;
#else
    // TODO: Need to get screen info from X11.
    // Temporally, rect's width and height are set to window size.
    ScreenInfo info;
    info.rect.setWidth(windowWidth);
    info.rect.setHeight(windowHeight);
    info.availableRect.setWidth(windowWidth);
    info.availableRect.setHeight(windowHeight);

    StarFishInstance* instance = new (NoGC) StarFishInstance;
    instance->m_starfish = new StarFish::StarFish(
        (StarFish::StarFishStartUpFlag)0, locale, timezoneID, window,
        windowWidth, windowHeight, defaultFontSizeMultiplier, info, "", "");
#if defined(STARFISH_ENABLE_INSPECTOR)
    TO_STARFISH(instance)->setupInspector();
#endif
    return instance;
#endif
}

extern "C" STARFISH_EXPORT void starfishRemove(StarFishInstance* instance)
{
#if defined(STARFISH_DALI)
    delete (StarFishController*)instance->m_data;
#endif
    delete TO_STARFISH(instance);
    GC_FREE(instance);

    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
}

extern "C" STARFISH_EXPORT void starfishLoadHTMLDocument(
    StarFishInstance* instance, const char* path)
{
#if defined(STARFISH_DALI)
    struct dummy {
        StarFish::StarFish* starfish;
        StarFish::String* data;
    };
    dummy* d = new dummy;
    d->starfish = TO_STARFISH(instance);
    d->data = StarFish::String::fromUTF8(path);
    TO_STARFISH(instance)
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            nullptr,
            [](size_t, void* data) {
                dummy* d = (dummy*)data;
                StarFish::StarFish* m_sf = d->starfish;
                StarFishEnterer enter(m_sf);

                m_sf->loadHTMLDocument(d->data);
                delete d;
            },
            d);
#else
    TO_STARFISH(instance)->loadHTMLDocument(String::fromUTF8(path));
#endif
}

extern "C" STARFISH_EXPORT void starfishNotifyPause(StarFishInstance* instance)
{
    TO_STARFISH(instance)->pause();
}

extern "C" STARFISH_EXPORT void starfishNotifyResume(StarFishInstance* instance)
{
    TO_STARFISH(instance)->resume();
}

extern "C" STARFISH_EXPORT void registerFileOpenCB(
    FILE* (*cb)(const char* fileName))
{
    open_cb = cb;
}

extern "C" STARFISH_EXPORT void registerFileLengthCB(long int (*cb)(FILE* fp))
{
    length_cb = cb;
}

extern "C" STARFISH_EXPORT void registerFileReadCB(
    size_t (*cb)(void* buf, size_t size, size_t count, FILE* fp))
{
    read_cb = cb;
}

extern "C" STARFISH_EXPORT void registerFileCloseCB(int (*cb)(FILE* fp))
{
    close_cb = cb;
}

extern "C" STARFISH_EXPORT void registerFileMatchLocationCB(
    const char* (*cb)(const char* fileName))
{
    matchLocation_cb = cb;
}
