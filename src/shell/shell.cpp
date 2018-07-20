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

#if defined(STARFISH_DALI_TBMSURFACE)
#include <tbm_surface.h>
#endif

#include <dali-toolkit/dali-toolkit.h>
//#include <dali-toolkit/devel-api/controls/web-view-lite/web-view-lite.h>
#include "platform/window/PlatformWindow.h"

#include "core/dom/MouseEvent.h"
#include "core/dom/TouchEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "platform/event/PlatformKeyEventData.h"
#include <uv.h>

bool isNeedsUpdate = false;
bool isFirstTime = true;

struct DaliStarFishBinder {
    void* lweInstance;
    void* buffer;
    std::list<size_t> asyncHandlePool;
    std::string url;
    int w, h, s;
    bool canGoBack, canGoForward;
    bool isRunning;
    std::function<void(LWE::WebContainer*, void*)> onRenderedHandler;
    std::function<void(LWE::WebContainer*, LWE::ResourceError)> onReceivedError;
    std::function<void(LWE::WebContainer*, const std::string&)>
        onPageFinishedHandler;
    std::function<void(LWE::WebContainer*, const std::string&)>
        onPageStartedHandler;
    std::function<void(LWE::WebContainer*, const std::string&)>
        onLoadResourceHandler;
    DaliStarFishBinder()
        : lweInstance(nullptr)
        , buffer(nullptr)
        , w(0)
        , h(0)
        , s(0)
        , canGoBack(false)
        , canGoForward(false)
        , isRunning(false)
    {
    }
};

extern "C" void startMainThreadIfNeeds(pthread_t& t);
extern "C" void createInstance(DaliStarFishBinder* binder);
extern "C" void loadURL(DaliStarFishBinder* binder, const std::string& url);
extern "C" void setSize(DaliStarFishBinder* binder);
extern "C" void loadData(DaliStarFishBinder* binder, const std::string& d);
extern "C" void reload(DaliStarFishBinder* binder);
extern "C" void stopLoading(DaliStarFishBinder* binder);
extern "C" void goBack(DaliStarFishBinder* binder);
extern "C" void goForward(DaliStarFishBinder* binder);
extern "C" void addJavaScriptInterface(
    DaliStarFishBinder* binder, const std::string& exposedObjectName,
    const std::string& jsFunctionName,
    std::function<std::string(const std::string&)> cb);
extern "C" void evaluateJavaScript(DaliStarFishBinder* binder,
                                   const std::string& script);
extern "C" void clearHistory(DaliStarFishBinder* binder);
extern "C" void destroy(DaliStarFishBinder* binder);
extern "C" void removeJavascriptInterface(DaliStarFishBinder* binder,
                                          const std::string& exposedObjectName,
                                          const std::string& jsFunctionName);
extern "C" void clearCache(DaliStarFishBinder* binder);
extern "C" void stopLoop(DaliStarFishBinder* binder);

extern "C" void registerOnRenderedHandler(
    DaliStarFishBinder* binder,
    const std::function<void(LWE::WebContainer* c, void* buf)>& callback);
extern "C" void registerOnPageStartedHandler(
    DaliStarFishBinder* binder,
    const std::function<void(LWE::WebContainer*, const std::string&)>&
        callback);
extern "C" void registerOnReceivedErrorHandler(
    DaliStarFishBinder* binder,
    const std::function<void(LWE::WebContainer*, LWE::ResourceError)>&
        callback);
extern "C" void registerOnPageFinishedHandler(
    DaliStarFishBinder* binder,
    const std::function<void(LWE::WebContainer*, const std::string&)>&
        callback);

extern "C" void dispatchMouseDownEvent(DaliStarFishBinder* binder, float x,
                                       float y);
extern "C" void dispatchMouseUpEvent(DaliStarFishBinder* binder, float x,
                                     float y);
extern "C" void dispatchMouseMoveEvent(DaliStarFishBinder* binder, float x,
                                       float y, bool isLButtonPressed,
                                       bool isRButtonPressed);
extern "C" void dispatchKeyDownEvent(DaliStarFishBinder* binder,
                                     KeyValue keyCode);
extern "C" void dispatchKeyPressEvent(DaliStarFishBinder* binder,
                                      KeyValue keyCode);
extern "C" void dispatchKeyUpEvent(DaliStarFishBinder* binder,
                                   KeyValue keyCode);

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

class DaliShellController : public ConnectionTracker {
public:
    DaliShellController(Application& application, int width, int height,
                        char* url)
        : mWidth(width)
        , mHeight(height)
        , mApplication(application)
        , mLWEBinder(nullptr)
        , mUrl(url)
        , mIsMouseLbuttonDown(false)
#if defined(STARFISH_DALI_TBMSURFACE)
        , tbmSurface(NULL)
#endif
    {
        mApplication.InitSignal().Connect(this, &DaliShellController::Create);
    }
    ~DaliShellController()
    {
        STARFISH_LOG_INFO("[Dali Shell] ~DaliShellController()\n");
        mTimer.TickSignal().Disconnect(this,
                                       &DaliShellController::updateBuffer);
        mTimer.Stop();
    }

    void InnerCreate(Application& application);
    void Create(Application& application);

    int mWidth;
    int mHeight;
    Application& mApplication;
    Dali::Toolkit::ImageView mImageView;
    DaliStarFishBinder* mLWEBinder;
    Dali::Toolkit::PushButton mCreateButton;
    Dali::Toolkit::PushButton mRemoveButton;
    pthread_t mThreadHandle;
    std::string mUrl;

    bool mIsMouseLbuttonDown;
    Dali::Timer mTimer;

#if defined(STARFISH_DALI_TBMSURFACE)
    Dali::NativeImageSourcePtr nativeImageSrc;
    Dali::NativeImage nativeImage;
    tbm_surface_h tbmSurface;
    tbm_surface_info_s tbmSurfaceInfo;
#else
    Dali::BufferImage bufferImage;
#endif

private:
    void OnKeyEvent(const Dali::KeyEvent& event);
    bool OnCreateButton(Toolkit::Button button)
    {
        if (mLWEBinder == nullptr || mLWEBinder->isRunning == false) {
            STARFISH_ASSERT(mApplication);
            InnerCreate(mApplication);
        }
        return true;
    }

    bool OnRemoveButton(Toolkit::Button button)
    {
        if (mLWEBinder && mLWEBinder->isRunning == true) {
            mLWEBinder->isRunning = false;
            Dali::Stage::GetCurrent().Remove(mImageView);

            destroy(mLWEBinder);
        }
        return true;
    }

    bool touchEventHandler(Dali::Actor actor, const Dali::TouchData& data)
    {
        STARFISH_LOG_INFO("[StarFish] touchEventHandler()\n");
        size_t pointCount = data.GetPointCount();
        if (pointCount == 1) {
            // Single touch event
            Dali::PointState::Type pointState = data.GetState(0);
            const Dali::Vector2& screen = data.GetLocalPosition(0);

            if (pointState == Dali::PointState::DOWN) {
                dispatchMouseDownEvent(mLWEBinder, screen.x, screen.y);
                mIsMouseLbuttonDown = true;
            } else if (pointState == Dali::PointState::UP) {
                dispatchMouseUpEvent(mLWEBinder, screen.x, screen.y);
                mIsMouseLbuttonDown = false;
            } else {
                dispatchMouseMoveEvent(mLWEBinder, screen.x, screen.y,
                                       mIsMouseLbuttonDown, false);
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
            dispatchKeyDownEvent(mLWEBinder, keyValue);
            dispatchKeyPressEvent(mLWEBinder, keyValue);
        } else if (event.state == Dali::KeyEvent::Up) {
            dispatchKeyUpEvent(mLWEBinder, keyValue);
        }

        return true;
    }

    bool updateBuffer()
    {
        if (!mLWEBinder || mLWEBinder->isRunning == false) {
            return true;
        }
        if (isNeedsUpdate) {
#if defined(STARFISH_DALI_TBMSURFACE)
            Dali::Stage::GetCurrent().KeepRendering(0.01f);
#else
            if (!bufferImage) {
                return false;
            }
            bufferImage.Update();
#endif
            isNeedsUpdate = false;
        }
        return true;
    }
};

void DaliShellController::Create(Application& application)
{
    InnerCreate(application);
    mCreateButton = Dali::Toolkit::PushButton::New();
    mCreateButton.SetBackgroundColor(Vector4(1.0f, 0.0f, 0.0f, 0.7f));
    mCreateButton.SetProperty(Toolkit::Button::Property::TOGGLABLE, true);
    mCreateButton.SetProperty(Toolkit::Button::Property::LABEL,
                              "Create Web-View");
    mCreateButton.SetParentOrigin(Dali::ParentOrigin::TOP_LEFT);
    mCreateButton.SetAnchorPoint(Dali::AnchorPoint::TOP_LEFT);
    mCreateButton.SetSize(300, 50);
    mCreateButton.SetPosition(0, 600);
    mCreateButton.StateChangedSignal().Connect(
        this, &DaliShellController::OnCreateButton);
    Dali::Stage::GetCurrent().Add(mCreateButton);

    mRemoveButton = Dali::Toolkit::PushButton::New();
    mRemoveButton.SetBackgroundColor(Vector4(0.0f, 1.0f, 0.0f, 0.7f));
    mRemoveButton.SetProperty(Toolkit::Button::Property::TOGGLABLE, true);
    mRemoveButton.SetProperty(Toolkit::Button::Property::LABEL,
                              "Remove Web-View");
    mRemoveButton.SetParentOrigin(Dali::ParentOrigin::TOP_LEFT);
    mRemoveButton.SetAnchorPoint(Dali::AnchorPoint::TOP_LEFT);
    mRemoveButton.SetSize(300, 50);
    mRemoveButton.SetPosition(0, 650);
    mRemoveButton.StateChangedSignal().Connect(
        this, &DaliShellController::OnRemoveButton);
    Dali::Stage::GetCurrent().Add(mRemoveButton);

    mTimer = Dali::Timer::New(20);
    mTimer.TickSignal().Connect(this, &DaliShellController::updateBuffer);
    mTimer.Start();
}

void DaliShellController::InnerCreate(Application& application)
{
    STARFISH_LOG_INFO("[Dali Shell] Create() start\n");

    startMainThreadIfNeeds(mThreadHandle);

    if (!mLWEBinder) {
        mLWEBinder = new DaliStarFishBinder();
    }
    mLWEBinder->isRunning = true;

    int width = mWidth;
    int height = mHeight;

    mLWEBinder->w = width;
    mLWEBinder->h = height;
    mLWEBinder->s = width * 4;

    if (isFirstTime == true) {
        isFirstTime = false;
        mImageView = Dali::Toolkit::ImageView::New();
        mImageView.SetParentOrigin(Dali::ParentOrigin::TOP_LEFT);
        mImageView.SetAnchorPoint(Dali::AnchorPoint::TOP_LEFT);

        Stage::GetCurrent().KeyEventSignal().Connect(
            this, &DaliShellController::OnKeyEvent);

        ((Dali::Toolkit::ImageView)mImageView)
            .TouchSignal()
            .Connect(this, &DaliShellController::touchEventHandler);
        ((Dali::Toolkit::ImageView)mImageView)
            .KeyEventSignal()
            .Connect(this, &DaliShellController::keyEventHandler);

        mLWEBinder->onRenderedHandler = [this](LWE::WebContainer* c,
                                               void* buf) {
            STARFISH_LOG_INFO("[Dali Shell] onRenderedHandler()\n");
            int w = c->width();
            int h = c->height();
            if (mLWEBinder->w != w || mLWEBinder->h != h) {
                return;
            }
#if defined(STARFISH_DALI_TBMSURFACE)
            memcpy(tbmSurfaceInfo.planes[0].ptr, buf,
                   mLWEBinder->w * mLWEBinder->h * sizeof(uint32_t));
#else
            memcpy(bufferImage.GetBuffer(), buf,
                   mLWEBinder->w * mLWEBinder->h * sizeof(uint32_t));
#endif
            isNeedsUpdate = true;
        };

        mLWEBinder->onReceivedError = [](LWE::WebContainer* container,
                                         LWE::ResourceError error) {
            STARFISH_LOG_INFO("[Dali Shell] onReceivedError()\n");
        };
        mLWEBinder->onPageStartedHandler = [](LWE::WebContainer* container,
                                              const std::string& url) {
            STARFISH_LOG_INFO("[Dali Shell] onPageStartedHandler()\n");
        };
        mLWEBinder->onPageFinishedHandler = [](LWE::WebContainer* container,
                                               const std::string& url) {
            STARFISH_LOG_INFO("[Dali Shell] onPageFinishedHandler()\n");
        };
        mLWEBinder->onLoadResourceHandler = [](LWE::WebContainer* container,
                                               const std::string& url) {
            STARFISH_LOG_INFO("[Dali Shell] onLoadResourceHandler()\n");
        };
    }

#if defined(STARFISH_DALI_TBMSURFACE)
    tbmSurface = tbm_surface_create(width, height, TBM_FORMAT_ARGB8888);
    if (tbm_surface_map(tbmSurface,
                        TBM_SURF_OPTION_READ | TBM_SURF_OPTION_WRITE,
                        &tbmSurfaceInfo) != TBM_SURFACE_ERROR_NONE) {
        STARFISH_LOG_INFO("Fail to map tbm_surface\n");
    }

    Dali::Any source(tbmSurface);
    nativeImageSrc = Dali::NativeImageSource::New(source);
    nativeImage = Dali::NativeImage::New(*nativeImageSrc);
    nativeImageSrc->SetSource(source);
    ((Dali::Toolkit::ImageView)mImageView).SetImage(nativeImage);
#else
    bufferImage = Dali::BufferImage::New(width, height, Dali::Pixel::BGRA8888);
    STARFISH_LOG_INFO("[Dali Shell] [Dali BufImg:%p]\n", bufferImage);
    ((Dali::Toolkit::ImageView)mImageView).SetImage(bufferImage);
#endif

    Dali::Stage::GetCurrent().Add(mImageView);

    STARFISH_LOG_INFO("[Dali Shell] createInstance()\n");
    createInstance(mLWEBinder);

    STARFISH_LOG_INFO("[Dali Shell] loadURL() : %s\n", mUrl.c_str());
    loadURL(mLWEBinder, mUrl);
}

void DaliShellController::OnKeyEvent(const Dali::KeyEvent& event)
{
    STARFISH_ASSERT(mLWEBinder);
    DaliStarFishBinder* binder = (DaliStarFishBinder*)mLWEBinder;

    STARFISH_LOG_INFO("[Dali Shell] key pressed [%d]\n", event.keyCode);

    if (event.state == KeyEvent::Up) {
        if (IsKey(event, DALI_KEY_ESCAPE) || IsKey(event, DALI_KEY_BACK)) {
            if (mLWEBinder->isRunning == true) {
                mLWEBinder->isRunning = false;
                Dali::Stage::GetCurrent().Remove(mImageView);

                destroy(mLWEBinder);
            }
            stopLoop((DaliStarFishBinder*)mLWEBinder);
            int status;
            pthread_join(mThreadHandle, (void**)&status);

            free(mLWEBinder);
            mLWEBinder = nullptr;
            mApplication.Quit();
        } else {
            // F1
            if (event.keyCode == 67) {
                if (binder->w != 800) {
                    STARFISH_LOG_INFO("[Dali Shell] setSize(800 * 600)\n");
                    binder->w = 800;
                    binder->h = 600;
                    binder->s = 800 * 4;
#if defined(STARFISH_DALI_TBMSURFACE)
                    if (tbmSurface != NULL) {
                        if (tbm_surface_unmap(tbmSurface) !=
                            TBM_SURFACE_ERROR_NONE) {
                            STARFISH_LOG_INFO(
                                "[Dali Shell] Failed to unmap tbm_surface\n");
                        }
                    }

                    tbmSurface = tbm_surface_create(binder->w, binder->h,
                                                    TBM_FORMAT_ARGB8888);
                    if (tbm_surface_map(tbmSurface, TBM_SURF_OPTION_READ |
                                                        TBM_SURF_OPTION_WRITE,
                                        &tbmSurfaceInfo) !=
                        TBM_SURFACE_ERROR_NONE) {
                        STARFISH_LOG_INFO(
                            "[Dali Shell] Fail to map tbm_surface\n");
                    }

                    Dali::Any source(tbmSurface);
                    nativeImageSrc->SetSource(source);
// ??? ((Dali::Toolkit::ImageView)mImageView).SetImage(nativeImage);
#else
                    bufferImage = Dali::BufferImage::New(binder->w, binder->h,
                                                         Dali::Pixel::BGRA8888);
                    ((Dali::Toolkit::ImageView)mImageView)
                        .SetImage(bufferImage);
                    STARFISH_LOG_INFO("[Dali Shell] [Dali BufImg:%p]\n",
                                      bufferImage);
#endif
                    mImageView.SetSize(binder->w, binder->h);
                    setSize(binder);
                }
                // F2
            } else if (event.keyCode == 68) {
                if (binder->w != 1280) {
                    STARFISH_LOG_INFO("[Dali Shell] setSize(1280 * 720)\n");
                    binder->w = 1280;
                    binder->h = 720;
                    binder->s = 1280 * 4;
#if defined(STARFISH_DALI_TBMSURFACE)
                    if (tbmSurface) {
                        if (tbm_surface_unmap(tbmSurface) !=
                            TBM_SURFACE_ERROR_NONE) {
                            STARFISH_LOG_INFO(
                                "[Dali Shell] Failed to unmap tbm_surface\n");
                        }
                    }

                    tbmSurface = tbm_surface_create(binder->w, binder->h,
                                                    TBM_FORMAT_ARGB8888);
                    if (tbm_surface_map(tbmSurface, TBM_SURF_OPTION_READ |
                                                        TBM_SURF_OPTION_WRITE,
                                        &tbmSurfaceInfo) !=
                        TBM_SURFACE_ERROR_NONE) {
                        STARFISH_LOG_INFO(
                            "[Dali Shell] Fail to map tbm_surface\n");
                    }

                    Dali::Any source(tbmSurface);
                    nativeImageSrc->SetSource(source);
// ??? ((Dali::Toolkit::ImageView)mImageView).SetImage(nativeImage);
#else
                    bufferImage = Dali::BufferImage::New(binder->w, binder->h,
                                                         Dali::Pixel::BGRA8888);
                    ((Dali::Toolkit::ImageView)mImageView)
                        .SetImage(bufferImage);
                    STARFISH_LOG_INFO("[Dali Shell] [Dali BufImg:%p]\n",
                                      bufferImage);
#endif
                    mImageView.SetSize(binder->w, binder->h);
                    setSize(binder);
                }
                // F3
            } else if (event.keyCode == 69) {
                std::string str =
                    "<!DOCTYPE html><html><body><div "
                    "style=\"width:200px;height:200px;background-color:green;"
                    "\"></div></body></html>";
                loadData(binder, str);
                // F4
            } else if (event.keyCode == 70) {
                stopLoading(binder);
                // F5
            } else if (event.keyCode == 71) {
                reload(binder);
                // F6
            } else if (event.keyCode == 72) {
                goBack(binder);
                // F7
            } else if (event.keyCode == 73) {
                goForward(binder);
                // F8
            } else if (event.keyCode == 74) {
                bool ret = binder->canGoBack;
                STARFISH_LOG_INFO("[Dali Shell] canGoBack() returns [%s]\n",
                                  ret ? "true" : "false");
                // F9
            } else if (event.keyCode == 75) {
                addJavaScriptInterface(
                    binder, "testObj", "testFunc",
                    [](const std::string& str) -> std::string {
                        return str + " world!!";
                    });
                evaluateJavaScript(binder, "testObj.testFunc('hello')");
                // F10
            } else if (event.keyCode == 76) {
                clearHistory(binder);
                // F11
            } else if (event.keyCode == 77) {
                removeJavascriptInterface(binder, "testObj", "testFunc");
                // F12
            } else if (event.keyCode == 78) {
                clearCache(binder);
            }
        }
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
#if defined(PORT_COMPOSITOR_BACKEND_GL)
    elm_config_accel_preference_set("opengl");
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
        } else if (strcmp(argv[i], "--debug-graphics-layer") == 0) {
            flag |= StarFish::enableDebugGraphicsLayer;
        } else if (strcmp(argv[i], "--debug-repaint-region") == 0) {
            flag |= StarFish::enableDebugRepaintRegion;
        } else if (strcmp(argv[i], "--pixel-test") == 0) {
#ifdef STARFISH_ENABLE_TEST
            StarFish::g_enablePixelTest = true;
            setenv("PIXEL_TEST", "1", 1);
#endif
        } else if (strcmp(argv[i], "--ref-test") == 0) {
#ifdef STARFISH_ENABLE_TEST
            StarFish::g_referenceTestState = 1;
            setenv("HIDE_WINDOW", "1", 1);
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

#if defined(STARFISH_DALI)
    Application application = Application::New(&argc, &argv);
    DaliShellController shell(application, width, height, argv[1]);
    application.MainLoop();
#else

    // TODO: Need to get screen info from X11.
    // Temporally, rect's width and height are set to window size.
    StarFish::ScreenInfo info;
    info.rect.setWidth(width);
    info.rect.setHeight(height);
    info.availableRect.setWidth(width);
    info.availableRect.setHeight(height);
#if defined(STARFISH_EFL_CAIRO) || defined(STARFISH_EFL_SKIA)
    info.devicePixelRatio = scaleFactor;
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

#if !defined(STARFISH_DALI)
    GC_gcollect_and_unmap();
#endif

    return 0;
}

#endif
