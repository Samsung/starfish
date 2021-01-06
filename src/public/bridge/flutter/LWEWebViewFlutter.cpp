/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#include "StarfishConfig.h"
#include "LWEWebView.h"

#include "Starfish.h"

#if defined(PORT_WEBVIEW_BRIDGE_FLUTTER)
#define STARFISH_ENABLE_PROFILE_TIMER

#define EFL_BETA_API_SUPPORT
#include <Ecore_Wl2.h>
#undef EFL_BETA_API_SUPPORT
#include <wayland-egl-core.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "core/modules/threading/Thread.h"
#include "core/modules/message_loop/MessageLoop.h"

#ifdef STREAMLINE_PROFILE
#include "streamline_annotate.h"
#else
#define ANNOTATE_SETUP
#define ANNOTATE_CHANNEL_COLOR(channel, color, str)
#define ANNOTATE_CHANNEL_END(channel)
#define ANNOTATE_GREEN 0x00ff001b
#endif

typedef EGLSyncKHR(EGLAPIENTRYP PFNEGLCREATESYNCKHRPROC)(
    EGLDisplay dpy, EGLenum type, const EGLint* attrib_list);
typedef EGLBoolean(EGLAPIENTRYP PFNEGLDESTROYSYNCKHRPROC)(EGLDisplay dpy,
                                                          EGLSyncKHR sync);
typedef EGLint(EGLAPIENTRYP PFNEGLCLIENTWAITSYNCKHRPROC)(EGLDisplay dpy,
                                                         EGLSyncKHR sync,
                                                         EGLint flags,
                                                         EGLTimeKHR timeout);

static PFNEGLCREATESYNCKHRPROC g_eglCreateSyncKHRProc;
static PFNEGLDESTROYSYNCKHRPROC g_eglDestroySyncKHRProc;
static PFNEGLCLIENTWAITSYNCKHRPROC g_eglClientWaitSyncKHRProc;

const int g_arrowKeyDownMinimumDelayInMS = 150;
static int g_arrowKeyDownTimestamp[4];

namespace LWE {

class WebViewFlutter : public WebView {
public:
    WebViewFlutter(unsigned x, unsigned y, unsigned width, unsigned height,
                   float devicePixelRatio, const char* defaultFontName,
                   const char* locale, const char* timezoneID,
                   const std::function<WebContainer::ExternalImageInfo(void)>&
                       prepareImageCb,
                   const std::function<void(WebContainer*)>& renderedCb)
        : WebView(nullptr)
        , m_isMouseLbuttonDown(false)
        , m_isBufferSwapped(false)
        , m_hasFocus(true)
        , m_isShowing(false)
        , m_lastInputTime(0)
    {
        g_eglCreateSyncKHRProc = reinterpret_cast<PFNEGLCREATESYNCKHRPROC>(
            eglGetProcAddress("eglCreateSyncKHR"));
        g_eglDestroySyncKHRProc = reinterpret_cast<PFNEGLDESTROYSYNCKHRPROC>(
            eglGetProcAddress("eglDestroySyncKHR"));
        g_eglClientWaitSyncKHRProc =
            reinterpret_cast<PFNEGLCLIENTWAITSYNCKHRPROC>(
                eglGetProcAddress("eglClientWaitSyncKHR"));

        m_ecoreWlDisplay = ecore_wl2_display_connect(nullptr);
        m_wlDisplay = ecore_wl2_display_get(m_ecoreWlDisplay);

        EGLint major, minor, count, n, size;
        EGLConfig* configs;
        int i;
        EGLint config_attribs[] = { EGL_SURFACE_TYPE,
                                    EGL_WINDOW_BIT,
                                    EGL_RED_SIZE,
                                    8,
                                    EGL_GREEN_SIZE,
                                    8,
                                    EGL_BLUE_SIZE,
                                    8,
                                    EGL_ALPHA_SIZE,
                                    8,
                                    EGL_DEPTH_SIZE,
                                    0,
                                    EGL_STENCIL_SIZE,
                                    0,
                                    EGL_RENDERABLE_TYPE,
                                    EGL_OPENGL_ES2_BIT,
                                    EGL_NONE };

        m_display = eglGetDisplay((EGLNativeDisplayType)m_wlDisplay);
        if (m_display == EGL_NO_DISPLAY) {
            STARFISH_LOG_ERROR("Can't create egl display\n");
            exit(1);
        } else {
            STARFISH_LOG_INFO("Created egl display\n");
        }

        if (eglInitialize(m_display, &major, &minor) != EGL_TRUE) {
            STARFISH_LOG_ERROR("Can't initialise egl display\n");
            exit(1);
        }
        STARFISH_LOG_INFO("EGL major: %d, minor %d\n", major, minor);

        if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) {
            STARFISH_LOG_ERROR("Can't bind egl api\n");
        }

        eglGetConfigs(m_display, NULL, 0, &count);
        STARFISH_LOG_INFO("EGL has %d configs\n", count);

        configs = ALLOCA(count * sizeof(*configs), void*);

        eglChooseConfig(m_display, config_attribs, configs, count, &n);

        EGLConfig eglConf = configs[0];
        for (i = 0; i < n; i++) {
            eglGetConfigAttrib(m_display, configs[i], EGL_BUFFER_SIZE, &size);
            STARFISH_LOG_INFO("Buffer size for config %d is %d\n", i, size);
            eglGetConfigAttrib(m_display, configs[i], EGL_RED_SIZE, &size);
            STARFISH_LOG_INFO("Red size for config %d is %d\n", i, size);
            eglGetConfigAttrib(m_display, configs[i], EGL_GREEN_SIZE, &size);
            STARFISH_LOG_INFO("Green size for config %d is %d\n", i, size);
            eglGetConfigAttrib(m_display, configs[i], EGL_BLUE_SIZE, &size);
            STARFISH_LOG_INFO("Blue size for config %d is %d\n", i, size);
            eglGetConfigAttrib(m_display, configs[i], EGL_ALPHA_SIZE, &size);
            STARFISH_LOG_INFO("Alpha size for config %d is %d\n", i, size);
            eglGetConfigAttrib(m_display, configs[i], EGL_STENCIL_SIZE, &size);
            STARFISH_LOG_INFO("Stencil size for config %d is %d\n", i, size);
            eglGetConfigAttrib(m_display, configs[i], EGL_DEPTH_SIZE, &size);
            STARFISH_LOG_INFO("Depth size for config %d is %d\n", i, size);
            // just choose the first one
            eglConf = configs[i];
            break;
        }

        // test version 3 first
        EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
        m_context = eglCreateContext(m_display, eglConf, EGL_NO_CONTEXT,
                                     contextAttribs);
        if (!m_context) {
            EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2,
                                        EGL_NONE };
            STARFISH_LOG_INFO(
                "failed to create opengl es 3+ context. use 2 instead\n");
            m_context = eglCreateContext(m_display, eglConf, EGL_NO_CONTEXT,
                                         contextAttribs);
        }

        {
            const EGLint attribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
            m_surface = eglCreatePbufferSurface(m_display, eglConf, attribs);
            if (m_surface == EGL_NO_SURFACE) {
                STARFISH_LOG_ERROR(
                    "eglCreatePbufferSurface fail error code:  %d\n ",
                    (int)eglGetError());
            }
        }

        if (eglMakeCurrent(m_display, m_surface, m_surface, m_context)) {
            STARFISH_LOG_INFO("Made current\n");
        } else {
            STARFISH_LOG_ERROR("Made current failed\n");
        }

        m_fence = nullptr;

        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glFlush();

        if (eglSwapBuffers(m_display, m_surface)) {
            STARFISH_LOG_INFO("Swapped buffers\n");
        } else {
            STARFISH_LOG_ERROR("Swapped buffers failed\n");
        }

        STARFISH_LOG_INFO("wl_display_dispatch few times\n");
        size_t dispatchCount = 0;
        while (dispatchCount < 3) {
            if (wl_display_dispatch_pending(m_wlDisplay) > 0) {
                wl_display_dispatch(m_wlDisplay);
            }
            dispatchCount++;
        }

        eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                       EGL_NO_CONTEXT);
        ::LWE::WebContainer* webContainer =
            ::LWE::WebContainer::CreateGLWithPlatformImage(
                width, height,
                [this](WebContainer* wc) {
                    if (m_isBufferSwapped) {
                        if (m_fence) {
                            Starfish::LongTaskFinder p(
                                "WebViewFlutter - eglClientWaitSyncKHRProc", 1);
                            EGLint result = g_eglClientWaitSyncKHRProc(
                                m_display, m_fence,
                                EGL_SYNC_FLUSH_COMMANDS_BIT_KHR,
                                EGL_FOREVER_KHR);
                            if (result == EGL_FALSE) {
                                STARFISH_LOG_ERROR(
                                    "EGL FENCE: error waiting for fence: %d\n",
                                    (int)eglGetError());
                            }
                            g_eglDestroySyncKHRProc(m_display, m_fence);
                            m_fence = nullptr;
                        }
                        m_isBufferSwapped = false;
                    }
                    {
                        Starfish::LongTaskFinder p(
                            "WebViewFlutter - eglMakeCurrent", 1);
                        if (!eglMakeCurrent(m_display, m_surface, m_surface,
                                            m_context)) {
                            auto eglError = eglGetError();
                            STARFISH_LOG_ERROR(
                                "Made current failed error -> %d\n",
                                (int)eglError);
                        }
                    }
                },
                [this](WebContainer* wc, bool mayNeedsSync) {
                    {
                        Starfish::LongTaskFinder p(
                            "WebViewFlutter - eglSwapBuffers", 2);
                        if (!eglSwapBuffers(m_display, m_surface)) {
                            auto eglError = eglGetError();
                            STARFISH_LOG_ERROR(
                                "Made current failed error -> %d\n",
                                (int)eglError);
                        }
                    }
                    if (m_lastInputTime) {
                        ANNOTATE_SETUP;
                        ANNOTATE_CHANNEL_COLOR(3002, ANNOTATE_GREEN,
                                               "response time");
#ifdef STARFISH_ENABLE_PROFILE_TIMER
                        uint64_t end = Starfish::longTickCount();
                        float time = (float)((end - m_lastInputTime) / 1000.f);
                        STARFISH_LOG_INFO("response time is %f ms\n", time);
#endif
                        m_lastInputTime = 0;
                        ANNOTATE_CHANNEL_END(3002);
                    }
                    if (mayNeedsSync) {
                        m_fence = g_eglCreateSyncKHRProc(
                            m_display, EGL_SYNC_FENCE_KHR, NULL);
                        if (!m_fence) {
                            STARFISH_LOG_ERROR("eglCreateSyncKHR Error: %d\n",
                                               (int)eglGetError());
                        }
                    }
                    m_isBufferSwapped = true;
                },
                prepareImageCb, renderedCb, devicePixelRatio, defaultFontName,
                locale, timezoneID);
        m_impl = webContainer;
    }

    virtual void Destroy() override
    {
        Blur();

        FetchWebContainer()->Destroy();

        if (m_fence) {
            g_eglDestroySyncKHRProc(m_display, m_fence);
        }
        eglDestroySurface(m_display, m_surface);
        eglDestroyContext(m_display, m_context);
        eglTerminate(m_display);
        ecore_wl2_display_disconnect(m_ecoreWlDisplay);
        delete this;
    }

    virtual void Focus() override
    {
        WebView::Focus();

        m_hasFocus = true;
    }

    virtual void Blur() override
    {
        WebView::Blur();

        m_hasFocus = false;
    }

    bool m_isMouseLbuttonDown;
    bool m_isBufferSwapped;
    bool m_hasFocus;
    bool m_isShowing;
    uint64_t m_lastInputTime;
    wl_display* m_wlDisplay;
    EGLDisplay m_display;
    EGLSurface m_surface;
    EGLContext m_context;
    EGLSyncKHR m_fence;
    Ecore_Wl2_Display* m_ecoreWlDisplay;

    virtual ::LWE::WebContainer* FetchWebContainer() override
    {
        return (::LWE::WebContainer*)m_impl;
    }
};
WebView* WebView::Create(void* win, unsigned x, unsigned y, unsigned width,
                         unsigned height, float devicePixelRatio,
                         const char* defaultFontName, const char* locale,
                         const char* timezoneID)
{
    // This is only for executable build. Flutter doesn't support executable
    // mode.
    return new WebViewFlutter(
        x, y, width, height, devicePixelRatio, defaultFontName, locale,
        timezoneID,
        []() -> WebContainer::ExternalImageInfo {
            WebContainer::ExternalImageInfo result;
            result.imageAddress = nullptr;
            return result;
        },
        [](WebContainer* c) {});
}
} // namespace LWE

extern "C" size_t LWE_EXPORT createWebViewInstance(
    unsigned x, unsigned y, unsigned width, unsigned height,
    float devicePixelRatio, const char* defaultFontName, const char* locale,
    const char* timezoneID,
    const std::function<::LWE::WebContainer::ExternalImageInfo(void)>&
        prepareImageCb,
    const std::function<void(::LWE::WebContainer*)>& renderedCb)
{
    ::LWE::WebViewFlutter* wv = new ::LWE::WebViewFlutter(
        x, y, width, height, devicePixelRatio, defaultFontName, locale,
        timezoneID, prepareImageCb, renderedCb);
    return (size_t)wv->FetchWebContainer();
}
#endif
