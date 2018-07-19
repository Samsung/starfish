/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "PlatformWindow.h"

#include "StarFish.h"
#include "core/animation/Animation.h"
#include "core/dom/Node.h"
#include "core/layout/StackingContext.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/modules/canvas/Canvas.h"
#include "platform/window/VirtualCursor.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "platform/event/PlatformKeyEventData.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/TouchEvent.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/profiling/Profiling.h"

#ifdef STARFISH_ENABLE_TEST
StarFish::CanvasSurface* g_surfaceForScreehShot;
bool g_fireOnloadEvent = false;
bool g_forceRendering = false;
#endif

namespace StarFish {

#if !defined(PORT_COMPOSITOR_BACKEND_EFL)
static size_t g_totalCanvasSurfaceSimpleSize;
class CanvasSurfaceSimple : public CanvasSurface {
public:
    CanvasSurfaceSimple(PlatformWindow* wnd, size_t w, size_t h)
    {
        m_window = wnd;
        m_width = w;
        m_height = h;
        m_imageWidth = m_bufferWidth = m_width = -1;
        m_imageHeight = m_bufferHeight = m_height = -1;
        m_pixelRatio = 1;
        m_buffer = nullptr;

        attachNativeBuffer(w, h);
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           CanvasSurfaceSimple* s =
                                               (CanvasSurfaceSimple*)obj;
                                           s->detachNativeBuffer();
                                       },
                                       NULL, NULL, NULL);
    }

    virtual void detachNativeBuffer()
    {
        if (m_buffer) {
            g_totalCanvasSurfaceSimpleSize -=
                m_bufferWidth * m_bufferHeight * sizeof(uint32_t);
            free(m_buffer);
            m_buffer = nullptr;
            STARFISH_LOG_INFO("total CanvasSurface size %fMB\n",
                              g_totalCanvasSurfaceSimpleSize / 1024.f / 1024.f);
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
            g_totalCanvasSurfaceSimpleSize +=
                m_bufferWidth * m_bufferHeight * sizeof(uint32_t);
            STARFISH_LOG_INFO("total CanvasSurface size %fMB\n",
                              g_totalCanvasSurfaceSimpleSize / 1024.f / 1024.f);
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

    virtual void* unwrap()
    {
        return nullptr;
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

protected:
    PlatformWindow* m_window;
    unsigned char* m_buffer;
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
    return new CanvasSurfaceSimple(wnd, w, h);
}
#endif

PlatformWindow::PlatformWindow(StarFish* starFish)
    : m_isClosed(false)
    , m_starFish(starFish)
    , m_webView(nullptr)
    , m_idleCleanerTimerID(SIZE_MAX)
#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
    , m_isButtonOfVirtualCursorClicked(false)
    , m_virtualCursorX(-1)
    , m_virtualCursorY(-1)
    , m_virtualCursorSpeed(0)
    , m_virtualCursorMoveingLastTimestamp(0)
    , m_virtualCursorImageData(nullptr)
#endif
{
}

void PlatformWindow::setWebView(WebView* webView)
{
    m_webView = webView;
}

void PlatformWindow::pause()
{
    if (webView()->mainBrowsingContext()) {
        webView()->mainBrowsingContext()->pause();
    }
}

void PlatformWindow::resume()
{
    if (webView()->mainBrowsingContext()) {
        webView()->mainBrowsingContext()->resume();
        webView()->setNeedsPainting();
    }
}

void PlatformWindow::close()
{
    STARFISH_LOG_INFO("PlatformWindow::close()\n");
    m_isClosed = true;
    clearResources();
    if (m_idleCleanerTimerID != SIZE_MAX) {
        starFish()->timer()->removeTimer(m_idleCleanerTimerID);
    }
}

void PlatformWindow::dispatchTouchEvent(TouchEventKind kind, TouchData* touches,
                                        size_t touchCount)
{
    for (size_t i = 0; i < touchCount; i++) {
        touches[i].setScreenX(touches[i].screenX() /
                              starFish()->screenInfo().devicePixelRatio);
        touches[i].setScreenY(touches[i].screenY() /
                              starFish()->screenInfo().devicePixelRatio);
        touches[i].setClientX(touches[i].clientX() /
                              starFish()->screenInfo().devicePixelRatio);
        touches[i].setClientY(touches[i].clientY() /
                              starFish()->screenInfo().devicePixelRatio);
    }
    registerOrUpdateIdleTimeCleaner();
    if (webView()->mainBrowsingContext()) {
        webView()->mainBrowsingContext()->dispatchTouchEvent(kind, touches,
                                                             touchCount);
    }
}

void PlatformWindow::dispatchMouseEvent(MouseEventKind kind, MouseData data)
{
    data.setScreenX(data.screenX() / starFish()->screenInfo().devicePixelRatio);
    data.setScreenY(data.screenY() / starFish()->screenInfo().devicePixelRatio);
    data.setClientX(data.clientX() / starFish()->screenInfo().devicePixelRatio);
    data.setClientY(data.clientY() / starFish()->screenInfo().devicePixelRatio);
    registerOrUpdateIdleTimeCleaner();
    if (webView()->mainBrowsingContext()) {
        webView()->mainBrowsingContext()->dispatchMouseEvent(kind, data);
    }
}

void PlatformWindow::dispatchMouseWheelEvent(float screenX, float screenY,
                                             int z, bool isVerticalWheelEvent)
{
    screenX /= starFish()->screenInfo().devicePixelRatio;
    screenY /= starFish()->screenInfo().devicePixelRatio;
    registerOrUpdateIdleTimeCleaner();
    if (webView()->mainBrowsingContext()) {
        webView()->mainBrowsingContext()->dispatchMouseWheelEvent(
            screenX, screenY, z, isVerticalWheelEvent);
    }
}

void PlatformWindow::dispatchKeyEvent(KeyEventKind kind,
                                      PlatformKeyEventData data)
{
    STARFISH_LOG_INFO("PlatformWindow::dispatchKeyEvent %d\n",
                      (int)data.keyValue());
    registerOrUpdateIdleTimeCleaner();

#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
    if (!isIMEEnabledNow()) {
        const int virtualCursorInitialSpeed = 1;
        const int virtualCursorMaxSpeed = 30;
        bool isMouseMoved = false;
        MouseEventKind eventKind = MouseEventMove;
#define ADJEST_VIRTUAL_CURSOR_POSITION()                                    \
    isMouseMoved = true;                                                    \
    if (m_virtualCursorX < 0) {                                             \
        m_virtualCursorX = 0;                                               \
        if (!m_isButtonOfVirtualCursorClicked) {                            \
            dispatchMouseWheelEvent(m_virtualCursorX, m_virtualCursorY, -1, \
                                    false);                                 \
        }                                                                   \
    }                                                                       \
    if (m_virtualCursorX >= width()) {                                      \
        m_virtualCursorX = width() - 1;                                     \
        if (!m_isButtonOfVirtualCursorClicked) {                            \
            dispatchMouseWheelEvent(m_virtualCursorX, m_virtualCursorY, 1,  \
                                    false);                                 \
        }                                                                   \
    }                                                                       \
    if (m_virtualCursorY < 0) {                                             \
        m_virtualCursorY = 0;                                               \
        if (!m_isButtonOfVirtualCursorClicked) {                            \
            dispatchMouseWheelEvent(m_virtualCursorX, m_virtualCursorY, -1, \
                                    true);                                  \
        }                                                                   \
    }                                                                       \
    if (m_virtualCursorY >= height()) {                                     \
        m_virtualCursorY = height() - 1;                                    \
        if (!m_isButtonOfVirtualCursorClicked) {                            \
            dispatchMouseWheelEvent(m_virtualCursorX, m_virtualCursorY, 1,  \
                                    true);                                  \
        }                                                                   \
    }
#define DO_REDRAW_DISPATCH()                                         \
    if (webView()->didCompositeBefore()) {                           \
        webView()->setNeedsComposite();                              \
    } else {                                                         \
        webView()->setNeedsPainting();                               \
    }                                                                \
    dispatchMouseEvent(                                              \
        eventKind,                                                   \
        MouseData(MouseData::MouseButtonValue::LeftButton,           \
                  m_isButtonOfVirtualCursorClicked                   \
                      ? MouseData::MouseButtonsValue::LeftButtonDown \
                      : MouseData::MouseButtonsValue::NoButtonDown,  \
                  m_virtualCursorX, m_virtualCursorY,                \
                  m_isButtonOfVirtualCursorClicked));
        if (KeyEventDown == kind) {
            if (data.keyCode() >= 37 && data.keyCode() <= 40) {
                auto ts = timestamp();
                if ((ts - m_virtualCursorMoveingLastTimestamp) > 250) {
                    m_virtualCursorSpeed = virtualCursorInitialSpeed;
                } else {
                    m_virtualCursorSpeed += 3;
                    if (m_virtualCursorSpeed > virtualCursorMaxSpeed) {
                        m_virtualCursorSpeed = virtualCursorMaxSpeed;
                    }
                }

                m_virtualCursorMoveingLastTimestamp = ts;
            }

            int virtualCursorSpeed = m_virtualCursorSpeed;

            if (data.keyValue() == ArrowLeftKey) {
                // left
                m_virtualCursorX -= virtualCursorSpeed;
                ADJEST_VIRTUAL_CURSOR_POSITION()
                DO_REDRAW_DISPATCH()
            } else if (data.keyValue() == ArrowUpKey) {
                // up
                m_virtualCursorY -= virtualCursorSpeed;
                ADJEST_VIRTUAL_CURSOR_POSITION()
                DO_REDRAW_DISPATCH()
            } else if (data.keyValue() == ArrowRightKey) {
                // right
                m_virtualCursorX += virtualCursorSpeed;
                ADJEST_VIRTUAL_CURSOR_POSITION()
                DO_REDRAW_DISPATCH()
            } else if (data.keyValue() == ArrowDownKey) {
                // down
                m_virtualCursorY += virtualCursorSpeed;
                ADJEST_VIRTUAL_CURSOR_POSITION()
                DO_REDRAW_DISPATCH()
            } else if (data.keyValue() == SpaceKey ||
                       data.keyValue() == EnterKey) {
                // click
                eventKind = MouseEventDown;
                m_isButtonOfVirtualCursorClicked = true;
                isMouseMoved = true;
                DO_REDRAW_DISPATCH()
            }
        } else {
            if (data.keyValue() == SpaceKey || data.keyValue() == EnterKey) {
                // click
                eventKind = MouseEventUp;
                m_isButtonOfVirtualCursorClicked = false;
                isMouseMoved = true;
                DO_REDRAW_DISPATCH()
            }
        }
        if (isMouseMoved) {
            return;
        }
    } else {
        if (data.keyValue() == EscapeKey) {
            webView()->blur();
            return;
        }
    }

#undef DO_REDRAW_DISPATCH
#undef ADJEST_VIRTUAL_CURSOR_POSITION
#endif

    if (webView()->mainBrowsingContext()) {
        webView()->mainBrowsingContext()->dispatchKeyEvent(kind, data);
    }
}

void PlatformWindow::dispatchCompositionEvent(CompositionEventKind kind,
                                              String* data, Node* node)
{
    if (webView()->mainBrowsingContext()) {
        webView()->mainBrowsingContext()->dispatchCompositionEvent(kind, data,
                                                                   node);
    }
}

RenderResult PlatformWindow::rendering()
{
    auto renderResult = webView()->rendering();
    if (renderResult.didPaintingOrCompositing && m_renderingFinishedCallback) {
        m_renderingFinishedCallback(renderResult);
    }
    return renderResult;
}

void PlatformWindow::registerPlatformCallbackHandler(
    const std::string& handlerName, const std::function<void(void*)>& handler)
{
    auto it = m_platformHandlersToCallbacks.find(handlerName);
    if (it == m_platformHandlersToCallbacks.end()) {
        m_platformHandlersToCallbacks.insert(
            std::make_pair(handlerName, handler));

    } else {
        it->second = handler;
    }
}

void PlatformWindow::callPlatformHandler(const std::string& handlerName,
                                         void* param)
{
    auto it = m_platformHandlersToCallbacks.find(handlerName);
    if (it == m_platformHandlersToCallbacks.end()) {
        return;
    }

    struct Env {
        PlatformWindow* window;
        std::string handlerName;
        void* param;
    };

    Env* env = new Env();
    env->window = this;
    env->handlerName = handlerName;
    env->param = param;

    starFish()->messageLoop()->addIdler(
        nullptr,
        [](size_t, void* env) {
            Env* e = (Env*)env;
            auto it =
                e->window->m_platformHandlersToCallbacks.find(e->handlerName);
            if (it != e->window->m_platformHandlersToCallbacks.end()) {
                (it->second)(e->param);
            }
            delete e;
        },
        env);
}

#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
void PlatformWindow::paintVirtualCursor(Canvas* canvas)
{
    if (m_virtualCursorX == -1) {
        m_virtualCursorX = width() / 2;
    }
    if (m_virtualCursorY == -1) {
        m_virtualCursorY = height() / 2;
    }
    if (!m_virtualCursorImageData) {
        m_virtualCursorImageData = NativeImageData::create(
            (const char*)g_virtualCursorPNGData, g_virtualCursorPNGDataSize);
    }

    canvas->drawImage(m_virtualCursorImageData,
                      Unit::Rect(m_virtualCursorX, m_virtualCursorY, 25, 36));
}
#endif

void PlatformWindow::onResize()
{
#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
    if (m_virtualCursorX > width()) {
        m_virtualCursorX = width() - 10;
    }
    if (m_virtualCursorY > height()) {
        m_virtualCursorY = height() - 10;
    }
#endif
    if (webView()->mainBrowsingContext()) {
        webView()->mainBrowsingContext()->window()->resize(
            width() / starFish()->screenInfo().devicePixelRatio,
            height() / starFish()->screenInfo().devicePixelRatio);
        webView()->setNeedsPainting();
    }
}

#define IDLE_TIMER_TIMEOUT 1500
void PlatformWindow::registerOrUpdateIdleTimeCleaner()
{
    if (m_idleCleanerTimerID != SIZE_MAX) {
        starFish()->timer()->removeTimer(m_idleCleanerTimerID);
    }

    m_idleCleanerTimerID = starFish()->timer()->addTimer(
        IDLE_TIMER_TIMEOUT, nullptr,
        [](Window* wnd, void* data) {
            PlatformWindow* pwnd = (PlatformWindow*)data;

            pwnd->onIdle();
            pwnd->webView()->onIdle();
            // STARFISH_LOG_INFO("Do idle time GC\n");
            auto fn = GC_get_on_collection_event();
            GC_set_on_collection_event(nullptr);
            clearStack<102400>();
            GC_gcollect_and_unmap();
            GC_set_on_collection_event(fn);

            pwnd->registerOrUpdateIdleTimeCleaner();
        },
        this, false);
}

#ifdef STARFISH_ENABLE_TEST
void PlatformWindow::screenShot(std::string filePath)
{
    bool oldNeedsPainting = webView()->m_needsPainting;
    bool oldOnLoad = g_fireOnloadEvent;
    g_fireOnloadEvent = true;
    g_forceRendering = true;
    webView()->setNeedsPainting();
    setenv("SCREEN_SHOT", filePath.data(), 1);
    rendering();
    setenv("SCREEN_SHOT", "", 1);
    g_fireOnloadEvent = oldOnLoad;
    g_forceRendering = false;

    webView()->m_needsPainting = oldNeedsPainting;
    webView()->setNeedsRendering();
}
#endif
}
