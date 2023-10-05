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

#if defined(PORT_WEBVIEW_BRIDGE_X11) || defined(STARFISH_ENABLE_WEBGL)

#include "platform/canvas/webgl/EGLUtil.h"
#include "platform/canvas/webgl/EGL.h"
#include "StarfishBase.h"

namespace EGLUtil {

bool createEGLContext(EGLContext& context, const EGLDisplay eglDisplay,
                      const EGLConfig eglConfig, const EGLContext shareContext)
{
    EGLint attributes[] = { EGL_CONTEXT_MAJOR_VERSION, 3, EGL_NONE };
    EGLContext eglContext =
        eglCreateContext(eglDisplay, eglConfig, shareContext, attributes);

    if (eglContext == EGL_NO_CONTEXT) {
        EGLint attributes[] = { EGL_CONTEXT_MAJOR_VERSION, 2, EGL_NONE };
        eglContext =
            eglCreateContext(eglDisplay, eglConfig, shareContext, attributes);

        if (eglContext == EGL_NO_CONTEXT) {
            STARFISH_LOG_ERROR("Unable to create EGL context (eglError: 0x%x)",
                               eglGetError());
            return false;
        }
    }

    context = eglContext;
    return true;
}

bool destroyEGLContext(const EGLDisplay display, const EGLContext context)
{
    if (!eglDestroyContext(display, context)) {
        STARFISH_LOG_ERROR("Unable to destory EGL context (eglError: 0x%x)",
                           eglGetError());
        return false;
    }
    return true;
};

bool makeCurrentEGLContext(const EGLDisplay display,
                           const EGLSurface drawSurface,
                           const EGLSurface readSurface,
                           const EGLContext context)
{
    if (!eglMakeCurrent(display, drawSurface, readSurface, context)) {
        STARFISH_LOG_ERROR("Failed to set current context (eglError: 0x%x)",
                           eglGetError());
        return false;
    }
    return true;
};

void resetCurrentEGLContext(const EGLDisplay display)
{
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

bool makeCurrentEGLContext(const EGLPlatform& platform)
{
    if (!eglMakeCurrent(platform.display, platform.surface, platform.surface,
                        platform.context)) {
        STARFISH_LOG_ERROR("Failed to set current context (eglError: 0x%x)",
                           eglGetError());
        return false;
    }
    return true;
}

EGLContext getCurrentContext()
{
    return eglGetCurrentContext();
}

bool resetCurrentEGLContext(const EGLPlatform& platform)
{
    eglMakeCurrent(platform.display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                   EGL_NO_CONTEXT);
    return true;
}

bool swapGLBuffer(const EGLPlatform& platform)
{
    eglSwapBuffers(platform.display, platform.surface);
    return true;
}

void printEGLInfo(const EGLDisplay eglDisplay, const EGLConfig eglConfig,
                  const EGLContext eglContext)
{
    STARFISH_LOG_INFO("EGL_VERSION = %s",
                      eglQueryString(eglDisplay, EGL_VERSION));

    STARFISH_LOG_INFO("EGL_CLIENT_APIS = %s",
                      eglQueryString(eglDisplay, EGL_CLIENT_APIS));

    EGLint v = 0;

    if (eglQueryContext(eglDisplay, eglContext, EGL_CONTEXT_CLIENT_TYPE, &v)) {
        STARFISH_LOG_INFO("EGL_CONTEXT_CLIENT_TYPE = 0x%04x", v);
        if (v == EGL_OPENGL_ES_API) {
            STARFISH_LOG_INFO("  - EGL_OPENGL_ES_API");
        } else if (v == EGL_OPENGL_API) {
            STARFISH_LOG_INFO("  - EGL_OPENGL_API");
        } else {
            STARFISH_LOG_INFO("  - EGL_UNEXPECTED_API");
        }
    }

    if (eglQueryContext(eglDisplay, eglContext, EGL_CONTEXT_CLIENT_VERSION,
                        &v)) {
        STARFISH_LOG_INFO("EGL_CONTEXT_CLIENT_VERSION = %d", v);
    }

    if (eglQueryContext(eglDisplay, eglContext, EGL_RENDER_BUFFER, &v)) {
        switch (v) {
        case EGL_SINGLE_BUFFER:
            STARFISH_LOG_INFO("EGL_RENDER_BUFFER = EGL_SINGLE_BUFFER");
            break;
        case EGL_BACK_BUFFER:
            STARFISH_LOG_INFO("EGL_RENDER_BUFFER = EGL_BACK_BUFFER");
            break;
        case EGL_NONE:
            STARFISH_LOG_INFO("EGL_RENDER_BUFFER = EGL_NONE");
            break;
        default:
            STARFISH_LOG_INFO("EGL_RENDER_BUFFER = unknown value %d", v);
            break;
        }
    }

    // EGL frame buffer configurations

    if (eglGetConfigAttrib(eglDisplay, eglConfig, EGL_CONFIG_ID, &v)) {
        STARFISH_LOG_INFO("EGL_CONFIG_ID = %d", v);
    }

    if (eglGetConfigAttrib(eglDisplay, eglConfig, EGL_NATIVE_RENDERABLE, &v)) {
        STARFISH_LOG_INFO("EGL_NATIVE_RENDERABLE = %s",
                          v ? "EGL_TRUE" : "EGL_FALSE");
    }

    if (eglGetConfigAttrib(eglDisplay, eglConfig, EGL_RENDERABLE_TYPE, &v)) {
        STARFISH_LOG_INFO("EGL_RENDERABLE_TYPE = 0x%04x", v);
        if (v & EGL_OPENGL_ES_BIT) {
            STARFISH_LOG_INFO("  - EGL_OPENGL_ES_BIT");
        }
        if (v & EGL_OPENGL_ES2_BIT) {
            STARFISH_LOG_INFO("  - EGL_OPENGL_ES2_BIT");
        }
        if (v & EGL_OPENGL_ES3_BIT) {
            STARFISH_LOG_INFO("  - EGL_OPENGL_ES3_BIT");
        }
        if (v & EGL_OPENGL_BIT) {
            STARFISH_LOG_INFO("  - EGL_OPENGL_BIT");
        }
    }

    if (eglGetConfigAttrib(eglDisplay, eglConfig, EGL_SURFACE_TYPE, &v)) {
        STARFISH_LOG_INFO("EGL_SURFACE_TYPE: 0x%04x", v);
        if (v & EGL_WINDOW_BIT) {
            STARFISH_LOG_INFO("  - EGL_WINDOW_BIT");
        }
        if (v & EGL_PIXMAP_BIT) {
            STARFISH_LOG_INFO("  - EGL_PIXMAP_BIT");
        }
        if (v & EGL_PBUFFER_BIT) {
            STARFISH_LOG_INFO("  - EGL_PBUFFER_BIT");
        }
        if (v & EGL_OPENGL_ES3_BIT) {
            STARFISH_LOG_INFO("  - EGL_OPENGL_ES3_BIT");
        }
        if (v & EGL_OPENGL_ES2_BIT) {
            STARFISH_LOG_INFO("  - EGL_OPENGL_ES2_BIT");
        }
        if (v & EGL_OPENGL_ES_BIT) {
            STARFISH_LOG_INFO("  - EGL_OPENGL_ES_BIT");
        }
        if (v & EGL_GL_TEXTURE_2D) {
            STARFISH_LOG_INFO("  - EGL_GL_TEXTURE_2D");
        }
        if (v & EGL_GL_TEXTURE_3D) {
            STARFISH_LOG_INFO("  - EGL_GL_TEXTURE_3D");
        }
        if (v & EGL_GL_RENDERBUFFER) {
            STARFISH_LOG_INFO("  - EGL_GL_RENDERBUFFER");
        }
        if (v & EGL_IMAGE_PRESERVED) {
            STARFISH_LOG_INFO("  - EGL_IMAGE_PRESERVED");
        }
    }

    if (eglGetConfigAttrib(eglDisplay, eglConfig, EGL_RED_SIZE, &v)) {
        STARFISH_LOG_INFO("EGL_RED_SIZE = %d", v);
    }
    if (eglGetConfigAttrib(eglDisplay, eglConfig, EGL_GREEN_SIZE, &v)) {
        STARFISH_LOG_INFO("EGL_GREEN_SIZE = %d", v);
    }
    if (eglGetConfigAttrib(eglDisplay, eglConfig, EGL_BLUE_SIZE, &v)) {
        STARFISH_LOG_INFO("EGL_BLUE_SIZE = %d", v);
    }
    if (eglGetConfigAttrib(eglDisplay, eglConfig, EGL_ALPHA_SIZE, &v)) {
        STARFISH_LOG_INFO("EGL_ALPHA_SIZE = %d", v);
    }
    if (eglGetConfigAttrib(eglDisplay, eglConfig, EGL_DEPTH_SIZE, &v)) {
        STARFISH_LOG_INFO("EGL_DEPTH_SIZE = %d", v);
    }
    if (eglGetConfigAttrib(eglDisplay, eglConfig, EGL_STENCIL_SIZE, &v)) {
        STARFISH_LOG_INFO("EGL_STENCIL_SIZE = %d", v);
    }
    if (eglGetConfigAttrib(eglDisplay, eglConfig, EGL_SAMPLES, &v)) {
        STARFISH_LOG_INFO("EGL_SAMPLES = %d", v);
    }
    if (eglGetConfigAttrib(eglDisplay, eglConfig, EGL_LEVEL, &v)) {
        STARFISH_LOG_INFO("EGL_LEVEL = %d", v);
    }

    STARFISH_LOG_INFO("EGL_EXTENSIONS = %s",
                      eglQueryString(eglDisplay, EGL_EXTENSIONS));
}

} // namespace EGLUtil
#endif
