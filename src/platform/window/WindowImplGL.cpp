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

#ifdef PORT_WINDOW_BACKEND_GL

#include "StarFish.h"

#include "core/animation/Animation.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/message_loop/MessageLoop.h"

#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "platform/window/PlatformWindow.h"

#include <GLES2/gl2.h>

#ifdef PORT_WINDOW_BACKEND_GLFW
#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
#endif

namespace StarFish {

class WindowImplGL : public PlatformWindow {
public:
    WindowImplGL(StarFish* sf, int32_t width, int32_t height)
        : PlatformWindow(sf)
        , m_width(width)
        , m_height(height)
        , m_glPaintingSurface(nullptr)
        , m_didPaintingOrCompositing(true)
    {
        m_renderingAnimator = SIZE_MAX;
        m_offsetYDueToSoftwareKeyboard = 0;

        m_lastMouseX = -1;
        m_lastMouseY = -1;
        m_isMouseLbuttonDown = false;
        m_isKeyDown = false;
    }

    virtual int32_t width() override
    {
        return m_width;
    }

    virtual int32_t height() override
    {
        return m_height;
    }

    virtual void resizeTo(int w, int h)
    {
        if (w != (int)m_width || h != (int)m_height) {
            m_width = w;
            m_height = h;
            PlatformWindow::resizeTo(w, h);
        }
    }
    virtual void* unwrap()
    {
        return nullptr;
    }

    virtual void setNeedsRendering() override;

    virtual void clearResources();
    virtual Canvas* preparePainting();
    virtual Compositor* prepareCompositor();

    virtual void glMakeCurrent() = 0;

    uint32_t m_width;
    uint32_t m_height;
    size_t m_renderingAnimator;
    CanvasSurface* m_glPaintingSurface;
    bool m_didPaintingOrCompositing;

    float m_lastMouseX, m_lastMouseY;
    bool m_isMouseLbuttonDown;
    bool m_isKeyDown;
    int m_offsetYDueToSoftwareKeyboard;
};

static size_t g_totalCanvasSurfaceGLSize;
class CanvasSurfaceGL : public CanvasSurface {
public:
    CanvasSurfaceGL(PlatformWindow* wnd, size_t w, size_t h)
    {
        m_window = (WindowImplGL*)wnd;
        m_width = w;
        m_height = h;
        m_imageWidth = m_bufferWidth = m_width = -1;
        m_imageHeight = m_bufferHeight = m_height = -1;
        m_pixelRatio = 1;
        m_textureID = 0;
        m_buffer = nullptr;

        attachNativeBuffer(w, h);
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           CanvasSurfaceGL* s =
                                               (CanvasSurfaceGL*)obj;
                                           s->detachNativeBuffer();
                                       },
                                       NULL, NULL, NULL);
    }

    virtual void detachNativeBuffer()
    {
        if (m_buffer) {
            m_window->glMakeCurrent();
            if (m_textureID) {
                glDeleteTextures(1, &m_textureID);
            }
            g_totalCanvasSurfaceGLSize -=
                m_bufferWidth * m_bufferHeight * sizeof(uint32_t);
            free(m_buffer);
            m_buffer = nullptr;
            m_textureID = 0;
            STARFISH_LOG_INFO("total CanvasSurface size %fMB\n",
                              g_totalCanvasSurfaceGLSize / 1024.f / 1024.f);
        }
    }

    void attachNativeBuffer(size_t w, size_t h)
    {
        if (m_width != w || m_height != h) {
            detachNativeBuffer();
            m_width = w;
            m_height = h;

            float windowDevicePixelRatio =
                m_window->starFish()->screenInfo().devicePixelRatio;

            if ((int)w < m_window->starFish()->screenInfo().rect.width()) {
                w += STARFISH_CANVAS_SURFACE_MARGIN;
            }
            if ((int)h < m_window->starFish()->screenInfo().rect.height()) {
                h += STARFISH_CANVAS_SURFACE_MARGIN;
            }

            m_pixelRatio = 1;

            while ((m_width / m_pixelRatio * windowDevicePixelRatio > 20000) ||
                   (m_height / m_pixelRatio * windowDevicePixelRatio > 20000)) {
                m_pixelRatio++;
            }

            m_imageWidth =
                std::max((size_t)1, (size_t)(m_width / m_pixelRatio *
                                             windowDevicePixelRatio));
            m_imageHeight =
                std::max((size_t)1, (size_t)(m_height / m_pixelRatio *
                                             windowDevicePixelRatio));

            m_bufferWidth = std::max(
                (size_t)1, (size_t)(w / m_pixelRatio * windowDevicePixelRatio));
            m_bufferHeight = std::max(
                (size_t)1, (size_t)(h / m_pixelRatio * windowDevicePixelRatio));
            m_bufferStride = m_bufferWidth * 4;
            m_buffer = (unsigned char*)malloc(m_bufferWidth * m_bufferHeight *
                                              sizeof(uint32_t));

            g_totalCanvasSurfaceGLSize +=
                m_bufferWidth * m_bufferHeight * sizeof(uint32_t);
            STARFISH_LOG_INFO("total CanvasSurface size %fMB\n",
                              g_totalCanvasSurfaceGLSize / 1024.f / 1024.f);
        }
    }

    virtual void resize(size_t w, size_t h)
    {
        STARFISH_RELEASE_ASSERT(w <= m_bufferWidth * m_pixelRatio);
        STARFISH_RELEASE_ASSERT(h <= m_bufferHeight * m_pixelRatio);

        m_width = w;
        m_height = h;

        m_imageWidth = std::max((size_t)1, m_width / m_pixelRatio);
        m_imageHeight = std::max((size_t)1, m_height / m_pixelRatio);

        STARFISH_RELEASE_ASSERT(m_imageWidth <= m_bufferWidth);
        STARFISH_RELEASE_ASSERT(m_imageHeight <= m_bufferHeight);
    }

    void ensureGenerateTexture()
    {
        if (m_textureID) {
            return;
        }

        m_window->glMakeCurrent();

        glGenTextures(1, &m_textureID);
        STARFISH_ASSERT(glGetError() == 0);

        glBindTexture(GL_TEXTURE_2D, m_textureID);
        STARFISH_ASSERT(glGetError() == 0);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_bufferWidth, m_bufferHeight,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, m_buffer);
        STARFISH_ASSERT(glGetError() == 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glBindTexture(GL_TEXTURE_2D, 0);
        STARFISH_ASSERT(glGetError() == 0);
    }

    virtual void* unwrap()
    {
        ensureGenerateTexture();
        return (void*)((size_t)m_textureID);
    }

    virtual uint8_t* data()
    {
        return m_buffer;
    }

    virtual size_t width()
    {
        return m_width;
    }

    virtual size_t height()
    {
        return m_height;
    }

    virtual size_t bufferWidth()
    {
        return m_bufferWidth;
    }

    virtual size_t bufferHeight()
    {
        return m_bufferHeight;
    }

    virtual size_t imageWidth()
    {
        return m_imageWidth;
    }

    virtual size_t imageHeight()
    {
        return m_imageHeight;
    }

    virtual size_t pixelRatio()
    {
        return m_pixelRatio;
    }

    virtual size_t bufferStride()
    {
        return m_bufferStride;
    }

    virtual void clear()
    {
        size_t end = m_bufferWidth * m_bufferHeight * sizeof(uint32_t);
        memset(m_buffer, 0x00, end);
    }

    virtual void notifyUpdateRegion(size_t x, size_t y, size_t w, size_t h)
    {
        if (m_textureID == 0) {
            ensureGenerateTexture();
            return;
        }
        m_window->glMakeCurrent();

        glBindTexture(GL_TEXTURE_2D, m_textureID);
        STARFISH_ASSERT(glGetError() == 0);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        auto data = m_buffer;
        data += y * m_bufferWidth * 4;
        x = 0;
        w = m_bufferWidth;

        if (data == m_buffer && x == 0 && y == 0 && w == m_bufferWidth &&
            h == m_bufferHeight) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_bufferWidth,
                         m_bufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         m_buffer);
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA,
                            GL_UNSIGNED_BYTE, data);
        }
        GLuint error;
        STARFISH_ASSERT((error = glGetError()) == 0);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        glBindTexture(GL_TEXTURE_2D, 0);
        STARFISH_ASSERT(glGetError() == 0);
    }

protected:
    WindowImplGL* m_window;
    unsigned char* m_buffer;
    CanvasSurface* m_glPaintingSurface;
    GLuint m_textureID;
    size_t m_width;
    size_t m_height;
    size_t m_imageWidth;
    size_t m_imageHeight;
    size_t m_bufferWidth;
    size_t m_bufferHeight;
    size_t m_bufferStride;
    size_t m_pixelRatio;
};

CanvasSurface* CanvasSurface::create(PlatformWindow* wnd, size_t w, size_t h)
{
    return new CanvasSurfaceGL(wnd, w, h);
}

void WindowImplGL::setNeedsRendering()
{
    WindowImplGL* wnd = this;

    // refresh rendering animator
    if (wnd->m_renderingAnimator != SIZE_MAX) {
        starFish()->messageLoop()->removeIdler(m_renderingAnimator);
        wnd->m_renderingAnimator = SIZE_MAX;
    }

    wnd->m_renderingAnimator = starFish()->messageLoop()->addIdler(
        nullptr,
        [](size_t handle, void* data) {
            WindowImplGL* wnd = (WindowImplGL*)data;
            if (!wnd->starFish()) {
                return;
            }
            ((WindowImplGL*)wnd)->m_renderingAnimator = SIZE_MAX;
            if (wnd->width() != 0 && wnd->height() != 0) {
                StarFishEnterer enter(wnd->starFish());
                wnd->rendering();
            }
        },
        starFish()->platformWindow());
}

Canvas* WindowImplGL::preparePainting()
{
    if (!m_glPaintingSurface) {
        m_glPaintingSurface =
            CanvasSurface::create(this, (size_t)width(), (size_t)height());
    }
    m_glPaintingSurface->attachNativeBuffer((size_t)width(), (size_t)height());
    return Canvas::create(starFish(), m_glPaintingSurface);
}

Compositor* WindowImplGL::prepareCompositor()
{
    glMakeCurrent();
    if (m_glPaintingSurface) {
        m_glPaintingSurface->detachNativeBuffer();
        m_glPaintingSurface = nullptr;
    }
    return Compositor::create(starFish(), (void*)nullptr);
}

void WindowImplGL::clearResources()
{
    if (m_renderingAnimator != SIZE_MAX) {
        starFish()->messageLoop()->removeIdler(m_renderingAnimator);
        m_renderingAnimator = SIZE_MAX;
    }
    webView()->clearStackingContext();
}

#if defined(STARFISH_ENABLE_TEST)
void screenShotInRendering(StarFish* starfish, const char* path,
                           std::function<void()> callback)
{
    WindowImplGL* wnd = (WindowImplGL*)starfish->platformWindow();
    // TODO
}
#endif

#ifdef PORT_WINDOW_BACKEND_GLFW

static void error_callback(int error, const char* description)
{
    fputs(description, stderr);
    STARFISH_CRASH();
}

class WindowImplGLFW : public WindowImplGL {
public:
    WindowImplGLFW(StarFish* sf, int32_t width, int32_t height)
        : WindowImplGL(sf, width, height)
    {
        glfwSetErrorCallback(error_callback);
        if (!glfwInit())
            exit(-1);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        m_glWindow = glfwCreateWindow(width, height, "StarFish", NULL, NULL);
    }

    virtual void glMakeCurrent()
    {
        glfwMakeContextCurrent(m_glWindow);
    }

    virtual void onIdle() override
    {
        PlatformWindow::onIdle();
        glfwPollEvents();
    }

    virtual RenderResult rendering() override
    {
        // ProfilerTimer renderingTimer(starFish(), "WindowImplEFL::rendering");
        glMakeCurrent();
        RenderResult ret = WindowImplGL::rendering();
        if (ret.didPaintingOrCompositing) {
            if (webView()->didCompositeBefore()) {
            } else {
                m_glPaintingSurface->notifyUpdateRegion(
                    (int)ret.updateRect.x(), (int)ret.updateRect.y(),
                    (int)ret.updateRect.width(), (int)ret.updateRect.height());
                Compositor* c = Compositor::create(starFish(), (void*)nullptr);
                c->clearColor(Unit::Color(0, 0, 0, 0));
                c->drawSurface(m_glPaintingSurface,
                               Unit::Rect(0, 0, width(), height()));
                delete c;
            }
            glfwSwapBuffers(m_glWindow);
        }
        return ret;
    }

    GLFWwindow* m_glWindow;
};

PlatformWindow* PlatformWindow::create(StarFish* sf, void* win, int width,
                                       int height)
{
    return new WindowImplGLFW(sf, width, height);
}

#endif

} // namespace StarFish
#endif
