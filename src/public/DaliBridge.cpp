/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_DALI)
#include "StarFishConfig.h"
#include "StarFish.h"
#include "LWEWebView.h"

#include <dali-toolkit/dali-toolkit.h>
#include <dali-toolkit/devel-api/controls/web-view-lite/web-view-lite.h>

#if defined(STARFISH_TIZEN)
#define STARFISH_DALI_TBMSURFACE
#endif

#if defined(STARFISH_DALI_TBMSURFACE)
#include <tbm_surface.h>
#endif

#include <cairo.h>
#include <uv.h>
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Locker.h"
#include "platform/event/PlatformKeyEventData.h"
#include "core/event/KeyBoardEventData.h"

#define TO_HANDLE_DATA(ptr) ((UVAsyncHandleData*)ptr->data)

class EventTracker;

uv_async_t gLauncherHandle;
pthread_mutex_t gMutex;
bool gIsAliveMainLoop = false;
bool gNeedsUpdate = false;
EventTracker* gEventInstance;

struct UVAsyncHandleData {
    std::function<void(void*)> cb;
    void* data;
};

static void* startMainThread(void* data);
struct DaliStarFishBinder {
    void* webContainerInstance;
#if defined(STARFISH_DALI_TBMSURFACE)
    Dali::NativeImageSourcePtr nativeImageSrc;
    tbm_surface_h tbmSurface;
    tbm_surface_info_s tbmSurfaceInfo;
#else
    Dali::BufferImage bufferImage;
#endif
    void* daliControlInstance;
    std::list<size_t> asyncHandlePool;
    int w, h, s;
    DaliStarFishBinder()
        : webContainerInstance(nullptr)
#if defined(STARFISH_DALI_TBMSURFACE)
        , nativeImageSrc(nullptr)
        , tbmSurface(nullptr)
#endif
        , daliControlInstance(nullptr)
        , w(0)
        , h(0)
        , s(0)
    {
    }
};

bool isAliveMainThread()
{
    return gIsAliveMainLoop;
}

bool isNeedsUpdate()
{
    return gNeedsUpdate;
}

void setNeedsUpdate(bool update)
{
    gNeedsUpdate = update;
}

extern "C" __attribute__((visibility("default"))) void initMainThread(
    void* (*f)(void*))
{
    pthread_mutex_init(&gMutex, NULL);

    pthread_mutex_lock(&gMutex);
    pthread_t t;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&t, &attr, f, NULL);
    pthread_mutex_lock(&gMutex);
    pthread_mutex_unlock(&gMutex);
}

extern "C" __attribute__((visibility("default"))) void startMainThreadIfNeeds()
{
    // STARFISH_LOG_INFO("[StarFish] startMainThreadIfNeeds()\n");
    if (!isAliveMainThread()) {
        initMainThread(startMainThread);
    }
}

extern "C" __attribute__((visibility("default"))) void dispatchMouseDownEvent(
    DaliStarFishBinder* binder, float x, float y)
{
    STARFISH_ASSERT(binder);

    UVAsyncHandleData* handle = new UVAsyncHandleData();
    handle->data = binder;
    handle->cb = [x, y](void* data) {
        // STARFISH_LOG_INFO("[StarFish] Callback in
        // dispatchMouseDownEvent()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        ((WebContainer*)binder->webContainerInstance)
            ->DispatchMouseDownEvent(MouseButtonValue::LeftButton,
                                     MouseButtonsValue::LeftButtonDown, x, y);
    };
    binder->asyncHandlePool.push_back((size_t)handle);
    gLauncherHandle.data = binder;
    uv_async_send(&gLauncherHandle);
}

extern "C" __attribute__((visibility("default"))) void dispatchMouseUpEvent(
    DaliStarFishBinder* binder, float x, float y)
{
    STARFISH_ASSERT(binder);

    UVAsyncHandleData* handle = new UVAsyncHandleData();
    handle->data = binder;
    handle->cb = [x, y](void* data) {
        // STARFISH_LOG_INFO("[StarFish] Callback in dispatchMouseUpEvent()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        ((WebContainer*)binder->webContainerInstance)
            ->DispatchMouseUpEvent(MouseButtonValue::NoButton,
                                   MouseButtonsValue::NoButtonDown, x, y);
    };
    binder->asyncHandlePool.push_back((size_t)handle);
    gLauncherHandle.data = binder;
    uv_async_send(&gLauncherHandle);
}

extern "C" __attribute__((visibility("default"))) void dispatchMouseMoveEvent(
    DaliStarFishBinder* binder, float x, float y, bool isLButtonPressed,
    bool isRButtonPressed)
{
    STARFISH_ASSERT(binder);

    UVAsyncHandleData* handle = new UVAsyncHandleData();
    handle->data = binder;
    handle->cb = [x, y, isLButtonPressed](void* data) {
        // STARFISH_LOG_INFO("[StarFish] Callback in
        // dispatchMouseMoveEvent()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        ((WebContainer*)binder->webContainerInstance)
            ->DispatchMouseMoveEvent(
                isLButtonPressed ? MouseButtonValue::LeftButton
                                 : MouseButtonValue::NoButton,
                isLButtonPressed ? MouseButtonsValue::LeftButtonDown
                                 : MouseButtonsValue::NoButtonDown,
                x, y);
    };
    binder->asyncHandlePool.push_back((size_t)handle);
    gLauncherHandle.data = binder;
    uv_async_send(&gLauncherHandle);
}

extern "C" __attribute__((visibility("default"))) void dispatchKeyDownEvent(
    DaliStarFishBinder* binder, KeyValue keyCode)
{
    STARFISH_ASSERT(binder);

    UVAsyncHandleData* handle = new UVAsyncHandleData();
    handle->data = binder;
    handle->cb = [keyCode](void* data) {
        STARFISH_LOG_INFO("[StarFish] Callback in dispatchKeyDownEvent()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        ((WebContainer*)binder->webContainerInstance)
            ->DispatchKeyDownEvent(keyCode);
    };
    binder->asyncHandlePool.push_back((size_t)handle);
    gLauncherHandle.data = binder;
    uv_async_send(&gLauncherHandle);
}

extern "C" __attribute__((visibility("default"))) void dispatchKeyPressEvent(
    DaliStarFishBinder* binder, KeyValue keyCode)
{
    STARFISH_ASSERT(binder);

    UVAsyncHandleData* handle = new UVAsyncHandleData();
    handle->data = binder;
    handle->cb = [keyCode](void* data) {
        // STARFISH_LOG_INFO("[StarFish] Callback in
        // dispatchKeyPressEvent()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        ((WebContainer*)binder->webContainerInstance)
            ->DispatchKeyPressEvent(keyCode);
    };
    binder->asyncHandlePool.push_back((size_t)handle);
    gLauncherHandle.data = binder;
    uv_async_send(&gLauncherHandle);
}

extern "C" __attribute__((visibility("default"))) void dispatchKeyUpEvent(
    DaliStarFishBinder* binder, KeyValue keyCode)
{
    STARFISH_ASSERT(binder);

    UVAsyncHandleData* handle = new UVAsyncHandleData();
    handle->data = binder;
    handle->cb = [keyCode](void* data) {
        // STARFISH_LOG_INFO("[StarFish] Callback in dispatchKeyUpEvent()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        ((WebContainer*)binder->webContainerInstance)
            ->DispatchKeyUpEvent(keyCode);
    };
    binder->asyncHandlePool.push_back((size_t)handle);
    gLauncherHandle.data = binder;
    uv_async_send(&gLauncherHandle);
}

LWE::KeyValue eventKeyToKeyboardData(const char* DALIKeyString,
                                     bool isShiftPressed)
{
    LWE::KeyValue keyValue = LWE::KeyValue::UnidentifiedKey;
    if (strcmp("Left", DALIKeyString) == 0) {
        keyValue = LWE::KeyValue::ArrowLeftKey;
    } else if (strcmp("Right", DALIKeyString) == 0) {
        keyValue = LWE::KeyValue::ArrowRightKey;
    } else if (strcmp("Up", DALIKeyString) == 0) {
        keyValue = LWE::KeyValue::ArrowUpKey;
    } else if (strcmp("Down", DALIKeyString) == 0) {
        keyValue = LWE::KeyValue::ArrowDownKey;
    } else if (strcmp("space", DALIKeyString) == 0) {
        keyValue = LWE::KeyValue::SpaceKey;
    } else if (strcmp("Return", DALIKeyString) == 0) {
        keyValue = LWE::KeyValue::EnterKey;
    } else if (strcmp("BackSpace", DALIKeyString) == 0) {
        keyValue = LWE::KeyValue::BackspaceKey;
    } else if (strcmp("Escape", DALIKeyString) == 0) {
        keyValue = LWE::KeyValue::EscapeKey;
    } else if (strcmp("minus", DALIKeyString) == 0) {
        if (isShiftPressed) {
            keyValue = LWE::KeyValue::MinusMarkKey;
        } else {
            keyValue = LWE::KeyValue::UnderScoreMarkKey;
        }
    } else if (strcmp("equal", DALIKeyString) == 0) {
        if (isShiftPressed) {
            keyValue = LWE::KeyValue::PlusMarkKey;
        } else {
            keyValue = LWE::KeyValue::EqualitySignKey;
        }
    } else if (strcmp("bracketleft", DALIKeyString) == 0) {
        if (isShiftPressed) {
            keyValue = LWE::KeyValue::LeftCurlyBracketMarkKey;
        } else {
            keyValue = LWE::KeyValue::LeftSquareBracketKey;
        }
    } else if (strcmp("bracketright", DALIKeyString) == 0) {
        if (isShiftPressed) {
            keyValue = LWE::KeyValue::RightCurlyBracketMarkKey;
        } else {
            keyValue = LWE::KeyValue::RightSquareBracketKey;
        }
    } else if (strcmp("semicolon", DALIKeyString) == 0) {
        if (isShiftPressed) {
            keyValue = LWE::KeyValue::ColonMarkKey;
        } else {
            keyValue = LWE::KeyValue::SemiColonMarkKey;
        }
    } else if (strcmp("apostrophe", DALIKeyString) == 0) {
        if (isShiftPressed) {
            keyValue = LWE::KeyValue::DoubleQuoteMarkKey;
        } else {
            keyValue = LWE::KeyValue::SingleQuoteMarkKey;
        }
    } else if (strcmp("comma", DALIKeyString) == 0) {
        if (isShiftPressed) {
            keyValue = LWE::KeyValue::LessThanMarkKey;
        } else {
            keyValue = LWE::KeyValue::CommaMarkKey;
        }
    } else if (strcmp("period", DALIKeyString) == 0) {
        if (isShiftPressed) {
            keyValue = LWE::KeyValue::GreaterThanSignKey;
        } else {
            keyValue = LWE::KeyValue::PeriodKey;
        }
    } else if (strcmp("slash", DALIKeyString) == 0) {
        if (isShiftPressed) {
            keyValue = LWE::KeyValue::QuestionMarkKey;
        } else {
            keyValue = LWE::KeyValue::SlashKey;
        }
    } else if (strlen(DALIKeyString) == 1) {
        char ch = DALIKeyString[0];
        if (ch >= '0' && ch <= '9') {
            if (isShiftPressed) {
                switch (ch) {
                case '1':
                    keyValue = LWE::KeyValue::ExclamationMarkKey;
                    break;
                case '2':
                    keyValue = LWE::KeyValue::AtMarkKey;
                    break;
                case '3':
                    keyValue = LWE::KeyValue::SharpMarkKey;
                    break;
                case '4':
                    keyValue = LWE::KeyValue::DollarMarkKey;
                    break;
                case '5':
                    keyValue = LWE::KeyValue::PercentMarkKey;
                    break;
                case '6':
                    keyValue = LWE::KeyValue::CaretMarkKey;
                    break;
                case '7':
                    keyValue = LWE::KeyValue::AmpersandMarkKey;
                    break;
                case '8':
                    keyValue = LWE::KeyValue::AsteriskMarkKey;
                    break;
                case '9':
                    keyValue = LWE::KeyValue::LeftParenthesisMarkKey;
                    break;
                case '0':
                    keyValue = LWE::KeyValue::RightParenthesisMarkKey;
                    break;
                }
            } else {
                keyValue = (LWE::KeyValue)(LWE::KeyValue::Digit0Key + ch - '0');
            }
        } else if (ch >= 'a' && ch <= 'z') {
            int kv = LWE::KeyValue::LowerAKey + ch - 'a';
            if (isShiftPressed) {
                kv -= ('z' - 'a');
                kv -= 7;
            }
            keyValue = (LWE::KeyValue)kv;
        }
    }
#ifdef STARFISH_TIZEN_TV
    if ((strcmp("XF86Red", DALIKeyString) == 0)) {
        keyValue = LWE::KeyValue::TabKey;
    }
#endif
    return keyValue;
}

class EventTracker : public Dali::ConnectionTracker {
public:
    EventTracker(DaliStarFishBinder* binder)
        : mBinder(binder)
        , m_isMouseLbuttonDown(false)
    {
        m_timer = Dali::Timer::New(20);
        m_timer.TickSignal().Connect(this, &EventTracker::updateBuffer);
        m_timer.Start();
    }

    bool touchEventHandler(Dali::Actor actor, const Dali::TouchData& data)
    {
        STARFISH_ASSERT(mBinder);

        size_t pointCount = data.GetPointCount();
        if (pointCount == 1) {
            // Single touch event
            Dali::PointState::Type pointState = data.GetState(0);
            const Dali::Vector2& screen = data.GetLocalPosition(0);

            if (pointState == Dali::PointState::DOWN) {
                dispatchMouseDownEvent(mBinder, screen.x, screen.y);
                m_isMouseLbuttonDown = true;
            } else if (pointState == Dali::PointState::UP) {
                dispatchMouseUpEvent(mBinder, screen.x, screen.y);
                m_isMouseLbuttonDown = false;
            } else {
                dispatchMouseMoveEvent(mBinder, screen.x, screen.y,
                                       m_isMouseLbuttonDown, false);
            }
        }
        return true;
    }

    void keyEventHandler(const Dali::KeyEvent& event)
    {
        LWE::KeyValue keyValue = LWE::KeyValue::UnidentifiedKey;
        if (32 < event.keyPressed.c_str()[0] &&
            127 > event.keyPressed.c_str()[0]) {
            keyValue = StarFish::PlatformKeyEventData(
                           (LWE::KeyValue)event.keyPressed.c_str()[0])
                           .keyValue();
        } else {
            keyValue = eventKeyToKeyboardData(event.keyPressedName.c_str(),
                                              event.keyModifier & 1);
        }
        if (event.state == Dali::KeyEvent::Down) {
            dispatchKeyDownEvent(mBinder, keyValue);
            dispatchKeyPressEvent(mBinder, keyValue);
        } else if (event.state == Dali::KeyEvent::Up) {
            dispatchKeyUpEvent(mBinder, keyValue);
        }
    }

    bool updateBuffer()
    {
        STARFISH_ASSERT(mBinder);
        if (isNeedsUpdate()) {
#if defined(STARFISH_DALI_TBMSURFACE)
            Dali::Stage::GetCurrent().KeepRendering(0.01f);
#else
            mBinder->bufferImage.Update();
#endif
            setNeedsUpdate(false);
        }

        return true;
    }

private:
    DaliStarFishBinder* mBinder;
    bool m_isMouseLbuttonDown;
    Dali::Timer m_timer;
};

extern "C" __attribute__((visibility("default"))) void createInstance(
    DaliStarFishBinder* binder)
{
    STARFISH_ASSERT(binder);
    Dali::Toolkit::WebViewLite* view =
        (Dali::Toolkit::WebViewLite*)binder->daliControlInstance;

    gEventInstance = new EventTracker(binder);
    view->TouchSignal().Connect(gEventInstance,
                                &EventTracker::touchEventHandler);
    Dali::Stage::GetCurrent().KeyEventSignal().Connect(
        gEventInstance, &EventTracker::keyEventHandler);

#if defined(STARFISH_DALI_TBMSURFACE)
    STARFISH_ASSERT(binder->nativeImageSrc);
    STARFISH_LOG_INFO(
        "[StarFish] createInstance() [binder->nativeImageSrc : %p]\n",
        binder->nativeImageSrc);
    //    view->SetImage(binder->nativeImage);
    binder->nativeImageSrc->SetSource(binder->tbmSurface);
#else
    STARFISH_ASSERT(binder->bufferImage);
    STARFISH_LOG_INFO(
        "[StarFish] createInstance() [binder->bufferImage : %p]\n",
        binder->bufferImage);
    view->SetImage(binder->bufferImage);
#endif

    UVAsyncHandleData* handle = new UVAsyncHandleData();
    handle->data = binder;
    handle->cb = [](void* data) {
        STARFISH_LOG_INFO("[StarFish] Callback in createInstance()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        binder->webContainerInstance = WebContainer::Create(
            malloc(binder->w * binder->h * sizeof(uint32_t)), binder->w,
            binder->h, binder->s, 1.0);
    };
    binder->asyncHandlePool.push_back((size_t)handle);
    gLauncherHandle.data = binder;
    uv_async_send(&gLauncherHandle);
}

extern "C" __attribute__((visibility("default"))) void loadURL(
    DaliStarFishBinder* binder, const std::string& url)
{
    STARFISH_ASSERT(binder);
    STARFISH_LOG_INFO("[StarFish] loadURL()\n");

    UVAsyncHandleData* handle = new UVAsyncHandleData();
    handle->data = binder;
    handle->cb = [url](void* data) {
        STARFISH_LOG_INFO("[StarFish] Callback in loadURL()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        ((WebContainer*)binder->webContainerInstance)->LoadURL(url);
        ((WebContainer*)binder->webContainerInstance)
            ->RegisterOnRenderedHandler(
                [binder](LWE::WebContainer* c, void* buf) {
#if defined(STARFISH_DALI_TBMSURFACE)
                    // STARFISH_LOG_INFO("[StarFish] Callback of Callback in
                    // RegisterOnRenderedHandler()
                    // [binder->tbmSurfaceInfo.planes[0].ptr:%p]\n",
                    // binder->tbmSurfaceInfo.planes[0].ptr);
                    memcpy(binder->tbmSurfaceInfo.planes[0].ptr, buf,
                           binder->w * binder->h * sizeof(uint32_t));
#else
                    // STARFISH_LOG_INFO("[StarFish] Callback of Callback in
                    // RegisterOnRenderedHandler() [Dali BufImg:%p]\n",
                    // binder->bufferImage);
                    memcpy(binder->bufferImage.GetBuffer(), buf,
                           binder->w * binder->h * sizeof(uint32_t));
#endif
                    gNeedsUpdate = true;
                });
    };
    binder->asyncHandlePool.push_back((size_t)handle);
    gLauncherHandle.data = binder;
    uv_async_send(&gLauncherHandle);
}

extern "C" __attribute__((visibility("default"))) void destory(
    DaliStarFishBinder* binder)
{
    STARFISH_ASSERT(binder);

    UVAsyncHandleData* handle = new UVAsyncHandleData();
    handle->data = binder;
    handle->cb = [](void* data) {
        // STARFISH_LOG_INFO("[StarFish] Callback in destory()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        ((WebContainer*)binder->webContainerInstance)->Destroy();

        auto iter = binder->asyncHandlePool.begin();
        for (; iter != binder->asyncHandlePool.end();) {
            iter = binder->asyncHandlePool.erase(iter);
        }
        binder->asyncHandlePool.clear();

        free(gEventInstance);
    };
    binder->asyncHandlePool.push_back((size_t)handle);
    gLauncherHandle.data = binder;
    uv_async_send(&gLauncherHandle);
}

extern "C" __attribute__((visibility("default"))) void
registerOnRenderedHandler(DaliStarFishBinder* binder)
{
    STARFISH_ASSERT(binder);

    UVAsyncHandleData* handle = new UVAsyncHandleData();
    handle->data = binder;
    handle->cb = [&](void* data) {
        // STARFISH_LOG_INFO("[StarFish] Callback in
        // registerOnRenderedHandler()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        ((WebContainer*)binder->webContainerInstance)
            ->RegisterOnRenderedHandler(
                [binder](LWE::WebContainer* c, void* buf) {
// STARFISH_LOG_INFO("[StarFish] Callback in
// RegisterOnRenderedHandler() [Dali BufImg:%p]\n",
// binder->bufferImage);
#if defined(STARFISH_DALI_TBMSURFACE)
                    memcpy(binder->tbmSurfaceInfo.planes[0].ptr, buf,
                           binder->w * binder->h * sizeof(uint32_t));
#else
                    memcpy(binder->bufferImage.GetBuffer(), buf,
                           binder->w * binder->h * sizeof(uint32_t));
#endif
                    gNeedsUpdate = true;
                });
    };
    binder->asyncHandlePool.push_back((size_t)handle);
    gLauncherHandle.data = binder;
    uv_async_send(&gLauncherHandle);
}

static void* startMainThread(void* data)
{
    uv_async_init(uv_default_loop(), &gLauncherHandle, [](uv_async_t* handle) {
        // STARFISH_LOG_INFO("[StarFish] uv_async_init() in
        // startMainThread()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)handle->data;
        while (!binder->asyncHandlePool.empty()) {
            UVAsyncHandleData* handleData = nullptr;
            {
                pthread_mutex_lock(&gMutex);
                handleData =
                    (UVAsyncHandleData*)*binder->asyncHandlePool.begin();
                binder->asyncHandlePool.erase(binder->asyncHandlePool.begin());
                pthread_mutex_unlock(&gMutex);
            }

            if (handleData) {
                handleData->cb(handleData->data);
                delete handleData;
            }
        }
    });

    gIsAliveMainLoop = true;
    pthread_mutex_unlock(&gMutex);
    while (true) {
        uv_run(uv_default_loop(), UV_RUN_ONCE);
    }
    return NULL;
}

/////////////////////////////////////////////
#endif
