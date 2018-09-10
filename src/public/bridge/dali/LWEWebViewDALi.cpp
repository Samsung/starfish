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

#include "StarFishConfig.h"
#include "LWEWebView.h"

#if defined(PORT_WEBVIEW_BRIDGE_DALI)
#ifdef STARFISH_TIZEN
#define STARFISH_DALI_TBMSURFACE
#endif
#if defined(STARFISH_DALI_TBMSURFACE)
#include <tbm_surface.h>
#endif

#include <dali-toolkit/dali-toolkit.h>
#include <uv.h>

using namespace Dali;

#define TO_CONTAINER(ptr) (((DALiShellController*)ptr)->mWebContainer)

static uv_async_t gLauncherHandle;
static pthread_mutex_t gMutex;
static bool gIsAliveMainLoop = false;
static int gDALiNumber = 0;

class Locker {
public:
    Locker(pthread_mutex_t& lock)
        : m_lock(lock)
    {
        pthread_mutex_lock(&m_lock);
    }

    ~Locker()
    {
        pthread_mutex_unlock(&m_lock);
    }

protected:
    pthread_mutex_t m_lock;
};

struct UVAsyncHandleData {
    std::function<void(void*)> cb;
    void* data;
};

static void* startMainThread(void* data);
static bool isAliveMainThread()
{
    return gIsAliveMainLoop;
}

static void initMainThread(void* (*f)(void*), pthread_t& t)
{
    pthread_mutex_init(&gMutex, NULL);

    pthread_mutex_lock(&gMutex);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&t, &attr, f, NULL);
    pthread_mutex_lock(&gMutex);
    pthread_mutex_unlock(&gMutex);
}

class DALiShellController;
void sendAsyncHandle(DALiShellController* controller,
                     std::function<void(void*)> cb);
static void* startMainThread(void* data);

class DALiShellController : public ConnectionTracker {
public:
    DALiShellController(Application& application, pthread_t threadHandle,
                        int width, int height, const std::string& url)
        : mApplication(application)
        , mThreadHandle(threadHandle)
        , mIsMouseLbuttonDown(false)
        , misNeedsUpdate(false)
        , mURL(url)
        , mOutputWidth(width)
        , mOutputHeight(height)
        , mOutputStride(width * sizeof(uint32_t))
        , mOutputBuffer((uint8_t*)malloc(width * height * sizeof(uint32_t)))
        , mCanGoBack(false)
        , mCanGoForward(false)
        , mIsRunning(false)
        , mWebContainer(nullptr)
#if defined(STARFISH_DALI_TBMSURFACE)
        , mTbmSurface(NULL)
#endif
    {
        mApplication.InitSignal().Connect(this, &DALiShellController::Create);
    }
    ~DALiShellController()
    {
        STARFISH_LOG_INFO("[DALi Shell] ~DALiShellController()\n");
        mTimer.TickSignal().Disconnect(this,
                                       &DALiShellController::updateBuffer);
        mTimer.Stop();
    }

    void startMainThreadIfNeeds(pthread_t& t)
    {
        if (!isAliveMainThread()) {
            initMainThread(startMainThread, t);
        }
    }

    void InnerCreate(Application& application);
    void Create(Application& application);

    Application& mApplication;
    Dali::Toolkit::ImageView mImageView;
    pthread_t mThreadHandle;

    bool mIsMouseLbuttonDown;
    bool misNeedsUpdate;
    Dali::Timer mTimer;

    std::string mURL;
    size_t mOutputWidth;
    size_t mOutputHeight;
    size_t mOutputStride;
    uint8_t* mOutputBuffer;
    bool mCanGoBack, mCanGoForward;
    bool mIsRunning;

    LWE::WebContainer* mWebContainer;
    std::list<size_t> mAsyncHandlePool;

#if defined(STARFISH_DALI_TBMSURFACE)
    Dali::NativeImageSourcePtr mNativeImageSrc;
    Dali::NativeImage mNativeImage;
    tbm_surface_h mTbmSurface;
#else
    Dali::BufferImage mBufferImage;
#endif
    std::function<void(LWE::WebContainer*,
                       const LWE::WebContainer::RenderResult&)>
        onRenderedHandler;
    std::function<void(LWE::WebContainer*, LWE::ResourceError)> onReceivedError;
    std::function<void(LWE::WebContainer*, const std::string&)>
        onPageFinishedHandler;
    std::function<void(LWE::WebContainer*, const std::string&)>
        onPageStartedHandler;
    std::function<void(LWE::WebContainer*, const std::string&)>
        onLoadResourceHandler;

private:
    void onKeyEvent(const Dali::KeyEvent& event);

    bool touchEventHandler(Dali::Actor actor, const Dali::TouchData& data)
    {
        // STARFISH_LOG_INFO("[DALi Shell] touchEventHandler()\n");
        size_t pointCount = data.GetPointCount();
        if (pointCount == 1) {
            // Single touch event
            Dali::PointState::Type pointState = data.GetState(0);
            const Dali::Vector2& screen = data.GetLocalPosition(0);

            if (pointState == Dali::PointState::DOWN) {
                dispatchMouseDownEvent(screen.x, screen.y);
                mIsMouseLbuttonDown = true;
            } else if (pointState == Dali::PointState::UP) {
                dispatchMouseUpEvent(screen.x, screen.y);
                mIsMouseLbuttonDown = false;
            } else {
                dispatchMouseMoveEvent(screen.x, screen.y, mIsMouseLbuttonDown,
                                       false);
            }
        }
        return true;
    }

    LWE::KeyValue keyStringToKeyValue(const char* DALIKeyString,
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
        } else if (strcmp("Tab", DALIKeyString) == 0) {
            return LWE::KeyValue::TabKey;
        } else if (strcmp("BackSpace", DALIKeyString) == 0) {
            keyValue = LWE::KeyValue::BackspaceKey;
        } else if (strcmp("Escape", DALIKeyString) == 0) {
            keyValue = LWE::KeyValue::EscapeKey;
        } else if (strcmp("Delete", DALIKeyString) == 0) {
            return LWE::KeyValue::DeleteKey;
        } else if (strcmp("at", DALIKeyString) == 0) {
            return LWE::KeyValue::AtMarkKey;
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
                    keyValue =
                        (LWE::KeyValue)(LWE::KeyValue::Digit0Key + ch - '0');
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

public:
    void keyEventHandler(const Dali::KeyEvent& event)
    {
        // STARFISH_LOG_INFO("[DALi Shell] keyEventHandler()\n");
        LWE::KeyValue keyValue = LWE::KeyValue::UnidentifiedKey;
        if (32 < event.keyPressed.c_str()[0] &&
            127 > event.keyPressed.c_str()[0]) {
            keyValue = (LWE::KeyValue)event.keyPressed.c_str()[0];
        } else {
            keyValue = keyStringToKeyValue(event.keyPressedName.c_str(),
                                           event.keyModifier & 1);
        }
        if (event.state == Dali::KeyEvent::Down) {
            dispatchKeyDownEvent(keyValue);
            dispatchKeyPressEvent(keyValue);
        } else if (event.state == Dali::KeyEvent::Up) {
            dispatchKeyUpEvent(keyValue);
        }
    }

    bool updateBuffer()
    {
        if (mIsRunning == false) {
            return true;
        }
        if (misNeedsUpdate) {
            Locker l(gMutex);
#if defined(STARFISH_DALI_TBMSURFACE)
            Dali::Stage::GetCurrent().KeepRendering(0.005f);
#else
            if (!mBufferImage) {
                return false;
            }
            mBufferImage.Update();
#endif
            misNeedsUpdate = false;
        }
        return true;
    }

    void dispatchMouseDownEvent(float x, float y)
    {
        STARFISH_ASSERT(mWebContainer);
        if (!mIsRunning) {
            return;
        }

        auto cb = [x, y](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell] dispatchMouseDownEvent()\n");
            TO_CONTAINER(data)
                ->DispatchMouseDownEvent(LWE::MouseButtonValue::LeftButton,
                                         LWE::MouseButtonsValue::LeftButtonDown,
                                         x, y);
        };
        sendAsyncHandle(this, cb);
    }

    void dispatchMouseUpEvent(float x, float y)
    {
        STARFISH_ASSERT(mWebContainer);
        if (!mIsRunning) {
            return;
        }

        auto cb = [x, y](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell] dispatchMouseUpEvent()\n");
            TO_CONTAINER(data)
                ->DispatchMouseUpEvent(LWE::MouseButtonValue::NoButton,
                                       LWE::MouseButtonsValue::NoButtonDown, x,
                                       y);
        };
        sendAsyncHandle(this, cb);
    }

    void dispatchMouseMoveEvent(float x, float y, bool isLButtonPressed,
                                bool isRButtonPressed)
    {
        STARFISH_ASSERT(mWebContainer);
        if (!mIsRunning) {
            return;
        }

        auto cb = [x, y, isLButtonPressed](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell] dispatchMouseMoveEvent()\n");
            TO_CONTAINER(data)
                ->DispatchMouseMoveEvent(
                    isLButtonPressed ? LWE::MouseButtonValue::LeftButton
                                     : LWE::MouseButtonValue::NoButton,
                    isLButtonPressed ? LWE::MouseButtonsValue::LeftButtonDown
                                     : LWE::MouseButtonsValue::NoButtonDown,
                    x, y);
        };
        sendAsyncHandle(this, cb);
    }

    void dispatchKeyDownEvent(LWE::KeyValue keyCode)
    {
        STARFISH_ASSERT(mWebContainer);
        if (!mIsRunning) {
            return;
        }

        auto cb = [keyCode](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell] dispatchKeyDownEvent()\n");
            TO_CONTAINER(data)->DispatchKeyDownEvent(keyCode);
        };
        sendAsyncHandle(this, cb);
    }

    void dispatchKeyPressEvent(LWE::KeyValue keyCode)
    {
        STARFISH_ASSERT(mWebContainer);
        if (!mIsRunning) {
            return;
        }

        auto cb = [keyCode](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell] dispatchKeyPressEvent()\n");
            TO_CONTAINER(data)->DispatchKeyPressEvent(keyCode);
        };
        sendAsyncHandle(this, cb);
    }

    void dispatchKeyUpEvent(LWE::KeyValue keyCode)
    {
        STARFISH_ASSERT(mWebContainer);
        if (!mIsRunning) {
            return;
        }

        auto cb = [keyCode](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell] dispatchKeyUpEvent()\n");
            TO_CONTAINER(data)->DispatchKeyUpEvent(keyCode);
        };
        sendAsyncHandle(this, cb);
    }

    void createInstance()
    {
        gDALiNumber++;

        auto cb = [](void* data) {
            DALiShellController* controller = (DALiShellController*)data;
            STARFISH_LOG_INFO("[DALi Shell] createInstance()\n");
            controller->mWebContainer = LWE::WebContainer::Create(
                controller->mOutputBuffer, controller->mOutputWidth,
                controller->mOutputHeight, controller->mOutputStride, 1.0,
                "SamsungOne", "ko-KR", "Asia/Seoul",
                "/tmp/StarFish_localStorage.txt", "/tmp/StarFish_Cookies.txt",
                "/tmp/StarFish-cache");
            TO_CONTAINER(data)
                ->RegisterOnRenderedHandler([controller](
                    LWE::WebContainer* container,
                    const LWE::WebContainer::RenderResult& renderResult) {
                    controller->onRenderedHandler(container, renderResult);
                });
            TO_CONTAINER(data)
                ->RegisterOnReceivedErrorHandler(
                    [controller](LWE::WebContainer* container,
                                 LWE::ResourceError error) -> void {
                        controller->mCanGoBack = container->CanGoBack();
                        controller->mCanGoForward = container->CanGoForward();
                        controller->onReceivedError(container, error);
                    });
            TO_CONTAINER(data)
                ->RegisterOnPageStartedHandler(
                    [controller](LWE::WebContainer* container,
                                 const std::string& url) -> void {
                        controller->mURL = url;
                        controller->mCanGoBack = container->CanGoBack();
                        controller->mCanGoForward = container->CanGoForward();
                        controller->onPageStartedHandler(container, url);
                    });
            TO_CONTAINER(data)
                ->RegisterOnPageLoadedHandler(
                    [controller](LWE::WebContainer* container,
                                 const std::string& url) -> void {
                        controller->mURL = url;
                        controller->mCanGoBack = container->CanGoBack();
                        controller->mCanGoForward = container->CanGoForward();
                        controller->onPageFinishedHandler(container, url);
                    });
            TO_CONTAINER(data)
                ->RegisterOnLoadResourceHandler(
                    [controller](LWE::WebContainer* container,
                                 const std::string& url) -> void {
                        controller->mURL = url;
                        controller->mCanGoBack = container->CanGoBack();
                        controller->mCanGoForward = container->CanGoForward();
                        controller->onLoadResourceHandler(container, url);
                    });
        };
        sendAsyncHandle(this, cb);
    }

    void loadURL(const std::string& url)
    {
        STARFISH_ASSERT(mWebContainer);
        auto cb = [url](void* data) {
            STARFISH_LOG_INFO("[DALi Shell] loadURL()\n");
            TO_CONTAINER(data)->LoadURL(url);
        };
        sendAsyncHandle(this, cb);
    }

    void registerOnRenderedHandler(
        const std::function<void(
            LWE::WebContainer* c,
            const LWE::WebContainer::RenderResult& renderResult)>& callback)
    {
        STARFISH_ASSERT(mWebContainer);
        auto cb = [callback](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell] registerOnRenderedHandler()\n");
            TO_CONTAINER(data)->RegisterOnRenderedHandler(callback);
        };
        sendAsyncHandle(this, cb);
    }

    void setSize()
    {
        STARFISH_ASSERT(mWebContainer);
        auto cb = [](void* data) {
            DALiShellController* controller = ((DALiShellController*)data);

            Locker l(gMutex);
            if (controller->mOutputBuffer) {
                free(controller->mOutputBuffer);
                controller->mOutputBuffer = nullptr;
            }
            controller->mOutputBuffer =
                (uint8_t*)malloc(controller->mOutputWidth *
                                 controller->mOutputHeight * sizeof(uint32_t));
            controller->mOutputStride =
                controller->mOutputWidth * sizeof(uint32_t);
            controller->mWebContainer->UpdateBuffer(
                controller->mOutputBuffer, controller->mOutputWidth,
                controller->mOutputHeight, controller->mOutputStride);
        };
        sendAsyncHandle(this, cb);
    }

    void loadData(const std::string& d)
    {
        STARFISH_ASSERT(mWebContainer);
        auto cb = [d](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell] loadData()\n");
            TO_CONTAINER(data)->LoadData(d);
        };
        sendAsyncHandle(this, cb);
    }

    void reload()
    {
        STARFISH_ASSERT(mWebContainer);
        auto cb = [](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell] reload()\n");
            TO_CONTAINER(data)->Reload();
        };
        sendAsyncHandle(this, cb);
    }

    void stopLoading()
    {
        STARFISH_ASSERT(mWebContainer);
        auto cb = [](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell] stopLoading()\n");
            TO_CONTAINER(data)->StopLoading();
        };
        sendAsyncHandle(this, cb);
    }

    void goBack()
    {
        STARFISH_ASSERT(mWebContainer);
        auto cb = [](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell] goBack()\n");
            TO_CONTAINER(data)->GoBack();
        };
        sendAsyncHandle(this, cb);
    }

    void goForward()
    {
        STARFISH_ASSERT(mWebContainer);
        auto cb = [](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell] goForward()\n");
            TO_CONTAINER(data)->GoForward();
        };
        sendAsyncHandle(this, cb);
    }

    void addJavaScriptInterface(
        const std::string& exposedObjectName, const std::string& jsFunctionName,
        std::function<std::string(const std::string&)> callback)
    {
        STARFISH_ASSERT(mWebContainer);
        auto cb = [exposedObjectName, jsFunctionName, callback](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell] addJavaScriptInterface()\n");
            TO_CONTAINER(data)
                ->AddJavaScriptInterface(exposedObjectName, jsFunctionName,
                                         callback);
        };
        sendAsyncHandle(this, cb);
    }
    void evaluateJavaScript(const std::string& script)
    {
        STARFISH_ASSERT(mWebContainer);
        auto cb = [script](void* data) {
            std::string ret = TO_CONTAINER(data)->EvaluateJavaScript(script);
            // STARFISH_LOG_INFO("[DALi Shell] evaluateJavaScript() returns
            // [%s]\n", ret.c_str());
        };
        sendAsyncHandle(this, cb);
    }

    void clearHistory()
    {
        STARFISH_ASSERT(mWebContainer);
        auto cb = [](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell] clearHistory()\n");
            DALiShellController* controller = (DALiShellController*)data;
            TO_CONTAINER(data)->ClearHistory();
            controller->mCanGoBack = TO_CONTAINER(data)->CanGoBack();
        };
        sendAsyncHandle(this, cb);
    }

    void destroy()
    {
        STARFISH_ASSERT(mWebContainer);
        auto cb = [](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell] destroy()\n");
            DALiShellController* controller = (DALiShellController*)data;

            TO_CONTAINER(data)->Destroy();

            while (!controller->mAsyncHandlePool.empty()) {
                UVAsyncHandleData* handleData = nullptr;
                {
                    Locker l(gMutex);
                    handleData = (UVAsyncHandleData*)*controller
                                     ->mAsyncHandlePool.begin();
                    controller->mAsyncHandlePool.erase(
                        controller->mAsyncHandlePool.begin());
                }

                if (handleData) {
                    handleData->cb(handleData->data);
                    delete handleData;
                }
            }

            gDALiNumber--;
        };
        sendAsyncHandle(this, cb);
    }

    void removeJavascriptInterface(const std::string& exposedObjectName,
                                   const std::string& jsFunctionName)
    {
        STARFISH_ASSERT(mWebContainer);
        auto cb = [exposedObjectName, jsFunctionName](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell] removeJavascriptInterface()\n");
            TO_CONTAINER(data)
                ->RemoveJavascriptInterface(exposedObjectName, jsFunctionName);
        };
        sendAsyncHandle(this, cb);
    }

    void clearCache()
    {
        STARFISH_ASSERT(mWebContainer);
        auto cb = [](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell] clearCache()\n");
            TO_CONTAINER(data)->ClearCache();
        };
        sendAsyncHandle(this, cb);
    }

    void registerOnReceivedErrorHandler(
        const std::function<void(LWE::WebContainer*, LWE::ResourceError)>&
            callback)
    {
        STARFISH_ASSERT(mWebContainer);
        auto cb = [callback](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell]
            // registerOnReceivedErrorHandler()\n");
            TO_CONTAINER(data)->RegisterOnReceivedErrorHandler(callback);
        };
        sendAsyncHandle(this, cb);
    }

    void registerOnPageStartedHandler(
        const std::function<void(LWE::WebContainer*, const std::string&)>&
            callback)
    {
        STARFISH_ASSERT(mWebContainer);
        auto cb = [callback](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell]
            // registerOnPageStartedHandler()\n");
            TO_CONTAINER(data)->RegisterOnPageStartedHandler(callback);
        };
        sendAsyncHandle(this, cb);
    }

    void registerOnPageFinishedHandler(
        const std::function<void(LWE::WebContainer*, const std::string&)>&
            callback)
    {
        STARFISH_ASSERT(mWebContainer);
        auto cb = [callback](void* data) {
            // STARFISH_LOG_INFO("[DALi Shell]
            // registerOnPageFinishedHandler()\n");
            TO_CONTAINER(data)->RegisterOnPageLoadedHandler(callback);
        };
        sendAsyncHandle(this, cb);
    }

    void callEmptyAsyncHandle(DALiShellController* controller)
    {
        STARFISH_ASSERT(mWebContainer);
        auto cb = [](void* data) {
            STARFISH_LOG_INFO("[DALi Shell] callEmptyAsyncHandle()\n");
        };
        sendAsyncHandle(controller, cb);
    }

    void stopLoop(DALiShellController* controller)
    {
        gDALiNumber = -1;
        callEmptyAsyncHandle(controller);
    }
};

void DALiShellController::Create(Application& application)
{
    startMainThreadIfNeeds(mThreadHandle);

    createInstance();

    while (true) {
        if (mWebContainer) {
            break;
        }
        usleep(100);
    }

    mIsRunning = true;
    mImageView = Dali::Toolkit::ImageView::New();
    mImageView.SetParentOrigin(Dali::ParentOrigin::TOP_LEFT);
    mImageView.SetAnchorPoint(Dali::AnchorPoint::TOP_LEFT);

    Stage::GetCurrent().KeyEventSignal().Connect(
        this, &DALiShellController::onKeyEvent);

    ((Dali::Toolkit::ImageView)mImageView)
        .TouchSignal()
        .Connect(this, &DALiShellController::touchEventHandler);
    Toolkit::KeyboardFocusManager::Get().SetCurrentFocusActor(mImageView);
    Stage::GetCurrent().KeyEventSignal().Connect(
        this, &DALiShellController::keyEventHandler);

    onRenderedHandler = [this](
        LWE::WebContainer* c,
        const LWE::WebContainer::RenderResult& renderResult) {
        // STARFISH_LOG_INFO("[DALi Shell] onRenderedHandler()\n");

        Locker l(gMutex);
        int w = mOutputWidth;
        int h = mOutputHeight;
        uint8_t* dstBuffer;
        size_t dstStride;
#if defined(STARFISH_DALI_TBMSURFACE)
        tbm_surface_info_s tbmSurfaceInfo;
        if (tbm_surface_map(mTbmSurface, TBM_SURF_OPTION_WRITE,
                            &tbmSurfaceInfo) != TBM_SURFACE_ERROR_NONE) {
            STARFISH_LOG_ERROR("Fail to map tbm_surface\n");
            abort();
        }
        dstBuffer = tbmSurfaceInfo.planes[0].ptr;
        dstStride = tbmSurfaceInfo.planes[0].stride;
#else
        dstBuffer = mBufferImage.GetBuffer();
        dstStride = mBufferImage.GetBufferStride();
#endif

        uint32_t srcStride = renderResult.updatedWidth * sizeof(uint32_t);
        uint8_t* srcBuffer = (uint8_t*)renderResult.updatedBufferAddress;

        if (dstStride == srcStride) {
            for (auto y = renderResult.updatedY;
                 y < (renderResult.updatedHeight + renderResult.updatedY);
                 y++) {
                auto start = renderResult.updatedX;
                memcpy(dstBuffer + (y * dstStride) + (start * 4),
                       srcBuffer + (y * srcStride) + (start * 4), srcStride);
            }
            misNeedsUpdate = true;
        }

#if defined(STARFISH_DALI_TBMSURFACE)
        tbm_surface_unmap(mTbmSurface);
#endif
    };

    onReceivedError = [](LWE::WebContainer* container,
                         LWE::ResourceError error) {
        // STARFISH_LOG_INFO("[DALi Shell] onReceivedError()\n");
    };
    onPageStartedHandler = [](LWE::WebContainer* container,
                              const std::string& url) {
        // STARFISH_LOG_INFO("[DALi Shell] onPageStartedHandler()\n");
    };
    onPageFinishedHandler = [](LWE::WebContainer* container,
                               const std::string& url) {
        // STARFISH_LOG_INFO("[DALi Shell] onPageFinishedHandler()\n");
    };
    onLoadResourceHandler = [](LWE::WebContainer* container,
                               const std::string& url) {
        // STARFISH_LOG_INFO("[DALi Shell] onLoadResourceHandler()\n");
    };

#if defined(STARFISH_DALI_TBMSURFACE)
    mTbmSurface =
        tbm_surface_create(mOutputWidth, mOutputHeight, TBM_FORMAT_ABGR8888);
    Dali::Any source(mTbmSurface);
    mNativeImageSrc = Dali::NativeImageSource::New(source);
    mNativeImage = Dali::NativeImage::New(*mNativeImageSrc);
    mNativeImageSrc->SetSource(source);
    ((Dali::Toolkit::ImageView)mImageView).SetImage(mNativeImage);
#else
    mBufferImage = Dali::BufferImage::New(mOutputWidth, mOutputHeight,
                                          Dali::Pixel::RGBA8888);
    // STARFISH_LOG_INFO("[DALi Shell] [Dali BufImg:%p]\n",
    // mBufferImage.GetBuffer());
    ((Dali::Toolkit::ImageView)mImageView).SetImage(mBufferImage);
#endif

    Dali::Stage::GetCurrent().Add(mImageView);

    loadURL(mURL);

    mTimer = Dali::Timer::New(20);
    mTimer.TickSignal().Connect(this, &DALiShellController::updateBuffer);
    mTimer.Start();
}

void DALiShellController::onKeyEvent(const Dali::KeyEvent& event)
{
    STARFISH_ASSERT(mWebContainer);

    // STARFISH_LOG_INFO("[DALi Shell] key pressed [%d]\n", event.keyCode);

    if (event.state == KeyEvent::Up) {
        if (IsKey(event, DALI_KEY_ESCAPE) || IsKey(event, DALI_KEY_BACK)) {
            if (mIsRunning == true) {
                mIsRunning = false;
                Dali::Stage::GetCurrent().Remove(mImageView);

                destroy();
            }
            stopLoop((DALiShellController*)this);
            int status;
            pthread_join(mThreadHandle, (void**)&status);

            mApplication.Quit();
        } else {
            // F1
            if (event.keyCode == 67) {
                if (mOutputWidth != 800) {
                    Locker l(gMutex);
                    mOutputWidth = 800;
                    mOutputHeight = 600;

#if defined(STARFISH_DALI_TBMSURFACE)
#else
                    mBufferImage = Dali::BufferImage::New(
                        mOutputWidth, mOutputHeight, Dali::Pixel::RGBA8888);
                    ((Dali::Toolkit::ImageView)mImageView)
                        .SetImage(mBufferImage);
#endif
                    setSize();
                }
                // F2
            } else if (event.keyCode == 68) {
                if (mOutputWidth != 1280) {
                    Locker l(gMutex);
                    mOutputWidth = 1280;
                    mOutputHeight = 720;
#if defined(STARFISH_DALI_TBMSURFACE)
#else
                    mBufferImage = Dali::BufferImage::New(
                        mOutputWidth, mOutputHeight, Dali::Pixel::RGBA8888);
                    ((Dali::Toolkit::ImageView)mImageView)
                        .SetImage(mBufferImage);
#endif
                    setSize();
                }
                // F3
            } else if (event.keyCode == 69) {
                std::string str =
                    "<!DOCTYPE html><html><body><div "
                    "style=\"width:200px;height:200px;background-color:green;"
                    "\"></div></body></html>";
                loadData(str);
                // F4
            } else if (event.keyCode == 70) {
                stopLoading();
                // F5
            } else if (event.keyCode == 71) {
                reload();
                // F6
            } else if (event.keyCode == 72) {
                goBack();
                // F7
            } else if (event.keyCode == 73) {
                goForward();
                // F8
            } else if (event.keyCode == 74) {
                bool ret = mCanGoBack;
                STARFISH_LOG_INFO("[DALi Shell] canGoBack() returns [%s]\n",
                                  ret ? "true" : "false");
                // F9
            } else if (event.keyCode == 75) {
                addJavaScriptInterface(
                    "testObj", "testFunc",
                    [](const std::string& str) -> std::string {
                        return str + " world!!";
                    });
                evaluateJavaScript("testObj.testFunc('hello')");
                // F10
            } else if (event.keyCode == 76) {
                clearHistory();
                // F11
            } else if (event.keyCode == 77) {
                removeJavascriptInterface("testObj", "testFunc");
                // F12
            } else if (event.keyCode == 78) {
                clearCache();
            }
        }
    }
}

void sendAsyncHandle(DALiShellController* controller,
                     std::function<void(void*)> cb)
{
    UVAsyncHandleData* handle = new UVAsyncHandleData();
    handle->cb = cb;
    handle->data = controller;

    pthread_mutex_lock(&gMutex);
    controller->mAsyncHandlePool.push_back((size_t)handle);
    pthread_mutex_unlock(&gMutex);

    gLauncherHandle.data = controller;
    uv_async_send(&gLauncherHandle);
}

static void* startMainThread(void* data)
{
    STARFISH_LOG_INFO("[DALi Shell] uv_run() start\n");
    uv_async_init(uv_default_loop(), &gLauncherHandle, [](uv_async_t* handle) {
        DALiShellController* controller = (DALiShellController*)handle->data;
        while (!controller->mAsyncHandlePool.empty()) {
            UVAsyncHandleData* handleData = nullptr;
            {
                pthread_mutex_lock(&gMutex);
                handleData =
                    (UVAsyncHandleData*)*controller->mAsyncHandlePool.begin();
                controller->mAsyncHandlePool.erase(
                    controller->mAsyncHandlePool.begin());
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
        if (gDALiNumber < 0) {
            break;
        }
    }

    STARFISH_LOG_INFO("[DALi Shell] uv_run() end\n");

    return NULL;
}

namespace LWE {

class WebViewDALi : public WebView {
public:
    WebViewDALi(void* winArg, int x, int y, int width, int height,
                float devicePixelRatio, const char* defaultFontName,
                const char* locale, const char* timezoneID,
                const char* localStorageFilePath,
                const char* cookieStoreFilePath,
                const char* httpCacheDirectorypath)
        : WebView(nullptr)
        , m_mainThreadHandle(0)
        , m_controller(nullptr)
        , m_width(width)
        , m_height(height)
    {
    }

    void LoadURL(const std::string& url) override
    {
        m_url = url;

        static int fakeArgc;
        static char** fakeArgv;
        m_application = Application::New(&fakeArgc, &fakeArgv);
        m_controller = new DALiShellController(
            m_application, m_mainThreadHandle, m_width, m_height, m_url);
        m_application.MainLoop();
    }

    virtual void Destroy() override
    {
        m_controller->destroy();
    }

protected:
    Application m_application;
    pthread_t m_mainThreadHandle;
    std::string m_url;
    virtual ::LWE::WebContainer* FetchWebContainer() override
    {
        return (::LWE::WebContainer*)m_impl;
    }

    DALiShellController* m_controller;
    int m_width, m_height;
};

WebView* WebView::Create(void* win, int x, int y, int width, int height,
                         float devicePixelRatio, const char* defaultFontName,
                         const char* locale, const char* timezoneID,
                         const char* localStorageFilePath,
                         const char* cookieStoreFilePath,
                         const char* httpCacheDirectorypath)
{
    return new WebViewDALi(win, x, y, width, height, devicePixelRatio,
                           defaultFontName, locale, timezoneID,
                           localStorageFilePath, cookieStoreFilePath,
                           httpCacheDirectorypath);
}
}

#endif
