/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "StarFish.h"

#include "StarFishPublic.h"

#ifdef TIZEN_DEVICE_API
#include "TizenDeviceAPILoaderForEscargot.h"
#endif
#include "platform/window/PlatformWindow.h"
#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"

#include <cstdlib>

#define TO_STARFISH(instance) ((StarFish::StarFish*)instance->m_starfish)
#if defined(STARFISH_DALI)
#include "core/dom/Document.h"

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
#include "core/page/WebView.h"

#include <uv.h>
#if defined(STARFISH_TIZEN)
#include <tbm_surface.h>
#endif

uv_async_t g_launcher_handle;
pthread_mutex_t* g_initMutex;
bool g_MainLoopAlive = false;

StarFish::KeyboardData DaliEventKeyToKeyboardData(const char* DALIKeyString,
                                                  bool isShiftPressed)
{
    StarFish::KeyValue keyValue = StarFish::KeyValue::UnidentifiedKey;
    if (strcmp("Left", DALIKeyString) == 0) {
        keyValue = StarFish::KeyValue::ArrowLeftKey;
    } else if (strcmp("Right", DALIKeyString) == 0) {
        keyValue = StarFish::KeyValue::ArrowRightKey;
    } else if (strcmp("Up", DALIKeyString) == 0) {
        keyValue = StarFish::KeyValue::ArrowUpKey;
    } else if (strcmp("Down", DALIKeyString) == 0) {
        keyValue = StarFish::KeyValue::ArrowDownKey;
    } else if (strcmp("space", DALIKeyString) == 0) {
        keyValue = StarFish::KeyValue::SpaceKey;
    } else if (strcmp("Return", DALIKeyString) == 0) {
        keyValue = StarFish::KeyValue::EnterKey;
    } else if (strcmp("BackSpace", DALIKeyString) == 0) {
        keyValue = StarFish::KeyValue::BackspaceKey;
    } else if (strcmp("Escape", DALIKeyString) == 0) {
        keyValue = StarFish::KeyValue::EscapeKey;
    } else if (strcmp("minus", DALIKeyString) == 0) {
        if (isShiftPressed) {
            keyValue = StarFish::KeyValue::MinusMarkKey;
        } else {
            keyValue = StarFish::KeyValue::UnderScoreMarkKey;
        }
    } else if (strcmp("equal", DALIKeyString) == 0) {
        if (isShiftPressed) {
            keyValue = StarFish::KeyValue::PlusMarkKey;
        } else {
            keyValue = StarFish::KeyValue::EqualitySignKey;
        }
    } else if (strcmp("bracketleft", DALIKeyString) == 0) {
        if (isShiftPressed) {
            keyValue = StarFish::KeyValue::LeftCurlyBracketMarkKey;
        } else {
            keyValue = StarFish::KeyValue::LeftSquareBracketKey;
        }
    } else if (strcmp("bracketright", DALIKeyString) == 0) {
        if (isShiftPressed) {
            keyValue = StarFish::KeyValue::RightCurlyBracketMarkKey;
        } else {
            keyValue = StarFish::KeyValue::RightSquareBracketKey;
        }
    } else if (strcmp("semicolon", DALIKeyString) == 0) {
        if (isShiftPressed) {
            keyValue = StarFish::KeyValue::ColonMarkKey;
        } else {
            keyValue = StarFish::KeyValue::SemiColonMarkKey;
        }
    } else if (strcmp("apostrophe", DALIKeyString) == 0) {
        if (isShiftPressed) {
            keyValue = StarFish::KeyValue::DoubleQuoteMarkKey;
        } else {
            keyValue = StarFish::KeyValue::SingleQuoteMarkKey;
        }
    } else if (strcmp("comma", DALIKeyString) == 0) {
        if (isShiftPressed) {
            keyValue = StarFish::KeyValue::LessThanMarkKey;
        } else {
            keyValue = StarFish::KeyValue::CommaMarkKey;
        }
    } else if (strcmp("period", DALIKeyString) == 0) {
        if (isShiftPressed) {
            keyValue = StarFish::KeyValue::GreaterThanSignKey;
        } else {
            keyValue = StarFish::KeyValue::PeriodKey;
        }
    } else if (strcmp("slash", DALIKeyString) == 0) {
        if (isShiftPressed) {
            keyValue = StarFish::KeyValue::QuestionMarkKey;
        } else {
            keyValue = StarFish::KeyValue::SlashKey;
        }
    } else if (strlen(DALIKeyString) == 1) {
        char ch = DALIKeyString[0];
        if (ch >= '0' && ch <= '9') {
            if (isShiftPressed) {
                switch (ch) {
                case '1':
                    keyValue = StarFish::KeyValue::ExclamationMarkKey;
                    break;
                case '2':
                    keyValue = StarFish::KeyValue::AtMarkKey;
                    break;
                case '3':
                    keyValue = StarFish::KeyValue::SharpMarkKey;
                    break;
                case '4':
                    keyValue = StarFish::KeyValue::DollarMarkKey;
                    break;
                case '5':
                    keyValue = StarFish::KeyValue::PercentMarkKey;
                    break;
                case '6':
                    keyValue = StarFish::KeyValue::CaretMarkKey;
                    break;
                case '7':
                    keyValue = StarFish::KeyValue::AmpersandMarkKey;
                    break;
                case '8':
                    keyValue = StarFish::KeyValue::AsteriskMarkKey;
                    break;
                case '9':
                    keyValue = StarFish::KeyValue::LeftParenthesisMarkKey;
                    break;
                case '0':
                    keyValue = StarFish::KeyValue::RightParenthesisMarkKey;
                    break;
                }
            } else {
                keyValue = (StarFish::KeyValue)(StarFish::KeyValue::Digit0Key +
                                                ch - '0');
            }
        } else if (ch >= 'a' && ch <= 'z') {
            int kv = StarFish::KeyValue::LowerAKey + ch - 'a';
            if (isShiftPressed) {
                kv -= ('z' - 'a');
                kv -= 7;
            }
            keyValue = (StarFish::KeyValue)kv;
        }
    }
#ifdef STARFISH_TIZEN_TV
    if ((strcmp("XF86Red", DALIKeyString) == 0)) {
        keyValue = StarFish::KeyValue::TabKey;
    }
#endif
    StarFish::KeyboardData kdata(keyValue);
    if (strcmp("Shift_L", DALIKeyString) == 0 ||
        strcmp("Shift_R", DALIKeyString) == 0) {
        kdata.setShiftKey();
    } else if (strcmp("Control_L", DALIKeyString) == 0 ||
               strcmp("Control_R", DALIKeyString) == 0) {
        kdata.setCtrlKey();
    } else if (strcmp("Alt_L", DALIKeyString) == 0 ||
               strcmp("Alt_R", DALIKeyString) == 0) {
        kdata.setAltKey();
    }

    return kdata;
}

void uv_term_cb(uv_signal_t* handle, int signum)
{
    exit(0);
}
bool needToInitMainThread()
{
    return !g_MainLoopAlive;
}

void* mainThread(void* data)
{
    volatile int stack = 0;

    GC_stack_base sb;
    sb.mem_base = (void*)&stack;
    GC_allow_register_threads();
    GC_register_my_thread(&sb);

    g_MainLoopAlive = true;
    pthread_mutex_unlock(g_initMutex);
    while (true) {
        uv_run(uv_default_loop(), UV_RUN_ONCE);
    }
    return NULL;
}

void initMainThread(void* (*f)(void*))
{
    g_initMutex = new pthread_mutex_t;
    pthread_mutex_init(g_initMutex, NULL);

    pthread_mutex_lock(g_initMutex);
    pthread_t t;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&t, &attr, f, NULL);

    pthread_mutex_lock(g_initMutex);
    pthread_mutex_unlock(g_initMutex);
}

class StarFishController : public Dali::ConnectionTracker {
public:
    StarFishController(StarFishInstance* instance)
        : m_isInit(false)
    {
        m_instance = instance;
    }
    ~StarFishController()
    {
#if defined(STARFISH_TIZEN)
        if (tbm_surface_unmap(m_surface1) != TBM_SURFACE_ERROR_NONE) {
            STARFISH_LOG_INFO("Failed to unmap tbm_surface\n");
        }
        if (tbm_surface_unmap(m_surface2) != TBM_SURFACE_ERROR_NONE) {
            STARFISH_LOG_INFO("Failed to unmap tbm_surface\n");
        }
        if (tbm_surface_destroy(m_surface1) != TBM_SURFACE_ERROR_NONE) {
            STARFISH_LOG_INFO("Failed to destroy tbm_surface\n");
        }
        if (tbm_surface_destroy(m_surface2) != TBM_SURFACE_ERROR_NONE) {
            STARFISH_LOG_INFO("Failed to destroy tbm_surface\n");
        }
#endif
    }

    bool updateBuffer()
    {
#if defined(STARFISH_TIZEN)
        if (TO_STARFISH(m_instance) != nullptr && m_isInit) {
            if (((StarFish::StarFish*)m_instance->m_starfish)) {
                int bufferIdx = ((StarFish::StarFish*)m_instance->m_starfish)
                                    ->frameBufferUpdate();
                if (bufferIdx == 1) {
                    Dali::Any source(m_surface1);
                    m_daliImg_src->SetSource(source);
                    Dali::Stage::GetCurrent().KeepRendering(0.0f);
                } else if (bufferIdx == 2) {
                    Dali::Any source(m_surface2);
                    m_daliImg_src->SetSource(source);
                    Dali::Stage::GetCurrent().KeepRendering(0.0f);
                }
            }
        }
#endif
        return true;
    }

    bool TouchEventHandler(Dali::Actor actor, const Dali::TouchData& data)
    {
        if (TO_STARFISH(m_instance) == nullptr || !m_isInit)
            return true;
        if (!(TO_STARFISH(m_instance)->platformWindow() &&
              TO_STARFISH(m_instance)->platformWindow()->webView() &&
              TO_STARFISH(m_instance)
                  ->platformWindow()
                  ->webView()
                  ->mainBrowsingContext())) {
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
            d->starfish = TO_STARFISH(m_instance);

            Dali::PointState::Type pointState = data.GetState(0);
            const Dali::Vector2& screen = data.GetScreenPosition(0);
            if (pointState == Dali::PointState::DOWN) {
                StarFish::StarFishEnterer enter(TO_STARFISH(m_instance));
                StarFish::MouseData data(
                    StarFish::MouseData::MouseButtonValue::LeftButton,
                    StarFish::MouseData::MouseButtonsValue::LeftButtonDown,
                    screen.x, screen.y, 0);
                d->data = data;
                TO_STARFISH(m_instance)
                    ->messageLoop()
                    ->addIdlerWithNoGCRootingInOtherThread(
                        TO_STARFISH(m_instance)
                            ->platformWindow()
                            ->webView()
                            ->mainBrowsingContext(),
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
                    screen.x, screen.y, 0);
                d->data = data;
                TO_STARFISH(m_instance)
                    ->messageLoop()
                    ->addIdlerWithNoGCRootingInOtherThread(
                        TO_STARFISH(m_instance)
                            ->platformWindow()
                            ->webView()
                            ->mainBrowsingContext(),
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
                StarFish::MouseData data(0, buttons, screen.x, screen.y, 0);

                d->data = data;
                TO_STARFISH(m_instance)
                    ->messageLoop()
                    ->addIdlerWithNoGCRootingInOtherThread(
                        TO_STARFISH(m_instance)
                            ->platformWindow()
                            ->webView()
                            ->mainBrowsingContext(),
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
        if (!(TO_STARFISH(m_instance)->platformWindow() &&
              TO_STARFISH(m_instance)->platformWindow()->webView() &&
              TO_STARFISH(m_instance)
                  ->platformWindow()
                  ->webView()
                  ->mainBrowsingContext())) {
            return true;
        }

        const Dali::Vector2& point = event.GetPoint(0).screen;
        StarFish::StarFishEnterer enter(TO_STARFISH(m_instance));
        unsigned char buttons =
            m_isMouseLbuttonDown
                ? StarFish::MouseData::MouseButtonsValue::LeftButtonDown
                : 0;
        StarFish::MouseData data(0, buttons, point.x, point.y, 0);

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
                TO_STARFISH(m_instance)
                    ->platformWindow()
                    ->webView()
                    ->mainBrowsingContext(),
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
    void KeyEventHandler(const Dali::KeyEvent& event)
    {
        if (TO_STARFISH(m_instance) == nullptr || !m_isInit)
            return;
        if (!(TO_STARFISH(m_instance)->platformWindow() &&
              TO_STARFISH(m_instance)->platformWindow()->webView() &&
              TO_STARFISH(m_instance)
                  ->platformWindow()
                  ->webView()
                  ->mainBrowsingContext())) {
            return;
        }

        StarFish::KeyboardData kdata(StarFish::KeyValue::UnidentifiedKey);
        if (32 < event.keyPressed.c_str()[0] &&
            127 > event.keyPressed.c_str()[0]) {
            kdata = StarFish::KeyboardData(
                (StarFish::KeyValue)event.keyPressed.c_str()[0]);
        } else {
            kdata = DaliEventKeyToKeyboardData(event.keyPressedName.c_str(),
                                               event.keyModifier & 1);
        }
        struct dummy {
            StarFish::StarFish* starfish;
            StarFish::KeyboardData data;
        };
        dummy* d = new dummy;
        d->starfish = TO_STARFISH(m_instance);
        d->data = kdata;
        if (event.state == Dali::KeyEvent::Down) {
            TO_STARFISH(m_instance)
                ->messageLoop()
                ->addIdlerWithNoGCRootingInOtherThread(
                    TO_STARFISH(m_instance)
                        ->platformWindow()
                        ->webView()
                        ->mainBrowsingContext(),
                    [](size_t, void* data) {
                        dummy* d = (dummy*)data;
                        StarFish::StarFish* m_sf = d->starfish;
                        StarFish::KeyboardData keyData = d->data;
                        StarFish::StarFishEnterer enter(m_sf);
                        m_sf->platformWindow()->dispatchKeyEvent(
                            StarFish::PlatformWindow::KeyEventDown, keyData);
                        m_sf->platformWindow()->dispatchKeyEvent(
                            StarFish::PlatformWindow::KeyEventPress, keyData);
                        delete d;
                    },
                    d);
        } else if (event.state == Dali::KeyEvent::Up) {
            TO_STARFISH(m_instance)
                ->messageLoop()
                ->addIdlerWithNoGCRootingInOtherThread(
                    TO_STARFISH(m_instance)
                        ->platformWindow()
                        ->webView()
                        ->mainBrowsingContext(),
                    [](size_t, void* data) {
                        dummy* d = (dummy*)data;
                        StarFish::StarFish* m_sf = d->starfish;
                        StarFish::KeyboardData keyData = d->data;
                        StarFish::StarFishEnterer enter(m_sf);
                        m_sf->platformWindow()->dispatchKeyEvent(
                            StarFish::PlatformWindow::KeyEventUp, keyData);
                        delete d;
                    },
                    d);
        }
    }

    bool m_isInit;
    bool m_isMouseLbuttonDown;
    int m_width;
    int m_height;
    int m_windowX;
    int m_windowY;
    StarFishInstance* m_instance;
    Dali::Toolkit::ImageView m_mainView;
    Dali::Timer m_timer;
#if defined(STARFISH_TIZEN)
    tbm_surface_h m_surface1;
    tbm_surface_h m_surface2;
    tbm_surface_info_s m_surface_info1;
    tbm_surface_info_s m_surface_info2;

    Dali::NativeImageSourcePtr m_daliImg_src;
    Dali::NativeImage m_daliImg;
#endif
};
#define TO_CONTROLLER(instance) ((StarFishController*)instance->m_data)

void starfishCreate_internal(uv_async_t* handle)
{
    int flag = 0;
    int x = 100;
    int y = 100;
    StarFishController* app = (StarFishController*)handle->data;

    StarFish::ScreenInfo info;
    info.rect.setWidth(app->m_width);
    info.rect.setHeight(app->m_height);
    info.availableRect.setWidth(app->m_width);
    info.availableRect.setHeight(app->m_height);

    std::string cacheDir(getenv("HOME"));
    cacheDir += "/Starfish-cache";

    StarFish::StarFish* starFish = new (NoGC) StarFish::StarFish(
        (StarFish::StarFishStartUpFlag)flag, "ko-KR", "Asia/Seoul", nullptr,
        app->m_width, app->m_height, app->m_windowX, app->m_windowY, 1,
        StarFish::String::createASCIIString("sans-serif"), info, "",
        "/tmp/StarFish_Cookies.txt", cacheDir.data(),
        StarFish::String::emptyString, StarFish::String::emptyString);

#if defined(STARFISH_TIZEN)
    starFish->registerFrameBuffer(app->m_surface_info1.planes[0].ptr,
                                  app->m_surface_info2.planes[0].ptr);
#endif
    app->m_instance->m_starfish = starFish;
    app->m_isInit = true;
    pthread_mutex_unlock(g_initMutex);

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

#ifdef STARFISH_TIZEN_TV
extern "C" STARFISH_EXPORT StarFishInstance* starfishCreate(
    void* window, int windowWidth, int windowHeight, int windowX, int windowY,
    const char* locale, const char* timezoneID, float defaultFontSizeMultiplier)
#else
extern "C" STARFISH_EXPORT StarFishInstance* starfishCreate(
    void* window, int windowWidth, int windowHeight, const char* locale,
    const char* timezoneID, const char* defaultFont,
    float defaultFontSizeMultiplier)
#endif
{
#ifndef STARFISH_TIZEN_TV
    int windowX = 0;
    int windowY = 0;
#else
    const char* defaultFont = "sans-serif";
#endif
#if defined(STARFISH_DALI)
    if (needToInitMainThread()) {
        initMainThread(&mainThread);
    }

    int width = windowWidth, height = windowHeight;

    StarFishInstance* instance = new StarFishInstance;
    instance->m_starfish = nullptr;

    StarFishController* starFishControl = new StarFishController(instance);
    instance->m_data = (void*)starFishControl;

#if defined(STARFISH_TIZEN)
    starFishControl->m_surface1 =
        tbm_surface_create(width, height, TBM_FORMAT_ARGB8888);
    starFishControl->m_surface2 =
        tbm_surface_create(width, height, TBM_FORMAT_ARGB8888);

    if (tbm_surface_map(starFishControl->m_surface1,
                        TBM_SURF_OPTION_READ | TBM_SURF_OPTION_WRITE,
                        &starFishControl->m_surface_info1) !=
        TBM_SURFACE_ERROR_NONE) {
        STARFISH_LOG_INFO("Fail to map tbm_surface\n");
    }
    if (tbm_surface_map(starFishControl->m_surface2,
                        TBM_SURF_OPTION_READ | TBM_SURF_OPTION_WRITE,
                        &starFishControl->m_surface_info2) !=
        TBM_SURFACE_ERROR_NONE) {
        STARFISH_LOG_INFO("Fail to map tbm_surface\n");
    }

    Dali::Any source(starFishControl->m_surface1);
    starFishControl->m_daliImg_src = Dali::NativeImageSource::New(source);
    starFishControl->m_daliImg =
        Dali::NativeImage::New(*starFishControl->m_daliImg_src);

    starFishControl->m_mainView =
        Dali::Toolkit::ImageView::New(starFishControl->m_daliImg);
#endif
    starFishControl->m_mainView.SetParentOrigin(Dali::ParentOrigin::TOP_LEFT);
    starFishControl->m_mainView.SetAnchorPoint(Dali::AnchorPoint::TOP_LEFT);
    starFishControl->m_mainView.SetPosition(0, 0);
    Dali::Stage::GetCurrent().Add(starFishControl->m_mainView);

    starFishControl->m_width = width;
    starFishControl->m_height = height;

    pthread_mutex_lock(g_initMutex);

    uv_async_init(uv_default_loop(), &g_launcher_handle,
                  starfishCreate_internal);
    g_launcher_handle.data = starFishControl;
    uv_async_send(&g_launcher_handle);

    Dali::Stage::GetCurrent().GetRootLayer().TouchSignal().Connect(
        starFishControl, &StarFishController::TouchEventHandler);
    Dali::Stage::GetCurrent().GetRootLayer().HoveredSignal().Connect(
        starFishControl, &StarFishController::HoverEventHandler);
    Dali::Stage::GetCurrent().KeyEventSignal().Connect(
        starFishControl, &StarFishController::KeyEventHandler);

    starFishControl->m_timer = Dali::Timer::New(20);
    starFishControl->m_timer.TickSignal().Connect(
        starFishControl, &StarFishController::updateBuffer);
    starFishControl->m_timer.Start();
    pthread_mutex_lock(g_initMutex);
    pthread_mutex_unlock(g_initMutex);

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
        windowWidth, windowHeight, windowX, windowY, defaultFontSizeMultiplier,
        String::fromUTF8(defaultFont), info, "", "", nullptr);
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
        char data[128];
    };
    dummy* d = new dummy;
    d->starfish = TO_STARFISH(instance);
    strcpy(d->data, path);
    TO_STARFISH(instance)
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            nullptr,
            [](size_t, void* data) {
                dummy* d = (dummy*)data;
                StarFish::StarFish* m_sf = d->starfish;
                StarFishEnterer enter(m_sf);
                m_sf->loadHTMLDocument(
                    StarFish::String::fromUTF8(&(d->data)[0]));
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

#if defined(STARFISH_TIZEN_WEARABLE)
extern "C" STARFISH_EXPORT void starfishRemoveForUpdate(
    StarFishInstance* instance)
{
    TO_STARFISH(instance)->enableUpdate();
    starfishRemove(instance);
}
#endif
#if defined(STARFISH_TIZEN_WEARABLE) && defined(TIZEN_DEVICE_API)
typedef int (*sfwebWidgetAPISetContentInfoOfContext_cb)(const void* ctx,
                                                        const void* data);
typedef int (*sfwebWidgetAPIGetContentInfoOfContext_cb)(const void* ctx,
                                                        void** out);
sfwebWidgetAPISetContentInfoOfContext_cb
    webWidgetAPISetContentInfoOfContext_cb = nullptr;
sfwebWidgetAPIGetContentInfoOfContext_cb
    webWidgetAPIGetContentInfoOfContext_cb = nullptr;

extern "C" STARFISH_EXPORT void starfishSetWidgetContext(
    StarFishInstance* instance, const void* widgetContext)
{
    TO_STARFISH(instance)->setWidgetContext(widgetContext);
}

extern "C" STARFISH_EXPORT void registerWebWidgetAPISetContentInfoOfContextCB(
    int (*cb)(const void* ctx, const void* data))
{
    webWidgetAPISetContentInfoOfContext_cb = cb;
}

extern "C" STARFISH_EXPORT void registerWebWidgetAPIGetContentInfoOfContextCB(
    int (*cb)(const void* ctx, void** out))
{
    webWidgetAPIGetContentInfoOfContext_cb = cb;
}

extern "C" STARFISH_EXPORT void starfishWebWidgetAPINotifyReceiveContent(
    StarFishInstance* instance, const void* data)
{
    StarFishEnterer enter(TO_STARFISH(instance));
    DeviceAPI::ExtensionManagerInstance* em = TO_STARFISH(instance)
                                                  ->platformWindow()
                                                  ->webView()
                                                  ->mainBrowsingContext()
                                                  ->scriptBindingInstance()
                                                  ->deviceAPI();
    DeviceAPI::WebWidgetAPIInstance* ww = em->webWidgetAPIInstance();
    if (ww) {
        ww->invokeReceiveContentListener(TO_STARFISH(instance)
                                             ->platformWindow()
                                             ->webView()
                                             ->mainBrowsingContext()
                                             ->scriptBindingInstance()
                                             ->scriptContext(),
                                         data);
    }
}
#endif
