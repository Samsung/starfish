/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#include "StarfishPlatform.h"

#if defined(PORT_WEBVIEW_BRIDGE_X11)
#include "platform/canvas/webgl/XGLUtil.h"
#include "public/bridge/x11/WindowEGL.h"
#define NO_ESCARGOT_GC_CONFLICT_GUARD
#include "platform/canvas/webgl/XGL.h"
#define NOT_EXPOSE_GC
#include "StarfishBase.h"

namespace LWE {

static bool createEGLDisplay(EGLDisplay& display, EGLConfig& config)
{
    // Connecting to the display
    EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglDisplay == EGL_NO_DISPLAY) {
        STARFISH_LOG_ERROR("Got no EGL display.");
        return false;
    }

    EGLint eglVersionMajor, eglVersionMinor;
    if (!eglInitialize(eglDisplay, &eglVersionMajor, &eglVersionMinor)) {
        STARFISH_LOG_ERROR("Unable to initialize EGL");
        return false;
    }

    // Set the current rendering API to OpenGL ES API
    eglBindAPI(EGL_OPENGL_ES_API);

    // Get frame buffer configuration supported
    EGLConfig eglConfig;
    {
        EGLint numConfig;
        EGLint configSize = 1;
        EGLint attributes[] = {
            EGL_SURFACE_TYPE,
            EGL_WINDOW_BIT,
            EGL_RED_SIZE,
            8,
            EGL_GREEN_SIZE,
            8,
            EGL_BLUE_SIZE,
            8,
            EGL_ALPHA_SIZE,
            0,
            EGL_DEPTH_SIZE,
            16,
            EGL_STENCIL_SIZE,
            8,
            EGL_SAMPLES,
            0,
            EGL_RENDERABLE_TYPE,
            EGL_OPENGL_ES2_BIT,
            EGL_NONE,
        };

        if (!eglChooseConfig(eglDisplay, attributes, &eglConfig, configSize,
                             &numConfig)) {
            STARFISH_LOG_ERROR("Failed to choose config (eglError: %d)",
                               eglGetError());
            return false;
        }
        if (numConfig != configSize) {
            STARFISH_LOG_ERROR("Didn't get exactly one config, but %d",
                               numConfig);
            return false;
        }
    }

    display = eglDisplay;
    config = eglConfig;

    return true;
}

static bool createEGLSurface(EGLSurface& surface, const EGLDisplay& eglDisplay,
                             const EGLConfig& eglConfig,
                             const unsigned long window)
{
    EGLSurface eglSurface;
    {
        EGLint attributes[] = { EGL_NONE };
        eglSurface =
            eglCreateWindowSurface(eglDisplay, eglConfig, window, attributes);
        if (eglSurface == EGL_NO_SURFACE) {
            STARFISH_LOG_ERROR("Unable to create EGL surface (eglError: 0x%x)",
                               eglGetError());
            return false;
        }
    }

    surface = eglSurface;

    return true;
}

bool initEGL(const NativeWindowType window, XGLPlatform* platform)
{
    EGLDisplay display{ nullptr };
    EGLSurface surface{ nullptr };
    EGLContext context{ nullptr };
    EGLConfig config{ nullptr };

    if (!createEGLDisplay(display, config) ||
        !createEGLSurface(surface, display, config, window) ||
        !XGLUtil::createXGLContext(context,
                                   { .type = XGLPlatform::Type::EGL,
                                     .context = nullptr,
                                     .egl{
                                         .display = display,
                                         .config = config,
                                         .surface = surface,
                                     } },
                                   nullptr)) {
        return false;
    }

    if (platform) {
        platform->type = XGLPlatform::Type::EGL;
        platform->egl.display = display;
        platform->egl.surface = surface;
        platform->egl.config = config;
        platform->context = context;
    }

    return true;
}

} // namespace LWE

#endif
