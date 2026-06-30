/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#include "ShellConfig.h"

#if defined(STARFISH_SHELL_X11) || defined(STARFISH_SHELL_ECORE_X) || \
    defined(STARFISH_SHELL_ECORE_WL2) || defined(STARFISH_SHELL_TCORE_WL)

#include "Window.h"
#include "RendererDelegate.h"

#include <EGL/egl.h>
#include <cstring>
#include <cstdio>
#include <memory>

namespace StarfishShell {

class RendererDelegateOffscreen : public RendererDelegate {
public:
    RendererDelegateOffscreen() = default;

    ~RendererDelegateOffscreen()
    {
        if (m_eglDisplay != EGL_NO_DISPLAY) {
            eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                           EGL_NO_CONTEXT);
            if (m_eglSurface != EGL_NO_SURFACE)
                eglDestroySurface(m_eglDisplay, m_eglSurface);
            if (m_eglContext != EGL_NO_CONTEXT)
                eglDestroyContext(m_eglDisplay, m_eglContext);
            eglTerminate(m_eglDisplay);
        }
    }

    bool needsInitialize() const
    {
        return m_eglDisplay == EGL_NO_DISPLAY;
    }

    bool initialize()
    {
        m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (m_eglDisplay == EGL_NO_DISPLAY) {
            printf("RendererDelegateOffscreen: eglGetDisplay failed\n");
            return false;
        }

        EGLint major, minor;
        if (!eglInitialize(m_eglDisplay, &major, &minor)) {
            printf("RendererDelegateOffscreen: eglInitialize failed (0x%x)\n",
                   eglGetError());
            return false;
        }

        eglBindAPI(EGL_OPENGL_ES_API);

        static const EGLint configAttribs[] = { EGL_SURFACE_TYPE,
                                                EGL_PBUFFER_BIT,
                                                EGL_RENDERABLE_TYPE,
                                                EGL_OPENGL_ES2_BIT,
                                                EGL_RED_SIZE,
                                                8,
                                                EGL_GREEN_SIZE,
                                                8,
                                                EGL_BLUE_SIZE,
                                                8,
                                                EGL_ALPHA_SIZE,
                                                8,
                                                EGL_DEPTH_SIZE,
                                                24,
                                                EGL_NONE };

        EGLint numConfigs;
        if (!eglChooseConfig(m_eglDisplay, configAttribs, &m_eglConfig, 1,
                             &numConfigs) ||
            numConfigs == 0) {
            printf("RendererDelegateOffscreen: eglChooseConfig failed (0x%x)\n",
                   eglGetError());
            return false;
        }

        static const EGLint pbufferAttribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1,
                                                 EGL_NONE };
        m_eglSurface =
            eglCreatePbufferSurface(m_eglDisplay, m_eglConfig, pbufferAttribs);
        if (m_eglSurface == EGL_NO_SURFACE) {
            printf(
                "RendererDelegateOffscreen: eglCreatePbufferSurface failed "
                "(0x%x)\n",
                eglGetError());
            return false;
        }

        EGLint ctxAttribs3[] = { EGL_CONTEXT_MAJOR_VERSION, 3, EGL_NONE };
        m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig,
                                        EGL_NO_CONTEXT, ctxAttribs3);
        if (m_eglContext == EGL_NO_CONTEXT) {
            EGLint ctxAttribs2[] = { EGL_CONTEXT_MAJOR_VERSION, 2, EGL_NONE };
            m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig,
                                            EGL_NO_CONTEXT, ctxAttribs2);
        }
        if (m_eglContext == EGL_NO_CONTEXT) {
            printf(
                "RendererDelegateOffscreen: eglCreateContext failed (0x%x)\n",
                eglGetError());
            return false;
        }

        return true;
    }

    bool makeCurrent() override
    {
        return eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface,
                              m_eglContext);
    }

    bool swapBuffers() override
    {
        return eglSwapBuffers(m_eglDisplay, m_eglSurface);
    }

    uintptr_t createSharedContext() override
    {
        EGLint ctxAttribs3[] = { EGL_CONTEXT_MAJOR_VERSION, 3, EGL_NONE };
        EGLContext ctx = eglCreateContext(m_eglDisplay, m_eglConfig,
                                          m_eglContext, ctxAttribs3);
        if (ctx == EGL_NO_CONTEXT) {
            EGLint ctxAttribs2[] = { EGL_CONTEXT_MAJOR_VERSION, 2, EGL_NONE };
            ctx = eglCreateContext(m_eglDisplay, m_eglConfig, m_eglContext,
                                   ctxAttribs2);
        }
        return ctx != EGL_NO_CONTEXT ? reinterpret_cast<uintptr_t>(ctx)
                                     : UINTPTR_MAX;
    }

    bool destroyContext(uintptr_t context) override
    {
        return eglDestroyContext(m_eglDisplay,
                                 reinterpret_cast<EGLContext>(context));
    }

    bool clearCurrentContext() override
    {
        return eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                              EGL_NO_CONTEXT);
    }

    bool makeCurrentWithContext(uintptr_t context) override
    {
        return eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface,
                              reinterpret_cast<EGLContext>(context));
    }

    void* getProcAddress(const char* name) override
    {
        return reinterpret_cast<void*>(eglGetProcAddress(name));
    }

    bool isSupportedExtension(const char* extension) override
    {
        const char* extensions = eglQueryString(m_eglDisplay, EGL_EXTENSIONS);
        return extensions && strstr(extensions, extension) != nullptr;
    }

private:
    EGLDisplay m_eglDisplay = EGL_NO_DISPLAY;
    EGLSurface m_eglSurface = EGL_NO_SURFACE;
    EGLContext m_eglContext = EGL_NO_CONTEXT;
    EGLConfig m_eglConfig = nullptr;
};

class WindowDummy final : public Window {
public:
    WindowDummy() = default;
    ~WindowDummy() = default;

    bool init(const char* appName, int width, int height) override
    {
        m_appLoop->init();
        m_renderer.reset(new RendererDelegateOffscreen());
        return true;
    }

    void terminate() override
    {
    }

    void getCursorPos(double& xpos, double& ypos) override
    {
        xpos = 0;
        ypos = 0;
    }

    void* getNativeWindowHandle() override
    {
        return nullptr;
    }

    RendererDelegate* renderer() override
    {
        if (m_renderer && m_renderer->needsInitialize() &&
            !m_renderer->initialize()) {
            printf(
                "WindowDummy: offscreen EGL init failed, renderer will be "
                "unavailable\n");
            m_renderer.reset();
        }
        return m_renderer.get();
    }

private:
    std::unique_ptr<RendererDelegateOffscreen> m_renderer;
};

Window* Window::create()
{
    return new WindowDummy();
}

LWE::KeyValue Window::convertKeyCode(const unsigned long key, INPUT action,
                                     unsigned mods)
{
    return LWE::KeyValue::UnidentifiedKey;
}

} // namespace StarfishShell

#endif
