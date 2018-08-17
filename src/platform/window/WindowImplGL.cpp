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
#include "core/modules/message_loop/Timer.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "platform/window/PlatformWindow.h"
#include "platform/event/PlatformKeyEventData.h"

#include <GLES2/gl2.h>

#ifdef PORT_WINDOW_BACKEND_GLFW
#define GLFW_INCLUDE_ES3
#include <GLFW/glfw3.h>
#endif

#if defined(STARFISH_ENABLE_TEST)
#include <cairo.h>
#endif

namespace StarFish {

#if defined(STARFISH_ENABLE_TEST)
std::function<void()> g_screenShotCallback;
std::string g_screenShotPath;
class WindowImplGL;
void screenShotImpl(WindowImplGL* wnd, const char* path,
                    std::function<void()> callback);
void screenShotInRendering(StarFish* starfish, const char* path,
                           std::function<void()> callback)
{
    g_screenShotCallback = callback;
    g_screenShotPath = path;
    return;
}
#endif

class WindowImplGL : public PlatformWindow {
public:
    WindowImplGL(StarFish* sf, int32_t width, int32_t height)
        : PlatformWindow(sf)
        , m_width(width)
        , m_height(height)
        , m_glPaintingSurface(nullptr)
        , m_compostiorContext(nullptr)
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

    virtual void resizeTo(int w, int h) override
    {
        if (w != (int)m_width || h != (int)m_height) {
            m_width = w;
            m_height = h;
            PlatformWindow::resizeTo(w, h);
        }
    }
    virtual void* unwrap() override
    {
        return nullptr;
    }

    virtual void setNeedsRendering() override;

    virtual RenderResult rendering() override
    {
        glMakeCurrent();

        if (!m_compostiorContext) {
            m_compostiorContext = Compositor::initCompositorContext(this);
        }

        RenderResult ret = PlatformWindow::rendering();
        if (ret.didPaintingOrCompositing) {
            if (webView()->didCompositeBefore()) {
            } else {
                m_glPaintingSurface->unMapBufferAndNotifyUpdateRegion(
                    (int)ret.updateRect.x(), (int)ret.updateRect.y(),
                    (int)ret.updateRect.width(), (int)ret.updateRect.height());
                float oldDPR = m_starFish->screenInfo().devicePixelRatio;
                m_starFish->screenInfo().devicePixelRatio = 1;
                Compositor* c = Compositor::create(
                    starFish(), m_compostiorContext, (void*)nullptr);
                c->clearColor(Unit::Color(0, 0, 0, 0));
                c->drawSurface(m_glPaintingSurface,
                               Unit::Rect(0, 0, width(), height()));
                delete c;
                m_starFish->screenInfo().devicePixelRatio = oldDPR;
            }
            glSwapBuffers();
        }

#if defined(STARFISH_ENABLE_TEST)
        if (g_screenShotCallback) {
            screenShotImpl(this, g_screenShotPath.data(), g_screenShotCallback);
            g_screenShotCallback = nullptr;
        }
#endif
        return ret;
    }

    virtual void clearResources() override;
    virtual Canvas* preparePainting() override;
    virtual void willCompositing() override
    {
        if (m_glPaintingSurface) {
            m_glPaintingSurface->detachNativeBuffer();
            m_glPaintingSurface = nullptr;
        }
    }

    virtual Compositor* prepareCompositor() override;

    virtual void glSwapBuffers() = 0;

    uint32_t m_width;
    uint32_t m_height;
    size_t m_renderingAnimator;
    CanvasSurface* m_glPaintingSurface;
    CompositorContext* m_compostiorContext;
    bool m_didPaintingOrCompositing;
    float m_lastMouseX, m_lastMouseY;
    bool m_isMouseLbuttonDown;
    bool m_isKeyDown;
    int m_offsetYDueToSoftwareKeyboard;
};

#if defined(STARFISH_ENABLE_TEST)
void screenShotImpl(WindowImplGL* wnd, const char* path,
                    std::function<void()> callback)
{
    glFinish();

    auto deviceWidth = wnd->width();
    auto deviceHeight = wnd->height();
    auto rowLength = deviceWidth * 4;

    auto dataLength = rowLength * deviceHeight;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    uint8_t* buffer = new uint8_t[dataLength];
    glReadPixels(0, 0, deviceWidth, deviceHeight, GL_RGBA, GL_UNSIGNED_BYTE,
                 buffer);

    // convert to rgba to bgra for cairo
    for (int y = 0; y < deviceHeight; y++) {
        for (int x = 0; x < deviceWidth; x++) {
            uint8_t* head = &buffer[rowLength * y + x * 4];
            std::swap(head[0], head[2]);
        }
    }

    // flip W
    /*
        for (int y = 0; y < deviceHeight; y++) {
            uint32_t* head = (uint32_t*)&buffer[rowLength * y];
            for (int x = 0; x < deviceWidth / 2; x++) {
                std::swap(head[x], head[deviceWidth - x - 1]);
            }
        }
    */
    // flip H
    for (int y = 0; y < deviceHeight / 2; y++) {
        uint32_t* head = (uint32_t*)&buffer[rowLength * y];
        uint32_t* head2 =
            (uint32_t*)&buffer[rowLength * (deviceHeight - y - 1)];
        for (int x = 0; x < deviceWidth; x++) {
            std::swap(head[x], head2[x]);
        }
    }

    cairo_surface_t* png_buffer;
    png_buffer = cairo_image_surface_create_for_data(
        (unsigned char*)buffer, CAIRO_FORMAT_ARGB32, deviceWidth, deviceHeight,
        rowLength);

    cairo_surface_write_to_png(png_buffer, path);
    cairo_surface_destroy(png_buffer);

    delete buffer;
    callback();
}
#endif

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
    float DPR = starFish()->screenInfo().devicePixelRatio;
    if (!m_glPaintingSurface) {
        m_glPaintingSurface =
            CanvasSurface::create(this, width() / DPR, height() / DPR);
    }
    m_glPaintingSurface->attachNativeBuffer(width() / DPR, height() / DPR);
    return Canvas::create(starFish(), m_glPaintingSurface);
}

Compositor* WindowImplGL::prepareCompositor()
{
    glMakeCurrent();
    if (m_glPaintingSurface) {
        m_glPaintingSurface->detachNativeBuffer();
        m_glPaintingSurface = nullptr;
    }
    return Compositor::create(starFish(), m_compostiorContext, (void*)nullptr);
}

void WindowImplGL::clearResources()
{
    if (m_renderingAnimator != SIZE_MAX) {
        starFish()->messageLoop()->removeIdler(m_renderingAnimator);
        m_renderingAnimator = SIZE_MAX;
    }
    webView()->clearStackingContext();

    Compositor::destroyCompositorContext(m_compostiorContext);
    m_compostiorContext = nullptr;
}

#ifdef PORT_WINDOW_BACKEND_GLFW

static void error_callback(int error, const char* description)
{
    STARFISH_LOG_ERROR("%s\n", description);
}

static void cursor_position_callback(GLFWwindow* window, double xpos,
                                     double ypos);
static void mouse_button_callback(GLFWwindow* window, int button, int action,
                                  int mods);
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
static void window_size_callback(GLFWwindow* window, int width, int height);
static void key_callback(GLFWwindow* window, int key, int scancode, int action,
                         int mods);

class WindowImplGLFW : public WindowImplGL {
public:
    WindowImplGLFW(StarFish* sf, int32_t width, int32_t height)
        : WindowImplGL(sf, width, height)
        , m_isMouseLbuttonDown(false)
    {
        glfwSetErrorCallback(error_callback);
        if (!glfwInit())
            exit(-1);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        m_glWindow = glfwCreateWindow(width, height, "StarFish", NULL, NULL);

        if (m_glWindow == nullptr) {
            STARFISH_LOG_ERROR(
                "failed to create OpenGL ES 3.0 context. try OpenGL 3.0 "
                "instead\n");
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
            m_glWindow =
                glfwCreateWindow(width, height, "StarFish", NULL, NULL);
            if (m_glWindow == nullptr) {
                STARFISH_LOG_ERROR(
                    "failed to create OpenGL 3.0 context. please check your "
                    "environment...\n");
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }

        glMakeCurrent();

        m_pollTimer = starFish()->timer()->addTimer(
            0.01, nullptr, [](Window* wnd, void* data) { glfwPollEvents(); },
            this, true);

        glfwSetWindowUserPointer(m_glWindow, this);
        glfwSetCursorPosCallback(m_glWindow, cursor_position_callback);
        glfwSetMouseButtonCallback(m_glWindow, mouse_button_callback);
        glfwSetScrollCallback(m_glWindow, scroll_callback);
        glfwSetWindowSizeCallback(m_glWindow, window_size_callback);
        glfwSetKeyCallback(m_glWindow, key_callback);

#if defined(STARFISH_ENABLE_TEST)
        // for screen shot
        glfwSwapInterval(0);
#endif
    }

    virtual void glMakeCurrent() override
    {
        glfwMakeContextCurrent(m_glWindow);
    }

    virtual void glSwapBuffers() override
    {
        glfwSwapBuffers(m_glWindow);
    }

    virtual void onIdle() override
    {
        PlatformWindow::onIdle();
        glfwPollEvents();
    }

    virtual RenderResult rendering() override
    {
        return WindowImplGL::rendering();
    }

    virtual void close() override
    {
        starFish()->timer()->removeTimer(m_pollTimer);
    }

    bool m_isMouseLbuttonDown;
    size_t m_pollTimer;
    GLFWwindow* m_glWindow;
};

static void cursor_position_callback(GLFWwindow* window, double xpos,
                                     double ypos)
{
    WindowImplGLFW* wnd = (WindowImplGLFW*)glfwGetWindowUserPointer(window);
    StarFishEnterer enter(wnd->starFish());
    unsigned char buttons =
        wnd->m_isMouseLbuttonDown ? MouseButtonsValue::LeftButtonDown : 0;
    MouseData mdata(0, buttons, xpos, ypos, 0);
    wnd->dispatchMouseEvent(MouseEventKind::MouseEventMove, mdata);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action,
                                  int mods)
{
    WindowImplGLFW* wnd = (WindowImplGLFW*)glfwGetWindowUserPointer(window);
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        StarFishEnterer enter(wnd->starFish());
        unsigned char buttons =
            wnd->m_isMouseLbuttonDown ? MouseButtonsValue::LeftButtonDown : 0;
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        MouseData mdata(0, buttons, xpos, ypos, 0);
        if (action == GLFW_PRESS) {
            wnd->m_isMouseLbuttonDown = true;
            wnd->dispatchMouseEvent(MouseEventKind::MouseEventDown, mdata);
        } else {
            wnd->m_isMouseLbuttonDown = false;
            wnd->dispatchMouseEvent(MouseEventKind::MouseEventUp, mdata);
        }
    }
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    WindowImplGLFW* wnd = (WindowImplGLFW*)glfwGetWindowUserPointer(window);
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    wnd->dispatchMouseWheelEvent(xpos, ypos, -yoffset, true);
}

static void window_size_callback(GLFWwindow* window, int width, int height)
{
    WindowImplGLFW* wnd = (WindowImplGLFW*)glfwGetWindowUserPointer(window);
    wnd->resizeTo(width, height);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action,
                         int mods)
{
    KeyValue keyValue = KeyValue::UnidentifiedKey;

    switch (key) {
    case GLFW_KEY_ESCAPE:
        keyValue = KeyValue::EscapeKey;
        break;
    case GLFW_KEY_ENTER:
        keyValue = KeyValue::EnterKey;
        break;
    case GLFW_KEY_SPACE:
        keyValue = KeyValue::SpaceKey;
        break;
    case GLFW_KEY_BACKSPACE:
        keyValue = KeyValue::BackspaceKey;
        break;
    case GLFW_KEY_LEFT:
        keyValue = KeyValue::ArrowLeftKey;
        break;
    case GLFW_KEY_RIGHT:
        keyValue = KeyValue::ArrowRightKey;
        break;
    case GLFW_KEY_DOWN:
        keyValue = KeyValue::ArrowDownKey;
        break;
    case GLFW_KEY_UP:
        keyValue = KeyValue::ArrowUpKey;
        break;
    default:
        keyValue = KeyValue::UnidentifiedKey;
        break;
    }

    PlatformKeyEventData pkdata(keyValue);
    WindowImplGLFW* wnd = (WindowImplGLFW*)glfwGetWindowUserPointer(window);
    StarFishEnterer enter(wnd->starFish());
    // TODO (repeat, modifiers)
    if (action == GLFW_PRESS) {
        wnd->dispatchKeyEvent(KeyEventKind::KeyEventDown, pkdata);
        wnd->dispatchKeyEvent(KeyEventKind::KeyEventPress, pkdata);
    } else {
        wnd->dispatchKeyEvent(KeyEventKind::KeyEventUp, pkdata);
    }
}

PlatformWindow* PlatformWindow::create(StarFish* sf, void* win, int width,
                                       int height)
{
    return new WindowImplGLFW(sf, width, height);
}

#endif

} // namespace StarFish
#endif
