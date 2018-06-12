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
#include "StarFish.h"

#include "LWEWebView.h"
#include "platform/window/PlatformWindow.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/page/History.h"
#include "core/page/Location.h"
#include "core/dom/Document.h"
#include "binding/ScriptWrappable.h"
#include "JavaScriptNativeHandler.h"
#include "core/modules/threading/Thread.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "platform/network/HTTPCache.h"
#include "core/dom/MouseEvent.h"
#include "core/event/KeyBoardEventData.h"
#include "platform/event/PlatformKeyEventData.h"

#include <EscargotPublic.h>

#if defined(STARFISH_ANDROID)
#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>

#define LOG_TAG "StarFish"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

struct WindowGlue {
    JNIEnv* m_env;
    jclass m_clazz;
    jmethodID m_startTimer;
    jmethodID m_cancelTimer;
    jmethodID m_requestRender;
    jmethodID m_onLoadResource;
    jmethodID m_onReceivedError;
    jmethodID m_onPageFinished;
    jmethodID m_onPageStarted;
    WindowGlue()
    {
        m_startTimer = m_requestRender = 0;
    }
} g_WindowGlue;
JavaVM* g_jvm;

std::map<LWE::WebContainer*, std::pair<jobject, void*>> g_webViews;

typedef bool (*TimerCallback)(int uid, void* data);
int startTimer(int ms, TimerCallback pointer, void* data);
void cancelTimer(int uid);

void callOnLoadResourceHandler(LWE::WebView* view, const char* url);
void callOnReceivedError(LWE::WebView* view, int errorCode, bool canGoBack,
                         bool canGoForward);
void callOnPageFinished(LWE::WebView* view, const char* url, bool canGoBack,
                        bool canGoForward);
void callOnPageStarted(LWE::WebView* view, const char* url, bool canGoBack,
                       bool canGoForward);

#elif defined(STARFISH_DALI)
#include <dali-toolkit/dali-toolkit.h>
#include <dali-toolkit/devel-api/controls/web-view-lite/web-view-lite.h>
#if defined(STARFISH_TIZEN)
#include <tbm_surface.h>
#endif
#include <uv.h>
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Locker.h"
#include "platform/event/PlatformKeyEventData.h"

uv_async_t gLauncherHandle;
pthread_mutex_t* gMutex;
bool gIsAliveMainLoop = false;

#endif

#define TO_STARFISH(ptr) ((StarFish::StarFish*)ptr)
#define TO_HISTORY(ptr)         \
    ((StarFish::StarFish*)ptr)  \
        ->platformWindow()      \
        ->webView()             \
        ->mainBrowsingContext() \
        ->window()              \
        ->history()

#define TO_LOCATION(ptr)        \
    ((StarFish::StarFish*)ptr)  \
        ->platformWindow()      \
        ->webView()             \
        ->mainBrowsingContext() \
        ->window()              \
        ->location()

#define TO_RESOURCE_LOADER(ptr) \
    ((StarFish::StarFish*)ptr)  \
        ->platformWindow()      \
        ->webView()             \
        ->mainBrowsingContext() \
        ->document()            \
        ->resourceLoader()

#define TO_SCRIPT_BINDING_INSTANCE(ptr) \
    ((StarFish::StarFish*)ptr)          \
        ->platformWindow()              \
        ->webView()                     \
        ->mainBrowsingContext()         \
        ->window()                      \
        ->scriptBindingInstance()

namespace LWE {

static StarFish::ScriptValue nativeCallbackFunction(
    StarFish::ScriptExecutionState state, StarFish::ScriptValue thisValue,
    size_t argc, StarFish::ScriptValue* argv, bool isNewExpression)
{
    auto callee = StarFish::toCalleeObject(state);
    if (callee) {
        void* data = callee->extraData();
        if (data) {
            StarFish::ScriptWrappable* w = (StarFish::ScriptWrappable*)data;
            if (w->isJavaScriptNativeHandler()) {
                StarFish::JavaScriptNativeHandler* jsNhandler =
                    (StarFish::JavaScriptNativeHandler*)w;
                StarFish::String* result = StarFish::String::emptyString;
                StarFish::String* param = StarFish::String::emptyString;
                if (argc > 0) {
                    StarFish::ScriptValue arg0 = argv[0];
                    param = StarFish::toBrowserString(state, arg0);
                }
                result = jsNhandler->callNativeHandler(param);
                return StarFish::createScriptValue(
                    StarFish::createScriptString(result));
            }
        }
    }
    return StarFish::scriptUndefined();
}

#if defined(STARFISH_DALI)
StarFish::PlatformKeyEventData DaliEventKeyToKeyboardData(
    const char* DALIKeyString, bool isShiftPressed)
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
    StarFish::PlatformKeyEventData kdata(keyValue);
    if (strcmp("Shift_L", DALIKeyString) == 0 ||
        strcmp("Shift_R", DALIKeyString) == 0) {
        kdata.setShiftKey(true);
    } else if (strcmp("Control_L", DALIKeyString) == 0 ||
               strcmp("Control_R", DALIKeyString) == 0) {
        kdata.setCtrlKey(true);
    } else if (strcmp("Alt_L", DALIKeyString) == 0 ||
               strcmp("Alt_R", DALIKeyString) == 0) {
        kdata.setAltKey(true);
    }

    return kdata;
}

class DaliPluginClient : public Dali::ConnectionTracker {
public:
    DaliPluginClient(int x, int y, int w, int h, void* actor)
        : m_isInit(false)
        , m_isMouseLbuttonDown(false)
        , m_lastClickedTimestamp(0)
        , m_clickedCount(0)
        , m_x(x)
        , m_y(y)
        , m_w(w)
        , m_h(h)
        , m_starFish(nullptr)
    {
#if defined(STARFISH_TIZEN)
        m_surface1 = tbm_surface_create(w, h, TBM_FORMAT_ARGB8888);
        m_surface2 = tbm_surface_create(w, h, TBM_FORMAT_ARGB8888);
        if (tbm_surface_map(m_surface1,
                            TBM_SURF_OPTION_READ | TBM_SURF_OPTION_WRITE,
                            &m_surface_info1) != TBM_SURFACE_ERROR_NONE) {
            STARFISH_LOG_INFO("Fail to map tbm_surface\n");
            return;
        }
        if (tbm_surface_map(m_surface2,
                            TBM_SURF_OPTION_READ | TBM_SURF_OPTION_WRITE,
                            &m_surface_info2) != TBM_SURFACE_ERROR_NONE) {
            STARFISH_LOG_INFO("Fail to map tbm_surface\n");
            return;
        }

        Dali::Any source(m_surface1);
        mImgSrc = Dali::NativeImageSource::New(source);
        mImg = Dali::NativeImage::New(*mImgSrc);
#else
        mImg = Dali::BufferImage::New(w, h, Dali::Pixel::BGRA8888);
#endif
        mMainImgView = (Dali::Toolkit::ImageView*)actor;
        mMainImgView->SetImage(mImg);

        mMainImgView->TouchSignal().Connect(
            this, &DaliPluginClient::TouchEventHandler);
        //        mMainImgView->HoveredSignal().Connect(this,
        //        &DaliWebViewClient::HoverEventHandler);
        Dali::Stage::GetCurrent().KeyEventSignal().Connect(
            this, &DaliPluginClient::KeyEventHandler);
    }
    ~DaliPluginClient()
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

    bool TouchEventHandler(Dali::Actor actor, const Dali::TouchData& data)
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
                StarFish::MouseEventKind kind;
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
                d->kind = StarFish::MouseEventKind::MouseEventDown;
                m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
                    m_starFish->platformWindow()
                        ->webView()
                        ->mainBrowsingContext(),
                    [](size_t, void* data) {
                        dummy* d = (dummy*)data;
                        StarFish::StarFish* m_sf = d->starfish;
                        StarFish::MouseData mouseData = d->data;
                        m_sf->platformWindow()->dispatchMouseEvent(
                            StarFish::MouseEventKind::MouseEventDown,
                            mouseData);
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
                d->kind = StarFish::MouseEventKind::MouseEventUp;
                m_starFish->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
                    m_starFish->platformWindow()
                        ->webView()
                        ->mainBrowsingContext(),
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
                    m_starFish->platformWindow()
                        ->webView()
                        ->mainBrowsingContext(),
                    [](size_t, void* data) {
                        dummy* d = (dummy*)data;
                        StarFish::StarFish* m_sf = d->starfish;
                        StarFish::MouseData mouseData = d->data;
                        m_sf->platformWindow()->dispatchMouseEvent(
                            StarFish::MouseEventKind::MouseEventMove,
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
        if (m_starFish == nullptr || !m_isInit)
            return true;
        if (!(m_starFish->platformWindow() &&
              m_starFish->platformWindow()->webView() &&
              m_starFish->platformWindow()->webView()->mainBrowsingContext())) {
            return true;
        }

        const Dali::Vector2& point = event.GetPoint(0).local;
        StarFish::StarFishEnterer enter(m_starFish);
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

    void KeyEventHandler(const Dali::KeyEvent& event)
    {
        if (m_starFish == nullptr || !m_isInit)
            return;
        if (!(m_starFish->platformWindow() &&
              m_starFish->platformWindow()->webView() &&
              m_starFish->platformWindow()->webView()->mainBrowsingContext())) {
            return;
        }

        StarFish::PlatformKeyEventData kdata(
            StarFish::KeyValue::UnidentifiedKey);
        if (32 < event.keyPressed.c_str()[0] &&
            127 > event.keyPressed.c_str()[0]) {
            kdata = StarFish::PlatformKeyEventData(
                (StarFish::KeyValue)event.keyPressed.c_str()[0]);
        } else {
            kdata = DaliEventKeyToKeyboardData(event.keyPressedName.c_str(),
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

    void SetStarFish(StarFish::StarFish* starFish)
    {
        m_starFish = starFish;
    }

#if defined(STARFISH_TIZEN)
    bool UpdateBuffer()
    {
        if (m_isInit) {
            // if (m_starFish != nullptr) {
            //     int bufferIdx = m_starFish->updateFrameBuffer();
            //     if (bufferIdx == 1) {
            //         Dali::Any source(m_surface1);
            //         ((Dali::NativeImageSourcePtr)mImgSrc)->SetSource(source);
            //         Dali::Stage::GetCurrent().KeepRendering(0.01f);
            //     } else if (bufferIdx == 2) {
            //         Dali::Any source(m_surface2);
            //         ((Dali::NativeImageSourcePtr)mImgSrc)->SetSource(source);
            //         Dali::Stage::GetCurrent().KeepRendering(0.01f);
            //     }
            // }
        }
        return true;
    }
#else
    bool UpdateBuffer()
    {
        // if (m_isInit && m_starFish != nullptr &&
        //     m_starFish->updateFrameBuffer()) {
        //     mImg.Update();
        // }
        return true;
    }
#endif
    bool m_isInit;
    bool m_isMouseLbuttonDown;
    uint32_t m_lastClickedTimestamp;
    uint32_t m_clickedCount;
    int m_x, m_y, m_w, m_h;

#if defined(STARFISH_TIZEN)
    tbm_surface_h m_surface1;
    tbm_surface_h m_surface2;
    tbm_surface_info_s m_surface_info1;
    tbm_surface_info_s m_surface_info2;

    Dali::NativeImageSourcePtr mImgSrc;
    Dali::NativeImage mImg;
#else
    Dali::BufferImage mImg;
#endif
    Dali::Toolkit::ImageView* mMainImgView;
    Dali::Timer m_timer;

    StarFish::StarFish* m_starFish;
};

static bool isAliveMainThread()
{
    return gIsAliveMainLoop;
}

struct mock {
public:
    void* webView;
    void* client;
};

static void* startMainThread(void* data)
{
    uv_async_init(uv_default_loop(), &gLauncherHandle, [](uv_async_t* handle) {
        mock* mk = (mock*)handle->data;
        DaliPluginClient* client = (DaliPluginClient*)mk->client;

        // WebContainer* webView = WebContainer::Create(nullptr, client->m_x,
        // client->m_y,
        //                                    client->m_w, client->m_h);
        //         StarFish::StarFish* starFish =
        //             (StarFish::StarFish*)webView->getInternalPtr();

        //         pthread_mutex_unlock(gMutex);

        //         mk->webView = webView;
        //         client->SetStarFish(starFish);

        // #if defined(STARFISH_TIZEN)
        //         starFish->registerFrameBuffer(client->m_surface_info1.planes[0].ptr,
        //                                       client->m_surface_info2.planes[0].ptr);
        // #else
        //         starFish->registerFrameBuffer((void*)client->mImg.GetBuffer(),
        //         nullptr);
        // #endif

        //         starFish->run();

        uv_close((uv_handle_t*)handle, nullptr);
    });

    gIsAliveMainLoop = true;
    pthread_mutex_unlock(gMutex);
    while (true) {
        uv_run(uv_default_loop(), UV_RUN_ONCE);
    }
    return NULL;
}

static void initMainThread(void* (*f)(void*))
{
    gMutex = new pthread_mutex_t;
    pthread_mutex_init(gMutex, NULL);

    pthread_mutex_lock(gMutex);
    pthread_t t;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&t, &attr, f, NULL);
    pthread_mutex_lock(gMutex);
    pthread_mutex_unlock(gMutex);
}

#endif

WebContainer* WebContainer::Create(void* buffer, uint width, uint height,
                                   uint stride, float scaleFactor)
{
#if !defined(PORT_GRAPHIC_BACKEND_GENERAL_BUFFER)
    STARFISH_LOG_ERROR("Cannot use WebContainer this port!");
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
    return nullptr;
#endif
    std::string screenShot;
    std::string customUserAgentString;
    std::string builtinPolyfillPathString;
    int flag = 0;

    const char* defaultFontName = "serif";

#if defined(STARFISH_DALI)
    defaultFontName = "samsungOne";
#endif

    std::string tempPath = "/tmp/";

#if defined(OS_WINDOWS)
    tempPath = ::StarFish::getWindowsTempDir();
#endif

    std::string localStoragePath = tempPath;
    localStoragePath += "StarFish_localStorage.txt";

    std::string cookiePath = tempPath;
    localStoragePath += "StarFish_Cookies.txt";

    StarFish::ScreenInfo info;
    info.rect.setWidth(width);
    info.rect.setHeight(height);
    info.availableRect.setWidth(width);
    info.availableRect.setHeight(height);
    info.deviceScaleFactor = scaleFactor;

    std::string cacheDir = tempPath;
    cacheDir += "Starfish-cache";
    StarFish::StarFish* starfish = new (NoGC) StarFish::StarFish(
        (StarFish::StarFishStartUpFlag)flag, "ko-KR", "Asia/Seoul", nullptr,
        width, height, 0, 0, 1,
        StarFish::String::createASCIIString(defaultFontName), info,
        localStoragePath.data(), cookiePath.data(), cacheDir.data(),
        StarFish::String::fromUTF8(customUserAgentString.data()),
        StarFish::String::fromUTF8(builtinPolyfillPathString.data()));
    starfish->platformWindow()->updateDrawingBufferAddress(buffer, width,
                                                           height, stride);

    WebContainer* newWebContainer = new WebContainer(starfish);

#if defined(STARFISH_ANDROID)
    starfish->setLWEWebView(newWebContainer);
#endif
    return newWebContainer;

    // #if defined(STARFISH_DALI)

    //     DaliPluginClient* client = new DaliPluginClient(x, y, w, h, actor);

    //     if (!isAliveMainThread()) {
    //         initMainThread(&startMainThread);
    //     }

    //     pthread_mutex_lock(gMutex);

    //     mock* mk = new mock;
    //     mk->client = client;
    //     gLauncherHandle.data = mk;
    //     uv_async_send(&gLauncherHandle);

    //     pthread_mutex_lock(gMutex);
    //     pthread_mutex_unlock(gMutex);

    //     client->m_isInit = true;
    //     client->m_timer = Dali::Timer::New(20);
    //     client->m_timer.TickSignal().Connect(client,
    //                                          &DaliPluginClient::UpdateBuffer);
    //     client->m_timer.Start();

    //     WebView* wv = (WebView*)mk->webView;
    //     delete mk;

    //     return wv;
    // #else
    //     return Create(win, x, y, w, h);
    // #endif
}

WebContainer::WebContainer(void* starFish)
    : m_starfish(starFish)
{
}

Settings WebContainer::GetSettings()
{
    STARFISH_ASSERT(m_starfish);
    Settings result(USER_AGENT(STARFISH_NAME, VERSION),
                    TO_STARFISH(m_starfish)->userAgent()->toUTF8NonGCString());
#ifdef STARFISH_ENABLE_HTTPCACHE
    result.SetCacheMode(TO_STARFISH(m_starfish)->httpCache()->cacheMode());
#endif
    return result;
}

void WebContainer::LoadURL(const std::string& url)
{
    STARFISH_ASSERT(m_starfish);
#if defined(STARFISH_DALI)
    struct dummy {
        StarFish::StarFish* starfish;
        char data[128];
    };
    dummy* d = new dummy;
    d->starfish = TO_STARFISH(m_starfish);

    strcpy(d->data, url.c_str());
    TO_STARFISH(m_starfish)
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            nullptr,
            [](size_t, void* data) {
                dummy* d = (dummy*)data;
                StarFish::StarFish* starFish = d->starfish;
                StarFish::StarFishEnterer enter(starFish);
                starFish->loadHTMLDocument(
                    StarFish::String::fromUTF8(&(d->data)[0]));
                delete d;
            },
            d);
#else
    TO_STARFISH(m_starfish)
        ->loadHTMLDocument(StarFish::String::fromUTF8(url.data()));
#endif
}

std::string WebContainer::GetURL()
{
    STARFISH_ASSERT(m_starfish);
    return TO_LOCATION(m_starfish)->url()->urlString()->toUTF8NonGCString();
}

void WebContainer::LoadData(const std::string& data)
{
    STARFISH_ASSERT(m_starfish);
    if (data.size() > 0) {
        auto dataURI = StarFish::StringUtils::toBase64HTMLDataURI(data);
        TO_STARFISH(m_starfish)
            ->loadHTMLDocument(StarFish::String::fromUTF8(dataURI.data()));
    } else {
        TO_STARFISH(m_starfish)
            ->loadHTMLDocument(StarFish::String::fromUTF8("about:blank"));
    }
}

void WebContainer::Reload()
{
    STARFISH_ASSERT(m_starfish);
    TO_LOCATION(m_starfish)->reload();
}

void WebContainer::StopLoading()
{
    STARFISH_ASSERT(m_starfish);
    TO_RESOURCE_LOADER(m_starfish).clear();
}

void WebContainer::GoBack()
{
    STARFISH_ASSERT(m_starfish);
    TO_HISTORY(m_starfish)->back();
}

void WebContainer::GoForward()
{
    STARFISH_ASSERT(m_starfish);
    TO_HISTORY(m_starfish)->forward();
}

bool WebContainer::CanGoBack()
{
    STARFISH_ASSERT(m_starfish);
    return TO_HISTORY(m_starfish)->canGoBack();
}

bool WebContainer::CanGoForward()
{
    STARFISH_ASSERT(m_starfish);
    return TO_HISTORY(m_starfish)->canGoForward();
}

void WebContainer::AddJavaScriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName,
    std::function<std::string(const std::string&)> cb)
{
    STARFISH_ASSERT(m_starfish);

    StarFish::String* objectName =
        StarFish::String::fromUTF8(exposedObjectName.c_str());
    StarFish::String* functionName =
        StarFish::String::fromUTF8(jsFunctionName.c_str());

    StarFish::registerJavaScriptNativeInterface(
        TO_SCRIPT_BINDING_INSTANCE(m_starfish), objectName, functionName,
        new StarFish::JavaScriptNativeHandler(TO_STARFISH(m_starfish),
                                              functionName, cb),
        nativeCallbackFunction);
}

std::string WebContainer::EvaluateJavaScript(const std::string& script)
{
    STARFISH_ASSERT(m_starfish);
    return TO_STARFISH(m_starfish)
        ->evaluate(StarFish::String::fromUTF8(script.c_str()))
        ->toUTF8NonGCString();
}

void WebContainer::ClearHistory()
{
    STARFISH_ASSERT(m_starfish);
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->webView()
        ->historyManager()
        ->clear();
}

void WebContainer::Destroy()
{
#if defined(STARFISH_DALI)
    STARFISH_ASSERT(m_starfish);
    delete TO_STARFISH(m_starfish);
#else
    STARFISH_ASSERT(m_starfish);
    delete TO_STARFISH(m_starfish);
#endif
}

void WebContainer::SetSettings(const Settings& setttings)
{
    STARFISH_ASSERT(m_starfish);
    TO_STARFISH(m_starfish)
        ->setCustomUserAgentString(
            StarFish::String::fromUTF8(setttings.GetUserAgentString().c_str()));
#ifdef STARFISH_ENABLE_HTTPCACHE
    TO_STARFISH(m_starfish)
        ->httpCache()
        ->setCacheMode(setttings.GetCacheMode());
#endif
}
void WebContainer::RemoveJavascriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName)
{
    STARFISH_ASSERT(m_starfish);
    StarFish::String* objectName =
        StarFish::String::fromUTF8(exposedObjectName.c_str());
    if (jsFunctionName != nullptr) {
        StarFish::String* functionName =
            StarFish::String::fromUTF8(jsFunctionName.c_str());
        StarFish::unregisterJavaScriptNativeInterface(
            TO_SCRIPT_BINDING_INSTANCE(m_starfish), objectName, functionName);
    } else {
        StarFish::unregisterJavaScriptNativeInterface(
            TO_SCRIPT_BINDING_INSTANCE(m_starfish), objectName);
    }
}
void WebContainer::ClearCache()
{
    STARFISH_ASSERT(m_starfish);
#ifdef STARFISH_ENABLE_HTTPCACHE
    TO_STARFISH(m_starfish)->httpCache()->clear();
#endif
}

void WebContainer::RegisterOnReceivedErrorHandler(
    const std::function<void(LWE::WebContainer*, LWE::ResourceError)>& cb)
{
    TO_STARFISH(m_starfish)
        ->registerWebViewHandler(
            std::string("OnReceivedError"),
            [this, cb](StarFish::String* url, int errorCode) -> void {
                // make error description
                cb(this, ResourceError(errorCode, std::string()));
            });
}

void WebContainer::RegisterOnPageFinishedHandler(
    const std::function<void(LWE::WebContainer*, const std::string&)>& cb)
{
    TO_STARFISH(m_starfish)
        ->registerWebViewHandler(
            std::string("OnPageFinished"),
            [this, cb](StarFish::String* url, int errorCode) -> void {
                cb(this, url->toUTF8NonGCString());
            });
}

void WebContainer::RegisterOnPageStartedHandler(
    const std::function<void(LWE::WebContainer*, const std::string&)>& cb)
{
    TO_STARFISH(m_starfish)
        ->registerWebViewHandler(
            std::string("OnPageStarted"),
            [this, cb](StarFish::String* url, int errorCode) -> void {
                cb(this, url->toUTF8NonGCString());
            });
}

void WebContainer::RegisterOnLoadResourceHandler(
    const std::function<void(LWE::WebContainer*, const std::string&)>& cb)
{
    TO_STARFISH(m_starfish)
        ->registerWebViewHandler(
            std::string("OnLoadResource"),
            [this, cb](StarFish::String* url, int errorCode) -> void {
                cb(this, url->toUTF8NonGCString());
            });
}

void WebContainer::UpdateBuffer(void* buffer, uint width, uint height,
                                uint stride)
{
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->updateDrawingBufferAddress(buffer, width, height, stride);
}
void WebContainer::RenderingDirectly()
{
    TO_STARFISH(m_starfish)->platformWindow()->rendering();
}

void WebContainer::RegisterOnRenderedHandler(
    const std::function<void(LWE::WebContainer*, void*)>& cb)
{
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->registerRenderingFinishedCallback([this, cb]() {
            cb(this, TO_STARFISH(m_starfish)
                         ->platformWindow()
                         ->drawingBufferAddress());
        });
}

void WebContainer::SetUserAgentString(const std::string& userAgent)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void WebContainer::SetCacheMode(int mode)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void WebContainer::DispatchMouseMoveEvent(MouseButtonValue button,
                                          MouseButtonsValue buttons, double x,
                                          double y)
{
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->dispatchMouseEvent(::StarFish::MouseEventKind::MouseEventMove,
                             ::StarFish::MouseData(button, buttons, x, y, 0));
}

void WebContainer::DispatchMouseDownEvent(MouseButtonValue button,
                                          MouseButtonsValue buttons, double x,
                                          double y)
{
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->dispatchMouseEvent(::StarFish::MouseEventKind::MouseEventDown,
                             ::StarFish::MouseData(button, buttons, x, y, 0));
}

void WebContainer::DispatchMouseUpEvent(MouseButtonValue button,
                                        MouseButtonsValue buttons, double x,
                                        double y)
{
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->dispatchMouseEvent(::StarFish::MouseEventKind::MouseEventUp,
                             ::StarFish::MouseData(button, buttons, x, y, 0));
}

void WebContainer::DispatchMouseWheelEvent(double x, double y, int delta)
{
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->dispatchMouseWheelEvent(x, y, delta, true);
}

void WebContainer::DispatchKeyDownEvent(KeyValue keyCode, int modifier)
{
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->dispatchKeyEvent(::StarFish::KeyEventKind::KeyEventDown,
                           ::StarFish::PlatformKeyEventData(keyCode));
}

void WebContainer::DispatchKeyUpEvent(KeyValue keyCode, int modifier)
{
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->dispatchKeyEvent(::StarFish::KeyEventKind::KeyEventUp,
                           ::StarFish::PlatformKeyEventData(keyCode));
}
}

#ifdef STARFISH_ANDROID

static jmethodID GetJMethod(JNIEnv* env, jclass clazz, const char name[],
                            const char signature[])
{
    jmethodID m = env->GetStaticMethodID(clazz, name, signature);
    if (!m) {
        LOGE("Could not find Java method %s\n", name);
    }
    return m;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_init(JNIEnv* env,
                                                        jobject thiz)
{
    LOGI(
        "Java_com_samsung_android_mobileservice_lwe_WebView_init called %p "
        "%p",
        env, thiz);

    if (g_jvm) {
        return;
    }
    env->GetJavaVM(&g_jvm);
    jclass clazz =
        env->FindClass("com/samsung/android/mobileservice/lwe/WebView");
    g_WindowGlue.m_clazz = (jclass)env->NewGlobalRef(clazz);
    g_WindowGlue.m_env = env;
    g_WindowGlue.m_startTimer = GetJMethod(env, clazz, "startTimer", "(III)I");
    g_WindowGlue.m_cancelTimer = GetJMethod(env, clazz, "cancelTimer", "(I)V");

    g_WindowGlue.m_requestRender =
        env->GetMethodID(clazz, "requestRender", "()V");
    g_WindowGlue.m_onLoadResource =
        env->GetMethodID(clazz, "onLoadResource", "(Ljava/lang/String;)V");
    g_WindowGlue.m_onReceivedError =
        env->GetMethodID(clazz, "onReceivedError", "(IZZ)V");
    g_WindowGlue.m_onPageFinished =
        env->GetMethodID(clazz, "onPageFinished", "(Ljava/lang/String;ZZ)V");
    g_WindowGlue.m_onPageStarted =
        env->GetMethodID(clazz, "onPageStarted", "(Ljava/lang/String;ZZ)V");

    env->DeleteLocalRef(clazz);

    LOGI("Java_com_samsung_android_mobileservice_lwe_WebView_init call end");
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_serviceQueueTimer(
    JNIEnv* env, jobject thiz, jint uid, jint fn, jint data)
{
    STARFISH_RELEASE_ASSERT(StarFish::isMainThread());
    TimerCallback tc = (TimerCallback)fn;
    bool ret = (*tc)(uid, (void*)data);
    return ret;
}

void callOnLoadResourceHandler(LWE::WebContainer* view, const char* url)
{
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            LOGE("Failed to attach");
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        LOGE("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    if (!env || !g_WindowGlue.m_onLoadResource) {
        LOGE("OnLoadResource error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    jstring jstr = env->NewStringUTF(url);
    env->CallVoidMethod(g_webViews[view].first, g_WindowGlue.m_onLoadResource,
                        jstr);
}

void callOnReceivedError(LWE::WebContainer* view, int errorCode, bool canGoBack,
                         bool canGoForward)
{
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            LOGE("Failed to attach");
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        LOGE("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    if (!env || !g_WindowGlue.m_onReceivedError) {
        LOGE("OnPageStarted error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    jint jint1 = errorCode;
    jboolean jboolean1 = canGoBack;
    jboolean jboolean2 = canGoForward;
    env->CallVoidMethod(g_webViews[view].first, g_WindowGlue.m_onReceivedError,
                        jint1, jboolean1, jboolean2);
}
void callOnPageFinished(LWE::WebContainer* view, const char* url,
                        bool canGoBack, bool canGoForward)
{
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            LOGE("Failed to attach");
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        LOGE("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    if (!env || !g_WindowGlue.m_onPageFinished) {
        LOGE("OnPageFinished error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    jstring jstr = env->NewStringUTF(url);
    jboolean jboolean1 = canGoBack;
    jboolean jboolean2 = canGoForward;
    env->CallVoidMethod(g_webViews[view].first, g_WindowGlue.m_onPageFinished,
                        jstr, jboolean1, jboolean2);
}
void callOnPageStarted(LWE::WebContainer* view, const char* url, bool canGoBack,
                       bool canGoForward)
{
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            LOGE("Failed to attach");
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        LOGE("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    if (!env || !g_WindowGlue.m_onPageStarted) {
        LOGE("OnPageStarted error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    jstring jstr = env->NewStringUTF(url);
    jboolean jboolean1 = canGoBack;
    jboolean jboolean2 = canGoForward;

    env->CallVoidMethod(g_webViews[view].first, g_WindowGlue.m_onPageStarted,
                        jstr, jboolean1, jboolean2);
}

int startTimer(int ms, TimerCallback pointer, void* data)
{
    JNIEnv* env = g_WindowGlue.m_env;

    // double check it's all ok
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        // std::cout << "GetEnv: not attached" << std::endl;
        // LOGE("GetEnv: not attached");
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            // std::cout << "Failed to attach" << std::endl;
            LOGE("Failed to attach");
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        LOGE("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    if (!env || !g_WindowGlue.m_startTimer) {
        LOGE("signalQueueTimer error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    int ret = env->CallStaticIntMethod(g_WindowGlue.m_clazz,
                                       g_WindowGlue.m_startTimer, ms,
                                       (long)pointer, (long)data);

    return ret;
}

void cancelTimer(int uid)
{
    JNIEnv* env = g_WindowGlue.m_env;

    // double check it's all ok
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        // std::cout << "GetEnv: not attached" << std::endl;
        // LOGE("GetEnv: not attached");
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            // std::cout << "Failed to attach" << std::endl;
            LOGE("Failed to attach");
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        LOGE("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    // LOGE("cancelTimer");
    if (!env || !g_WindowGlue.m_cancelTimer) {
        LOGE("cancel error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    env->CallStaticVoidMethod(g_WindowGlue.m_clazz, g_WindowGlue.m_cancelTimer,
                              uid);
}

void requestRender(void* view)
{
    JNIEnv* env = g_WindowGlue.m_env;

    // double check it's all ok
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        // std::cout << "GetEnv: not attached" << std::endl;
        // LOGE("GetEnv: not attached");
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            // std::cout << "Failed to attach" << std::endl;
            LOGE("Failed to attach");
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        LOGE("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    if (!env || !g_WindowGlue.m_requestRender) {
        LOGE("reuqest render error");
        return;
    }
    env->CallVoidMethod(g_webViews[(LWE::WebContainer*)view].first,
                        g_WindowGlue.m_requestRender);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_Create(
    JNIEnv* env, jobject thiz, jint w, jint h, jfloat devicePixelRatio,
    jstring jua)
{
    LWE::WebContainer* webContainer =
        LWE::WebContainer::Create(nullptr, w, h, 0, devicePixelRatio);

    webContainer->RegisterOnReceivedErrorHandler(
        [](LWE::WebContainer* view, LWE::ResourceError error) -> void {
            callOnReceivedError(view, error.GetErrorCode(), view->CanGoBack(),
                                view->CanGoBack());
        });

    webContainer->RegisterOnPageFinishedHandler(
        [](LWE::WebContainer* view, const std::string& url) -> void {
            callOnPageFinished(view, url.c_str(), view->CanGoBack(),
                               view->CanGoBack());
        });

    webContainer->RegisterOnPageStartedHandler(
        [](LWE::WebContainer* view, const std::string& url) -> void {
            callOnPageStarted(view, url.c_str(), view->CanGoBack(),
                              view->CanGoBack());
        });

    webContainer->RegisterOnLoadResourceHandler(
        [](LWE::WebContainer* view, const std::string& url) -> void {
            callOnLoadResourceHandler(view, url.c_str());
        });

    jobject java_webview = env->NewGlobalRef(thiz);
    g_webViews.insert(
        std::make_pair(webContainer, std::make_pair(java_webview, nullptr)));

    return (jlong)webContainer;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_Destroy(JNIEnv* env,
                                                           jobject thiz,
                                                           jlong wv)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->Destroy();
    env->DeleteGlobalRef(g_webViews[webContainer].first);
    //    delete (g_webViews[webView].second);
    g_webViews.erase(webContainer);
    delete webContainer;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_resizeWebView(JNIEnv* env,
                                                                 jobject thiz,
                                                                 jlong sf,
                                                                 jint w, jint h)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)sf;
    webContainer->UpdateBuffer(nullptr, w, h, 0);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_loadUrl(JNIEnv* env,
                                                           jobject thiz,
                                                           jlong wv,
                                                           jstring url)
{
    const char* nativeString = env->GetStringUTFChars(url, 0);
    std::string urlString = std::string(nativeString);
    env->ReleaseStringUTFChars(url, nativeString);

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->LoadURL(urlString);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_loadData(JNIEnv* env,
                                                            jobject thiz,
                                                            jlong wv,
                                                            jstring data)
{
    const char* nativeString = env->GetStringUTFChars(data, 0);
    std::string dataString = std::string(nativeString);
    env->ReleaseStringUTFChars(data, nativeString);

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->LoadData(dataString);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_EvaluateJavaScript(
    JNIEnv* env, jobject thiz, jlong wv, jstring data)
{
    const char* nativeString = env->GetStringUTFChars(data, 0);
    std::string dataString = std::string(nativeString);
    env->ReleaseStringUTFChars(data, nativeString);

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    std::string result = webContainer->EvaluateJavaScript(dataString);
    jstring jstr = env->NewStringUTF(result.c_str());
    return jstr;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_getDefaultUserAgent(
    JNIEnv* env, jobject thiz)
{
    std::string result = USER_AGENT(STARFISH_NAME, VERSION);
    jstring jstr = env->NewStringUTF(result.c_str());
    return jstr;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_GoBack(JNIEnv* env,
                                                          jobject thiz,
                                                          jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->GoBack();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_GoForward(JNIEnv* env,
                                                             jobject thiz,
                                                             jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->GoForward();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_Reload(JNIEnv* env,
                                                          jobject thiz,
                                                          jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->Reload();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_StopLoading(JNIEnv* env,
                                                               jobject thiz,
                                                               jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->StopLoading();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_ClearHistory(JNIEnv* env,
                                                                jobject thiz,
                                                                jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->ClearHistory();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_addJavascriptInterface(
    JNIEnv* env, jobject thiz, jlong wv, jstring objName, jstring funtionName,
    jobject instance)
{
    const char* nativeString1 = env->GetStringUTFChars(objName, 0);
    std::string objNameString = std::string(nativeString1);
    env->ReleaseStringUTFChars(objName, nativeString1);

    const char* nativeString2 = env->GetStringUTFChars(funtionName, 0);
    std::string functionNameString = std::string(nativeString2);
    env->ReleaseStringUTFChars(funtionName, nativeString2);

    jobject callback_obj = env->NewGlobalRef(instance);
    jclass clz = env->GetObjectClass(callback_obj);
    jmethodID callback_methodID = env->GetMethodID(
        clz, nativeString2, "(Ljava/lang/String;)Ljava/lang/String;");

    std::shared_ptr<_jobject> javaObjectRef(callback_obj, [](jobject ref) {
        JNIEnv* env = g_WindowGlue.m_env;

        int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
        if (getEnvStat == JNI_EDETACHED) {
            if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        } else if (getEnvStat == JNI_OK) {
        } else if (getEnvStat == JNI_EVERSION) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        if (!env) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        env->DeleteGlobalRef(ref);
    });

    std::function<std::string(const std::string&)> NB =
        [javaObjectRef, callback_obj, clz,
         callback_methodID](const std::string& param) -> std::string {

        JNIEnv* env = g_WindowGlue.m_env;

        int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
        if (getEnvStat == JNI_EDETACHED) {
            if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        } else if (getEnvStat == JNI_OK) {
        } else if (getEnvStat == JNI_EVERSION) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        if (!env) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        jstring result = (jstring)env->CallObjectMethod(
            callback_obj, callback_methodID, env->NewStringUTF(param.c_str()));

        const char* nativeString3 = env->GetStringUTFChars(result, 0);
        std::string resultStr = std::string(nativeString3);
        env->ReleaseStringUTFChars(result, nativeString3);

        return resultStr;
    };

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->AddJavaScriptInterface(objNameString, functionNameString, NB);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_removeJavascriptInterface(
    JNIEnv* env, jobject thiz, jlong wv, jstring objName)
{
    const char* nativeString = env->GetStringUTFChars(objName, 0);
    const std::string objectName(nativeString);
    env->ReleaseStringUTFChars(objName, nativeString);

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->RemoveJavascriptInterface(objectName, nullptr);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_rendering(JNIEnv* env,
                                                             jobject thiz,
                                                             jlong wv,
                                                             jobject bitmap)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;

    int ret;
    AndroidBitmapInfo info;
    if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
        return;
    }

    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("Bitmap format is not RGBA_8888 !");
        return;
    }

    void* pixels;
    if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
        LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
    }

    webContainer->UpdateBuffer(pixels, info.width, info.height, info.stride);
    webContainer->RenderingDirectly();

    AndroidBitmap_unlockPixels(env, bitmap);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_setUserAgentString(
    JNIEnv* env, jobject thiz, jlong wv, jstring userAgent)
{
    const char* nativeString = env->GetStringUTFChars(userAgent, 0);
    StarFish::String* uaString = StarFish::String::fromUTF8(nativeString);
    env->ReleaseStringUTFChars(userAgent, nativeString);

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    // ((StarFish::StarFish*)webView->getInternalPtr())
    //     ->setCustomUserAgentString(uaString);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_setCacheMode(JNIEnv* env,
                                                                jobject thiz,
                                                                jlong wv,
                                                                jint mode)
{
#ifdef STARFISH_ENABLE_HTTPCACHE
    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
// ((StarFish::StarFish*)webContainer->getInternalPtr())
//     ->httpCache()
//     ->setCacheMode(mode);
#endif
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_ClearCache(JNIEnv* env,
                                                              jobject thiz,
                                                              jlong wv)
{
#ifdef STARFISH_ENABLE_HTTPCACHE
// LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
// ((StarFish::StarFish*)webContainer->getInternalPtr())->httpCache()->clear();
#endif
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_dispatchMouseDown(
    JNIEnv* env, jobject thiz, jlong data, jfloat x, jfloat y)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->DispatchMouseDownEvent(
        LeftButton, MouseButtonsValue::LeftButtonDown, x, y);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_dispatchMouseMove(
    JNIEnv* env, jobject thiz, jlong data, jfloat x, jfloat y,
    bool isLButtonPressed, bool isRButtonPressed)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->DispatchMouseMoveEvent(
        isLButtonPressed ? MouseButtonValue::LeftButton
                         : MouseButtonValue::NoButton,
        isLButtonPressed ? MouseButtonsValue::LeftButtonDown
                         : MouseButtonsValue::NoButtonDown,
        x, y);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_dispatchMouseUp(
    JNIEnv* env, jobject thiz, jlong data, jfloat x, jfloat y)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->DispatchMouseUpEvent(MouseButtonValue::NoButton,
                                       MouseButtonsValue::NoButtonDown, x, y);
}

#endif
