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
#include "core/modules/canvas/Compositor.h"
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

PlatformWindow::PlatformWindow(StarFish* starFish)
    : m_isClosed(false)
    , m_starFish(starFish)
    , m_webView(nullptr)
    , m_renderingAnimator(SIZE_MAX)
    , m_compostiorContext(nullptr)
    , m_idleCleanerTimerID(SIZE_MAX)
#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
    , m_isButtonOfVirtualCursorClicked(false)
    , m_virtualCursorX(-1)
    , m_virtualCursorY(-1)
    , m_virtualCursorSpeed(0)
    , m_virtualCursorMoveingLastTimestamp(0)
    , m_virtualCursorCanvasSurface(nullptr)
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

    if (kind == KeyEventKind::KeyEventDown ||
        kind == KeyEventKind::KeyEventUp) {
        bool active = kind == KeyEventKind::KeyEventDown;
        if (data.keyValue() == KeyValue::ShiftLeftKey ||
            data.keyValue() == KeyValue::ShiftRightKey) {
            m_eventModifierData.setShiftKey(active);
        } else if (data.keyValue() == KeyValue::AltLeftKey ||
                   data.keyValue() == KeyValue::AltRightKey) {
            m_eventModifierData.setAltKey(active);
        } else if (data.keyValue() == KeyValue::ControlLeftKey ||
                   data.keyValue() == KeyValue::ControlRightKey) {
            m_eventModifierData.setCtrlKey(active);
        } else if (data.keyValue() == KeyValue::MetaKey) {
            m_eventModifierData.setMetaKey(active);
        }
    }

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

void PlatformWindow::clearResources()
{
    if (m_renderingAnimator != SIZE_MAX) {
        starFish()->messageLoop()->removeIdler(m_renderingAnimator);
        m_renderingAnimator = SIZE_MAX;
    }
    webView()->clearStackingContext();

    Compositor::destroyCompositorContext(m_compostiorContext);
    m_compostiorContext = nullptr;
}

void PlatformWindow::setNeedsRendering()
{
    PlatformWindow* wnd = this;

    if (m_setNeedsRenderingCallback) {
        m_setNeedsRenderingCallback(this);
        return;
    }

    // refresh rendering animator if needs
    if (wnd->m_renderingAnimator != SIZE_MAX) {
        if (webView()->hasActiveAnimationExecutor()) {
            return;
        }
        starFish()->messageLoop()->removeIdler(m_renderingAnimator);
        wnd->m_renderingAnimator = SIZE_MAX;
    }

    wnd->m_renderingAnimator = starFish()->messageLoop()->addIdler(
        nullptr,
        [](size_t handle, void* data) {
            PlatformWindow* wnd = (PlatformWindow*)data;
            if (!wnd->starFish()) {
                wnd->m_renderingAnimator = SIZE_MAX;
                return;
            }
            wnd->m_renderingAnimator = SIZE_MAX;
            if (wnd->width() != 0 && wnd->height() != 0) {
                wnd->rendering();
            }
        },
        wnd);
}

RenderResult PlatformWindow::rendering()
{
    auto renderResult = webView()->rendering();
    if (renderResult.didPaintingOrCompositing && m_renderingFinishedCallback) {
        m_renderingFinishedCallback(renderResult);
    }
    return renderResult;
}

void PlatformWindow::registerCallbackHandler(
    const std::string& handlerName, const std::function<void(void*)>& handler)
{
    auto it = m_handlersToCallbacks.find(handlerName);
    if (it == m_handlersToCallbacks.end()) {
        m_handlersToCallbacks.insert(std::make_pair(handlerName, handler));
    } else {
        it->second = handler;
    }
}

void PlatformWindow::callHandler(const std::string& handlerName, void* param)
{
    auto it = m_handlersToCallbacks.find(handlerName);
    if (it == m_handlersToCallbacks.end()) {
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
            auto it = e->window->m_handlersToCallbacks.find(e->handlerName);
            if (it != e->window->m_handlersToCallbacks.end()) {
                (it->second)(e->param);
            }
            delete e;
        },
        env);
}

#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
template <typename T>
void PlatformWindow::paintVirtualCursor(T canvas)
{
    if (m_virtualCursorX == -1) {
        m_virtualCursorX = width() / 2;
    }
    if (m_virtualCursorY == -1) {
        m_virtualCursorY = height() / 2;
    }
    if (!m_virtualCursorCanvasSurface) {
        m_virtualCursorCanvasSurface = CanvasSurface::create(this, 25, 36);
        Canvas* c = Canvas::create(starFish(), m_virtualCursorCanvasSurface);
        c->drawImage(
            NativeImageData::create((const char*)g_virtualCursorPNGData,
                                    g_virtualCursorPNGDataSize),
            Unit::Rect(0, 0, 25, 36));
        delete c;
    }

    canvas->drawImage(m_virtualCursorCanvasSurface,
                      Unit::Rect(m_virtualCursorX, m_virtualCursorY, 25, 36));
}
template void PlatformWindow::paintVirtualCursor<Canvas*>(Canvas*);
template void PlatformWindow::paintVirtualCursor<Compositor*>(Compositor*);
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
void PlatformWindow::screenShot(std::string filePath, void (*callback)(void*),
                                void* data)
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

    callback(data);
}
#endif
}
