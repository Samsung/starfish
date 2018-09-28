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

#include "StarFish.h"

#if defined(PORT_WEBVIEW_BRIDGE_WAYLAND)

#define EFL_BETA_API_SUPPORT
#include <Ecore_Wl2.h>
#undef EFL_BETA_API_SUPPORT
#include <wayland-egl-core.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>

namespace LWE {

class WebViewWayland : public WebView {
public:
    WebViewWayland(void* winArg, unsigned x, unsigned y, unsigned width,
                   unsigned height, float devicePixelRatio,
                   const char* defaultFontName, const char* locale,
                   const char* timezoneID)
        : WebView(nullptr)
        , m_isMouseLbuttonDown(false)
    {
        Ecore_Wl2_Window* win = (Ecore_Wl2_Window*)winArg;

        Ecore_Wl2_Display* ecoreWlDisplay = ecore_wl2_window_display_get(win);
        auto display = ecore_wl2_display_get(ecoreWlDisplay);
        mWlDisplay = display;
        auto wlSurface = ecore_wl2_window_surface_get(win);

        STARFISH_LOG_INFO("wl_display %p surface %p\n", display, wlSurface);

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
                                    EGL_DEPTH_SIZE,
                                    0,
                                    EGL_STENCIL_SIZE,
                                    8,
                                    EGL_RENDERABLE_TYPE,
                                    EGL_OPENGL_ES2_BIT,
                                    EGL_NONE };

        static const EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2,
                                                  EGL_NONE };

        mDisplay = eglGetDisplay((EGLNativeDisplayType)display);
        if (mDisplay == EGL_NO_DISPLAY) {
            STARFISH_LOG_INFO("Can't create egl display\n");
            exit(1);
        } else {
            STARFISH_LOG_INFO("Created egl display\n");
        }

        if (eglInitialize(mDisplay, &major, &minor) != EGL_TRUE) {
            STARFISH_LOG_INFO("Can't initialise egl display\n");
            exit(1);
        }
        STARFISH_LOG_INFO("EGL major: %d, minor %d\n", major, minor);

        eglGetConfigs(mDisplay, NULL, 0, &count);
        STARFISH_LOG_INFO("EGL has %d configs\n", count);

        configs = (void**)alloca(count * sizeof(*configs));

        eglChooseConfig(mDisplay, config_attribs, configs, count, &n);

        EGLConfig eglConf = configs[0];
        for (i = 0; i < n; i++) {
            eglGetConfigAttrib(mDisplay, configs[i], EGL_BUFFER_SIZE, &size);
            STARFISH_LOG_INFO("Buffer size for config %d is %d\n", i, size);
            eglGetConfigAttrib(mDisplay, configs[i], EGL_RED_SIZE, &size);
            STARFISH_LOG_INFO("Red size for config %d is %d\n", i, size);
            eglGetConfigAttrib(mDisplay, configs[i], EGL_GREEN_SIZE, &size);
            STARFISH_LOG_INFO("Green size for config %d is %d\n", i, size);
            eglGetConfigAttrib(mDisplay, configs[i], EGL_BLUE_SIZE, &size);
            STARFISH_LOG_INFO("Blue size for config %d is %d\n", i, size);
            eglGetConfigAttrib(mDisplay, configs[i], EGL_ALPHA_SIZE, &size);
            STARFISH_LOG_INFO("Alpha size for config %d is %d\n", i, size);
            eglGetConfigAttrib(mDisplay, configs[i], EGL_STENCIL_SIZE, &size);
            STARFISH_LOG_INFO("Stencil size for config %d is %d\n", i, size);
            eglGetConfigAttrib(mDisplay, configs[i], EGL_DEPTH_SIZE, &size);
            STARFISH_LOG_INFO("Depth size for config %d is %d\n", i, size);
            // just choose the first one
            eglConf = configs[i];
            break;
        }

        mContext = eglCreateContext(mDisplay, eglConf, EGL_NO_CONTEXT,
                                    context_attribs);

        mEglWindow = wl_egl_window_create(wlSurface, width, height);
        if (mEglWindow == EGL_NO_SURFACE) {
            STARFISH_LOG_INFO("Can't create egl window\n");
            exit(1);
        } else {
            STARFISH_LOG_INFO("Created egl window\n");
        }

        mSurface = eglCreateWindowSurface(mDisplay, eglConf, mEglWindow, NULL);

        if (eglMakeCurrent(mDisplay, mSurface, mSurface, mContext)) {
            STARFISH_LOG_INFO("Made current\n");
        } else {
            STARFISH_LOG_INFO("Made current failed\n");
        }

        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glFlush();

        if (eglSwapBuffers(mDisplay, mSurface)) {
            STARFISH_LOG_INFO("Swapped buffers\n");
        } else {
            STARFISH_LOG_INFO("Swapped buffers failed\n");
        }

        size_t dispatchCount = 0;
        while (dispatchCount < 3) {
            if (wl_display_dispatch_pending(display) > 0) {
                wl_display_dispatch(display);
            }
            dispatchCount++;
        }

        eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                       EGL_NO_CONTEXT);
        ::LWE::WebContainer* webContainer = ::LWE::WebContainer::CreateGL(
            width, height,
            [this](WebContainer* wc) {
                if (!eglMakeCurrent(mDisplay, mSurface, mSurface, mContext)) {
                    auto eglError = eglGetError();
                    STARFISH_LOG_INFO("Made current failed error -> %d\n",
                                      (int)eglError);
                }
            },
            [this](WebContainer* wc) {
                if (!eglSwapBuffers(mDisplay, mSurface)) {
                    fprintf(stderr, "Swapped buffers failed\n");
                    auto eglError = eglGetError();
                    STARFISH_LOG_INFO("Made current failed error -> %d\n",
                                      (int)eglError);
                }
            },
            devicePixelRatio, defaultFontName, locale, timezoneID);

        m_impl = webContainer;
    }
    virtual void Destroy() override
    {
        WebView::Destroy();
    }

    bool m_isMouseLbuttonDown;
    wl_display* mWlDisplay;
    EGLDisplay mDisplay;
    EGLSurface mSurface;
    EGLContext mContext;
    wl_egl_window* mEglWindow;

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
    return new WebViewWayland(win, x, y, width, height, devicePixelRatio,
                              defaultFontName, locale, timezoneID);
}
}

#endif
