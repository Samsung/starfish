/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include <SkMatrix.h>

#include "StarfishConfig.h"
#include "PlatformWindow.h"

#include "Starfish.h"
#include "core/animation/AnimationTask.h"
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
#include "platform/window/PlatformWindowFactory.h"

#ifdef STARFISH_ENABLE_TEST
Starfish::CanvasSurface* g_surfaceForScreehShot;
bool g_fireOnloadEvent = false;
bool g_forceRendering = false;
#endif

namespace Starfish {

extern int g_portWindowBackend;

// The if-def statements below are temporary soluation to avoid affecting other
// ports of LWE except flutter. In the future, It will be removed when LWE's all
// ports are changed to a single binary.
PlatformWindow* PlatformWindow::create(Starfish* starfish, uint32_t width,
                                       uint32_t height)
{
    switch (static_cast<PORT_WINDOW_BACKEND>(g_portWindowBackend)) {
#ifdef PORT_WINDOW_BACKEND_GB
    case PORT_WINDOW_BACKEND::GB:
        return PlatformWindowFactory::createGb(starfish, width, height);
#endif
#ifdef PORT_WINDOW_BACKEND_GL
    case PORT_WINDOW_BACKEND::GL:
        return PlatformWindowFactory::createGl(starfish, width, height);
#endif
#ifdef PORT_WINDOW_BACKEND_HEADLESS
    case PORT_WINDOW_BACKEND::HEADLESS:
        return PlatformWindowFactory::createHeadless(starfish, width, height);
#endif
    default:
        break;
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return nullptr;
}

PlatformWindow::PlatformWindow(Starfish* starfish)
    : m_starfish(starfish)
    , m_webView(nullptr)
    , m_renderingAnimator(TimerInvalidID)
    , m_compostiorContext(nullptr)
    , m_lastMouseMoveX(std::numeric_limits<float>::max())
    , m_lastMouseMoveY(std::numeric_limits<float>::max())
    , m_isDestroyed(false)
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
    if (webView()) {
        webView()->pause();
    }
}

void PlatformWindow::resume()
{
    if (webView()) {
        webView()->resume();
    }
}

void PlatformWindow::clearNativeHandlers()
{
    m_setNeedsRenderingCallback = nullptr;
    m_renderingFinishedCallback = nullptr;
    m_showSoftwareKeyboardIfPossibleCallback = nullptr;
    m_hideSoftwareKeyboardIfPossibleCallback = nullptr;
    m_glMakeCurrentCallback = nullptr;
    m_glSwapBufferCallback = nullptr;

    std::unordered_map<WindowHandlerKind, std::function<void(void*)>>().swap(
        m_handlersToCallbacks);
}

void PlatformWindow::destroy()
{
    STARFISH_LOG_INFO("PlatformWindow::destroy()");
    m_isDestroyed = true;
    clearResources();
}

void PlatformWindow::dispatchTouchEvent(TouchEventKind kind, TouchData* touches,
                                        size_t touchCount)
{
    for (size_t i = 0; i < touchCount; i++) {
        touches[i].setScreenX(touches[i].screenX() /
                              webView()->screenInfo().devicePixelRatio);
        touches[i].setScreenY(touches[i].screenY() /
                              webView()->screenInfo().devicePixelRatio);
        touches[i].setClientX(touches[i].clientX() /
                              webView()->screenInfo().devicePixelRatio);
        touches[i].setClientY(touches[i].clientY() /
                              webView()->screenInfo().devicePixelRatio);
    }
    webView()->dispatchTouchEvent(kind, touches, touchCount);
}

void PlatformWindow::dispatchMouseEvent(MouseEventKind kind, MouseData data)
{
    if (kind == MouseEventKind::MouseEventMove) {
        if (m_lastMouseMoveX == data.screenX() &&
            m_lastMouseMoveY == data.screenY()) {
            return;
        }
        m_lastMouseMoveX = data.screenX();
        m_lastMouseMoveY = data.screenY();
    } else {
        m_lastMouseMoveX = data.screenX();
        m_lastMouseMoveY = data.screenY();
    }
    data.setScreenX(data.screenX() / webView()->screenInfo().devicePixelRatio);
    data.setScreenY(data.screenY() / webView()->screenInfo().devicePixelRatio);
    data.setClientX(data.clientX() / webView()->screenInfo().devicePixelRatio);
    data.setClientY(data.clientY() / webView()->screenInfo().devicePixelRatio);
    webView()->dispatchMouseEvent(kind, data);
}

void PlatformWindow::dispatchMouseWheelEvent(float screenX, float screenY,
                                             int z, bool isVerticalWheelEvent)
{
    screenX /= webView()->screenInfo().devicePixelRatio;
    screenY /= webView()->screenInfo().devicePixelRatio;
    webView()->dispatchMouseWheelEvent(screenX, screenY, z,
                                       isVerticalWheelEvent);
}

void PlatformWindow::dispatchKeyEvent(KeyEventKind kind,
                                      PlatformKeyEventData data)
{
    STARFISH_LOG_INFO("PlatformWindow::dispatchKeyEvent %d",
                      (int)data.keyValue());

    LongTaskFinder p("PlatformWindow::dispatchKeyEvent", 1);

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

    webView()->dispatchKeyEvent(kind, data);
}

void PlatformWindow::dispatchCompositionEvent(CompositionEventKind kind,
                                              String* data,
                                              Nullable<Node*> node)
{
    webView()->dispatchCompositionEvent(kind, data, node);
}

void PlatformWindow::clearResources()
{
    if (m_renderingAnimator != TimerInvalidID) {
        webView()->timer()->removeGenericAnimator(m_renderingAnimator);
        m_renderingAnimator = TimerInvalidID;
    }
    webView()->clearStackingContext();

    Compositor::destroyCompositorContext(this, m_compostiorContext);
    m_compostiorContext = nullptr;
}

void PlatformWindow::setNeedsRendering()
{
    if (UNLIKELY(!canRendering())) {
        return;
    }

    if (m_setNeedsRenderingCallback) {
        m_setNeedsRenderingCallback(this);
        return;
    }

    PlatformWindow* wnd = this;

    if (wnd->m_renderingAnimator != TimerInvalidID) {
        return;
    }

    wnd->m_renderingAnimator = webView()->timer()->addAnimator(
        nullptr,
        [](void* data) {
            PlatformWindow* wnd = (PlatformWindow*)data;
            if (!wnd->starfish()) {
                wnd->m_renderingAnimator = TimerInvalidID;
                return false;
            }

            if (wnd->width() != 0 && wnd->height() != 0) {
                wnd->rendering();
            } else {
                STARFISH_LOG_WARN("PlatformWindow size error");
            }

            if (wnd->webView()->needsContinuousRendering()) {
                STARFISH_ASSERT(!wnd->m_setNeedsRenderingCallback);
                return true;
            }

            wnd->m_renderingAnimator = TimerInvalidID;
            return false;
        },
        wnd);
}

RenderResult PlatformWindow::rendering()
{
    if (UNLIKELY(!canRendering())) {
        return RenderResult();
    }

    auto renderResult = webView()->rendering();
    if (renderResult.didPaintingOrCompositing && m_renderingFinishedCallback) {
        m_renderingFinishedCallback(renderResult);
    }
    if (m_setNeedsRenderingCallback) {
        if (webView()->needsContinuousRendering()) {
            webView()->timer()->addAnimator(
                webView()->mainBrowsingContext()->window(),
                [](void* data) -> bool {
                    WebView* wv = (WebView*)data;
                    wv->setNeedsRendering();
                    return false;
                },
                webView());
        }
    }

    return renderResult;
}

void PlatformWindow::registerCallbackHandler(
    WindowHandlerKind handlerKind, const std::function<void(void*)>& handler)
{
    auto it = m_handlersToCallbacks.find(handlerKind);
    if (it == m_handlersToCallbacks.end()) {
        m_handlersToCallbacks.insert(std::make_pair(handlerKind, handler));
    } else {
        it->second = handler;
    }
}

void PlatformWindow::callHandler(WindowHandlerKind handlerKind, void* param)
{
    auto it = m_handlersToCallbacks.find(handlerKind);
    if (it == m_handlersToCallbacks.end()) {
        return;
    }

    struct Env : public gc {
        PlatformWindow* window;
        WindowHandlerKind handlerKind;
        void* param;
    };

    Env* env = new Env();
    env->window = this;
    env->handlerKind = handlerKind;
    env->param = param;

    webView()->messageLoop()->addIdler(
        nullptr,
        [](size_t, void* env) {
            Env* e = (Env*)env;
            auto it = e->window->m_handlersToCallbacks.find(e->handlerKind);
            if (it != e->window->m_handlersToCallbacks.end()) {
                (it->second)(e->param);
            }
        },
        env);
}

void PlatformWindow::registerCanRenderingCallback(
    const std::function<bool(PlatformWindow* wnd)>& cb)
{
    m_canRenderingCallback = cb;
    m_webView->m_isActive = cb(this);
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
        Canvas* c = Canvas::create(webView(), m_virtualCursorCanvasSurface);
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
    webView()->resize(width(), height());
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
} // namespace Starfish
