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
#include "DALiStarFishBinder.h"

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

class DaliShellController : public ConnectionTracker {
public:
    DaliShellController(Application& application, DaliStarFishBinder* binder,
                        pthread_t threadHandle, const std::string& url)
        : mApplication(application)
        , mLWEBinder(binder)
        , mThreadHandle(threadHandle)
        , mIsMouseLbuttonDown(false)
        , misNeedsUpdate(false)
        , mURL(url)
#if defined(STARFISH_DALI_TBMSURFACE)
        , mTbmSurface(NULL)
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

    Application& mApplication;
    Dali::Toolkit::ImageView mImageView;
    DaliStarFishBinder* mLWEBinder;
    pthread_t mThreadHandle;

    bool mIsMouseLbuttonDown;
    bool misNeedsUpdate;
    Dali::Timer mTimer;

    std::string mURL;

#if defined(STARFISH_DALI_TBMSURFACE)
    Dali::NativeImageSourcePtr mNativeImageSrc;
    Dali::NativeImage mNativeImage;
    tbm_surface_h mTbmSurface;
#else
    Dali::BufferImage mBufferImage;
#endif

private:
    void onKeyEvent(const Dali::KeyEvent& event);

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
        if (misNeedsUpdate) {
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
};

void DaliShellController::Create(Application& application)
{
    mLWEBinder->isRunning = true;
    mImageView = Dali::Toolkit::ImageView::New();
    mImageView.SetParentOrigin(Dali::ParentOrigin::TOP_LEFT);
    mImageView.SetAnchorPoint(Dali::AnchorPoint::TOP_LEFT);

    Stage::GetCurrent().KeyEventSignal().Connect(
        this, &DaliShellController::onKeyEvent);

    ((Dali::Toolkit::ImageView)mImageView)
        .TouchSignal()
        .Connect(this, &DaliShellController::touchEventHandler);
    ((Dali::Toolkit::ImageView)mImageView)
        .KeyEventSignal()
        .Connect(this, &DaliShellController::keyEventHandler);

    mLWEBinder->onRenderedHandler = [this](
        LWE::WebContainer* c,
        const LWE::WebContainer::RenderResult& renderResult) {
        STARFISH_LOG_INFO("[Dali Shell] onRenderedHandler()\n");
        int w = mLWEBinder->outputWidth;
        int h = mLWEBinder->outputHeight;
        uint8_t* buffer;
        size_t stride;
#if defined(STARFISH_DALI_TBMSURFACE)
        tbm_surface_info_s tbmSurfaceInfo;
        if (tbm_surface_map(mTbmSurface, TBM_SURF_OPTION_WRITE,
                            &tbmSurfaceInfo) != TBM_SURFACE_ERROR_NONE) {
            STARFISH_LOG_INFO("Fail to map tbm_surface\n");
            abort();
        }
        buffer = tbmSurfaceInfo.planes[0].ptr;
        stride = tbmSurfaceInfo.planes[0].stride;
#else
        buffer = mBufferImage.GetBuffer();
        stride = mBufferImage.GetBufferStride();
#endif

        for (auto y = renderResult.updatedY;
             y < (renderResult.updatedHeight + renderResult.updatedY); y++) {
            auto start = renderResult.updatedX;
            memcpy(buffer + (y * stride) + (start * 4),
                   ((uint8_t*)renderResult.updatedBufferAddress) + (y * w * 4) +
                       (start * 4),
                   renderResult.updatedWidth * sizeof(uint32_t));
        }
        misNeedsUpdate = true;

#if defined(STARFISH_DALI_TBMSURFACE)
        tbm_surface_unmap(mTbmSurface);
#endif
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

#if defined(STARFISH_DALI_TBMSURFACE)
    mTbmSurface = tbm_surface_create(
        mLWEBinder->outputWidth, mLWEBinder->outputHeight, TBM_FORMAT_ABGR8888);
    Dali::Any source(mTbmSurface);
    mNativeImageSrc = Dali::NativeImageSource::New(source);
    mNativeImage = Dali::NativeImage::New(*mNativeImageSrc);
    mNativeImageSrc->SetSource(source);
    ((Dali::Toolkit::ImageView)mImageView).SetImage(mNativeImage);
#else
    mBufferImage =
        Dali::BufferImage::New(mLWEBinder->outputWidth,
                               mLWEBinder->outputHeight, Dali::Pixel::RGBA8888);
    STARFISH_LOG_INFO("[Dali Shell] [Dali BufImg:%p]\n",
                      mBufferImage.GetBuffer());
    ((Dali::Toolkit::ImageView)mImageView).SetImage(mBufferImage);
#endif

    Dali::Stage::GetCurrent().Add(mImageView);

    loadURL(mLWEBinder, mURL);

    mTimer = Dali::Timer::New(20);
    mTimer.TickSignal().Connect(this, &DaliShellController::updateBuffer);
    mTimer.Start();
}

void DaliShellController::onKeyEvent(const Dali::KeyEvent& event)
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
                // F2
            } else if (event.keyCode == 68) {
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

namespace LWE {

class WebViewDALi : public WebView {
public:
    WebViewDALi(void* winArg, int x, int y, int width, int height,
                float devicePixelRatio, const char* locale,
                const char* timezoneID, const char* localStorageFilePath,
                const char* cookieStoreFilePath,
                const char* httpCacheDirectorypath)
        : WebView(nullptr)
    {
        startMainThreadIfNeeds(m_mainThreadHandle);

        auto binder = new DaliStarFishBinder();
        binder->url = "about:blank";
        binder->outputWidth = width;
        binder->outputHeight = height;
        binder->outputStride = width * sizeof(uint32_t);
        binder->outputBuffer = (uint8_t*)malloc(
            binder->outputWidth * binder->outputHeight * sizeof(uint32_t));
        createInstance(binder);

        m_LWEBinder = binder;
        while (true) {
            m_impl = binder->lweInstance;
            if (m_impl) {
                break;
            }
            usleep(100);
        }
    }
    virtual void Destroy() override
    {
        destroy(m_LWEBinder);
    }

    void RunMessageLoop() override
    {
        static int fakeArgc;
        static char** fakeArgv;
        Application application = Application::New(&fakeArgc, &fakeArgv);
        DaliShellController shell(application, m_LWEBinder, m_mainThreadHandle,
                                  m_url);
        application.MainLoop();
    }

    void StopMessageLoop() override
    {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    void LoadURL(const std::string& url) override
    {
        m_url = url;
    }

protected:
    DaliStarFishBinder* m_LWEBinder;
    pthread_t m_mainThreadHandle;
    std::string m_url;
    virtual ::LWE::WebContainer* FetchWebContainer() override
    {
        return (::LWE::WebContainer*)m_impl;
    }
};

WebView* WebView::Create(void* win, int x, int y, int width, int height,
                         float devicePixelRatio, const char* locale,
                         const char* timezoneID,
                         const char* localStorageFilePath,
                         const char* cookieStoreFilePath,
                         const char* httpCacheDirectorypath)
{
    return new WebViewDALi(win, x, y, width, height, devicePixelRatio, locale,
                           timezoneID, localStorageFilePath,
                           cookieStoreFilePath, httpCacheDirectorypath);
}
}

#endif
