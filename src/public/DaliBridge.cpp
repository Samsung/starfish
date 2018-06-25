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
int gDaliNumber = false;

struct UVAsyncHandleData {
    std::function<void(void*)> cb;
    void* data;
};

static void* startMainThread(void* data);
struct DaliStarFishBinder {
    void* lweInstance;
#if defined(STARFISH_DALI_TBMSURFACE)
    Dali::NativeImageSourcePtr nativeImageSrc;
    tbm_surface_h tbmSurface;
    tbm_surface_info_s tbmSurfaceInfo;
#else
    Dali::BufferImage bufferImage;
#endif
    void* daliInstance;
    void* buffer;
    std::list<size_t> asyncHandlePool;
    int w, h, s;
    bool canGoBack, canGoForward;
    bool isRunning;
    bool isFirstTime;
    std::function<void(LWE::WebContainer*, LWE::ResourceError)> onReceivedError;
    std::function<void(LWE::WebContainer*, const std::string&)>
        onPageFinishedHandler;
    std::function<void(LWE::WebContainer*, const std::string&)>
        onPageStartedHandler;
    std::function<void(LWE::WebContainer*, const std::string&)>
        onLoadResourceHandler;
    DaliStarFishBinder()
        : lweInstance(nullptr)
#if defined(STARFISH_DALI_TBMSURFACE)
        , nativeImageSrc(nullptr)
        , tbmSurface(nullptr)
#endif
        , daliInstance(nullptr)
        , buffer(nullptr)
        , w(0)
        , h(0)
        , s(0)
        , canGoBack(false)
        , canGoForward(false)
        , isRunning(false)
        , isFirstTime(true)
    {
    }
};

#define TO_WEBCONTAINER(ptr) ((WebContainer*)ptr->lweInstance)

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

void sendAsyncHandle(DaliStarFishBinder* binder, std::function<void(void*)> cb)
{
    UVAsyncHandleData* handle = new UVAsyncHandleData();
    handle->data = binder;
    handle->cb = cb;
    pthread_mutex_lock(&gMutex);
    binder->asyncHandlePool.push_back((size_t)handle);
    pthread_mutex_unlock(&gMutex);
    gLauncherHandle.data = binder;
    uv_async_send(&gLauncherHandle);
}

extern "C" __attribute__((visibility("default"))) void initMainThread(
    void* (*f)(void*), pthread_t& t)
{
    pthread_mutex_init(&gMutex, NULL);

    pthread_mutex_lock(&gMutex);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&t, &attr, f, NULL);
    pthread_mutex_lock(&gMutex);
    pthread_mutex_unlock(&gMutex);
}

extern "C" __attribute__((visibility("default"))) void startMainThreadIfNeeds(
    pthread_t& t)
{
    if (!isAliveMainThread()) {
        initMainThread(startMainThread, t);
    }
}

extern "C" __attribute__((visibility("default"))) void dispatchMouseDownEvent(
    DaliStarFishBinder* binder, float x, float y)
{
    STARFISH_ASSERT(binder);
    if (!binder->isRunning) {
        return;
    }

    auto cb = [x, y](void* data) {
        // STARFISH_LOG_INFO("[StarFish] dispatchMouseDownEvent()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        TO_WEBCONTAINER(binder)
            ->DispatchMouseDownEvent(MouseButtonValue::LeftButton,
                                     MouseButtonsValue::LeftButtonDown, x, y);
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void dispatchMouseUpEvent(
    DaliStarFishBinder* binder, float x, float y)
{
    STARFISH_ASSERT(binder);
    if (!binder->isRunning) {
        return;
    }

    auto cb = [x, y](void* data) {
        // STARFISH_LOG_INFO("[StarFish] dispatchMouseUpEvent()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        TO_WEBCONTAINER(binder)
            ->DispatchMouseUpEvent(MouseButtonValue::NoButton,
                                   MouseButtonsValue::NoButtonDown, x, y);
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void dispatchMouseMoveEvent(
    DaliStarFishBinder* binder, float x, float y, bool isLButtonPressed,
    bool isRButtonPressed)
{
    STARFISH_ASSERT(binder);
    if (!binder->isRunning) {
        return;
    }

    auto cb = [x, y, isLButtonPressed](void* data) {
        // STARFISH_LOG_INFO("[StarFish] dispatchMouseMoveEvent()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        TO_WEBCONTAINER(binder)
            ->DispatchMouseMoveEvent(
                isLButtonPressed ? MouseButtonValue::LeftButton
                                 : MouseButtonValue::NoButton,
                isLButtonPressed ? MouseButtonsValue::LeftButtonDown
                                 : MouseButtonsValue::NoButtonDown,
                x, y);
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void dispatchKeyDownEvent(
    DaliStarFishBinder* binder, KeyValue keyCode)
{
    STARFISH_ASSERT(binder);
    if (!binder->isRunning) {
        return;
    }

    auto cb = [keyCode](void* data) {
        // STARFISH_LOG_INFO("[StarFish] dispatchKeyDownEvent()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        TO_WEBCONTAINER(binder)->DispatchKeyDownEvent(keyCode);
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void dispatchKeyPressEvent(
    DaliStarFishBinder* binder, KeyValue keyCode)
{
    STARFISH_ASSERT(binder);
    if (!binder->isRunning) {
        return;
    }

    auto cb = [keyCode](void* data) {
        // STARFISH_LOG_INFO("[StarFish] dispatchKeyPressEvent()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        TO_WEBCONTAINER(binder)->DispatchKeyPressEvent(keyCode);
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void dispatchKeyUpEvent(
    DaliStarFishBinder* binder, KeyValue keyCode)
{
    STARFISH_ASSERT(binder);
    if (!binder->isRunning) {
        return;
    }

    auto cb = [keyCode](void* data) {
        // STARFISH_LOG_INFO("[StarFish] dispatchKeyUpEvent()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        TO_WEBCONTAINER(binder)->DispatchKeyUpEvent(keyCode);
    };
    sendAsyncHandle(binder, cb);
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

    ~EventTracker()
    {
        m_timer.TickSignal().Disconnect(gEventInstance,
                                        &EventTracker::updateBuffer);
        m_timer.Stop();
    }

    bool touchEventHandler(Dali::Actor actor, const Dali::TouchData& data)
    {
        STARFISH_LOG_INFO("[StarFish] touchEventHandler()\n");
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

    bool keyEventHandler(Dali::Toolkit::Control control,
                         const Dali::KeyEvent& event)
    {
        STARFISH_LOG_INFO("[StarFish] keyEventHandler()\n");
        LWE::KeyValue keyValue = LWE::KeyValue::UnidentifiedKey;
        if (32 < event.keyPressed.c_str()[0] &&
            127 > event.keyPressed.c_str()[0]) {
            keyValue = (LWE::KeyValue)event.keyPressed.c_str()[0];
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

        return true;
    }

    bool updateBuffer()
    {
        if (!mBinder || mBinder->isRunning == false) {
            return false;
        }

        if (isNeedsUpdate()) {
#if defined(STARFISH_DALI_TBMSURFACE)
            Dali::Stage::GetCurrent().KeepRendering(0.01f);
#else
            if (!mBinder->bufferImage) {
                return false;
            }
            mBinder->bufferImage.Update();
#endif
            setNeedsUpdate(false);
        }
        return true;
    }

    Dali::Timer* timer()
    {
        return &m_timer;
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
    gDaliNumber++;
    Dali::Toolkit::WebViewLite* view =
        (Dali::Toolkit::WebViewLite*)binder->daliInstance;

    if (gEventInstance) {
        free(gEventInstance);
        gEventInstance = nullptr;
    }
    gEventInstance = new EventTracker(binder);
    view->TouchSignal().Connect(gEventInstance,
                                &EventTracker::touchEventHandler);
    view->KeyEventSignal().Connect(gEventInstance,
                                   &EventTracker::keyEventHandler);

#if defined(STARFISH_DALI_TBMSURFACE)
    STARFISH_ASSERT(binder->nativeImageSrc);
    STARFISH_LOG_INFO(
        "[StarFish] createInstance() [binder->nativeImageSrc : %p]\n",
        binder->nativeImageSrc);
    binder->nativeImageSrc->SetSource(binder->tbmSurface);
#else
    STARFISH_ASSERT(binder->bufferImage);
    STARFISH_LOG_INFO(
        "[StarFish] createInstance() [binder->bufferImage : %p]\n",
        binder->bufferImage);
    view->SetImage(binder->bufferImage);
#endif

    auto cb = [](void* data) {
        STARFISH_LOG_INFO("[StarFish] createInstance()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;

        if (binder->buffer) {
            free(binder->buffer);
            binder->buffer = nullptr;
        }
        binder->buffer = malloc(binder->w * binder->h * sizeof(uint32_t));
        binder->lweInstance = WebContainer::Create(binder->buffer, binder->w,
                                                   binder->h, binder->s, 1.0);
        TO_WEBCONTAINER(binder)
            ->RegisterOnRenderedHandler(
                [binder](LWE::WebContainer* c, void* buf) {
                    int w = TO_WEBCONTAINER(binder)->width();
                    int h = TO_WEBCONTAINER(binder)->height();
                    if (binder->w != w || binder->h != h) {
                        return;
                    }
#if defined(STARFISH_DALI_TBMSURFACE)
                    memcpy(binder->tbmSurfaceInfo.planes[0].ptr, buf,
                           binder->w * binder->h * sizeof(uint32_t));
#else
                    memcpy(binder->bufferImage.GetBuffer(), buf,
                           binder->w * binder->h * sizeof(uint32_t));
#endif
                    gNeedsUpdate = true;
                });
        TO_WEBCONTAINER(binder)
            ->RegisterOnReceivedErrorHandler(
                [binder](LWE::WebContainer* container,
                         LWE::ResourceError error) -> void {
                    binder->canGoBack = container->CanGoBack();
                    binder->canGoForward = container->CanGoForward();
                    binder->onReceivedError(container, error);
                });
        TO_WEBCONTAINER(binder)
            ->RegisterOnPageStartedHandler(
                [binder](LWE::WebContainer* container,
                         const std::string& url) -> void {
                    binder->canGoBack = container->CanGoBack();
                    binder->canGoForward = container->CanGoForward();
                    binder->onPageStartedHandler(container, url);
                });
        TO_WEBCONTAINER(binder)
            ->RegisterOnPageFinishedHandler(
                [binder](LWE::WebContainer* container,
                         const std::string& url) -> void {
                    binder->canGoBack = container->CanGoBack();
                    binder->canGoForward = container->CanGoForward();
                    binder->onPageFinishedHandler(container, url);
                });
        TO_WEBCONTAINER(binder)
            ->RegisterOnLoadResourceHandler(
                [binder](LWE::WebContainer* container,
                         const std::string& url) -> void {
                    binder->canGoBack = container->CanGoBack();
                    binder->canGoForward = container->CanGoForward();
                    binder->onLoadResourceHandler(container, url);
                });
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void loadURL(
    DaliStarFishBinder* binder, const std::string& url)
{
    STARFISH_ASSERT(binder);
    auto cb = [url](void* data) {
        STARFISH_LOG_INFO("[StarFish] loadURL()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        TO_WEBCONTAINER(binder)->LoadURL(url);
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void setSize(
    DaliStarFishBinder* binder)
{
    STARFISH_ASSERT(binder);
    Dali::Toolkit::WebViewLite* view =
        (Dali::Toolkit::WebViewLite*)binder->daliInstance;
#if defined(STARFISH_DALI_TBMSURFACE)
    STARFISH_ASSERT(binder->nativeImageSrc);
    STARFISH_LOG_INFO("[StarFish] setSize() [binder->nativeImageSrc : %p]\n",
                      binder->nativeImageSrc);
    binder->nativeImageSrc->SetSource(binder->tbmSurface);
#else
    STARFISH_ASSERT(binder->bufferImage);
    STARFISH_LOG_INFO("[StarFish] setSize() [binder->bufferImage : %p]\n",
                      binder->bufferImage);
    view->SetImage(binder->bufferImage);
#endif

    auto cb = [](void* data) {
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;

        if (binder->buffer) {
            free(binder->buffer);
            binder->buffer = nullptr;
        }
        binder->buffer = malloc(binder->w * binder->h * sizeof(uint32_t));
        STARFISH_LOG_INFO(
            "[StarFish] setSize() [binder->buffer:%p][w:%d][h:%d]\n",
            binder->buffer, binder->w, binder->h);
        TO_WEBCONTAINER(binder)
            ->UpdateBuffer(binder->buffer, binder->w, binder->h, binder->s);
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void loadData(
    DaliStarFishBinder* binder, const std::string& d)
{
    STARFISH_ASSERT(binder);
    auto cb = [d](void* data) {
        STARFISH_LOG_INFO("[StarFish] loadData()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        TO_WEBCONTAINER(binder)->LoadData(d);
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void reload(
    DaliStarFishBinder* binder)
{
    STARFISH_ASSERT(binder);
    auto cb = [](void* data) {
        STARFISH_LOG_INFO("[StarFish] reload()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        TO_WEBCONTAINER(binder)->Reload();
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void stopLoading(
    DaliStarFishBinder* binder)
{
    STARFISH_ASSERT(binder);
    auto cb = [](void* data) {
        STARFISH_LOG_INFO("[StarFish] stopLoading()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        TO_WEBCONTAINER(binder)->StopLoading();
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void goBack(
    DaliStarFishBinder* binder)
{
    STARFISH_ASSERT(binder);
    auto cb = [](void* data) {
        STARFISH_LOG_INFO("[StarFish] goBack()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        TO_WEBCONTAINER(binder)->GoBack();
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void goForward(
    DaliStarFishBinder* binder)
{
    STARFISH_ASSERT(binder);
    auto cb = [](void* data) {
        STARFISH_LOG_INFO("[StarFish] goForward()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        TO_WEBCONTAINER(binder)->GoForward();
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void addJavaScriptInterface(
    DaliStarFishBinder* binder, const std::string& exposedObjectName,
    const std::string& jsFunctionName,
    std::function<std::string(const std::string&)> callback)
{
    STARFISH_ASSERT(binder);
    auto cb = [exposedObjectName, jsFunctionName, callback](void* data) {
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        TO_WEBCONTAINER(binder)
            ->AddJavaScriptInterface(exposedObjectName, jsFunctionName,
                                     callback);
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void evaluateJavaScript(
    DaliStarFishBinder* binder, const std::string& script)
{
    STARFISH_ASSERT(binder);
    auto cb = [script](void* data) {
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        std::string ret = TO_WEBCONTAINER(binder)->EvaluateJavaScript(script);
        STARFISH_LOG_INFO("[StarFish] evaluateJavaScript() returns [%s]\n",
                          ret.c_str());
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void clearHistory(
    DaliStarFishBinder* binder)
{
    STARFISH_ASSERT(binder);
    auto cb = [](void* data) {
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        TO_WEBCONTAINER(binder)->ClearHistory();
        binder->canGoBack = TO_WEBCONTAINER(binder)->CanGoBack();
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void destroy(
    DaliStarFishBinder* binder)
{
    STARFISH_ASSERT(binder);
    auto cb = [](void* data) {
        STARFISH_LOG_INFO("[StarFish] destory() 1\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;

        TO_WEBCONTAINER(binder)->Destroy();

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

        gDaliNumber--;

        GC_gcollect_and_unmap();
        GC_gcollect_and_unmap();
        GC_gcollect_and_unmap();
        GC_gcollect_and_unmap();
        GC_gcollect_and_unmap();
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void
removeJavascriptInterface(DaliStarFishBinder* binder,
                          const std::string& exposedObjectName,
                          const std::string& jsFunctionName)
{
    STARFISH_ASSERT(binder);
    auto cb = [exposedObjectName, jsFunctionName](void* data) {
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        TO_WEBCONTAINER(binder)
            ->RemoveJavascriptInterface(exposedObjectName, jsFunctionName);
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void clearCache(
    DaliStarFishBinder* binder)
{
    STARFISH_ASSERT(binder);
    auto cb = [](void* data) {
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        TO_WEBCONTAINER(binder)->ClearCache();
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void
registerOnReceivedErrorHandler(
    DaliStarFishBinder* binder,
    const std::function<void(LWE::WebContainer*, LWE::ResourceError)>& callback)
{
    STARFISH_ASSERT(binder);
    auto cb = [callback](void* data) {
        STARFISH_LOG_INFO("[StarFish] registerOnReceivedErrorHandler()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        TO_WEBCONTAINER(binder)->RegisterOnReceivedErrorHandler(callback);
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void
registerOnPageFinishedHandler(
    DaliStarFishBinder* binder,
    const std::function<void(LWE::WebContainer*, const std::string&)>& callback)
{
    STARFISH_ASSERT(binder);
    auto cb = [callback](void* data) {
        STARFISH_LOG_INFO("[StarFish] registerOnPageFinishedHandler()\n");
        DaliStarFishBinder* binder = (DaliStarFishBinder*)data;
        TO_WEBCONTAINER(binder)->RegisterOnPageFinishedHandler(callback);
    };
    sendAsyncHandle(binder, cb);
}

void callEmptyAsyncHandle(DaliStarFishBinder* binder)
{
    STARFISH_ASSERT(binder);
    auto cb = [](void* data) {
        STARFISH_LOG_INFO("[StarFish] callEmptyAsyncHandle()\n");
    };
    sendAsyncHandle(binder, cb);
}

extern "C" __attribute__((visibility("default"))) void stopLoop(
    DaliStarFishBinder* binder)
{
    gDaliNumber = -1;
    callEmptyAsyncHandle(binder);
}

static void* startMainThread(void* data)
{
    uv_async_init(uv_default_loop(), &gLauncherHandle, [](uv_async_t* handle) {
        STARFISH_LOG_INFO("[StarFish] uv_async_init() in startMainThread()\n");
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
        if (gDaliNumber < 0) {
            break;
        }
    }

    STARFISH_LOG_INFO("[StarFish] uv_run() after\n");

    return NULL;
}

/////////////////////////////////////////////
#endif
