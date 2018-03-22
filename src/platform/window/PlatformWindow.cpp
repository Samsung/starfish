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
#include "core/modules/message_loop/Timer.h"
#include "core/modules/profiling/Profiling.h"

#ifdef STARFISH_ENABLE_TEST
StarFish::CanvasSurface* g_surfaceForScreehShot;
bool g_fireOnloadEvent = false;
bool g_forceRendering = false;
#endif

namespace StarFish {

PlatformWindow::PlatformWindow(StarFish* starFish)
    : m_starFish(starFish)
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
    }
}

void PlatformWindow::close()
{
    STARFISH_LOG_INFO("PlatformWindow::close()\n");
    clearResources();
    if (m_idleCleanerTimerID != SIZE_MAX) {
        starFish()->timer()->removeTimer(m_idleCleanerTimerID);
    }
    webView()->close();
}

void PlatformWindow::dispatchTouchEvent(TouchEventKind kind, TouchData* touches,
                                        size_t touchCount)
{
    registerOrUpdateIdleTimeCleaner();
    if (webView()->mainBrowsingContext()) {
        webView()->mainBrowsingContext()->dispatchTouchEvent(kind, touches,
                                                             touchCount);
    }
}

void PlatformWindow::dispatchMouseEvent(MouseEventKind kind, MouseData data)
{
    registerOrUpdateIdleTimeCleaner();
    if (webView()->mainBrowsingContext()) {
        webView()->mainBrowsingContext()->dispatchMouseEvent(kind, data);
    }
}

void PlatformWindow::dispatchMouseWheelEvent(float screenX, float screenY,
                                             int z, bool isVerticalWheelEvent)
{
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

bool PlatformWindow::rendering()
{
    return webView()->rendering();
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
        webView()->mainBrowsingContext()->window()->resize(width(), height());
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
