/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

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
#include "core/modules/canvas/image/ImageData.h"
#include "core/dom/KeyboardEvent.h"
#include "core/dom/MouseEvent.h"

#ifdef STARFISH_ENABLE_TEST
StarFish::CanvasSurface* g_surfaceForScreehShot;
bool g_fireOnloadEvent = false;
bool g_forceRendering = false;
#endif

#if defined(STARFISH_ENABLE_TEST)
#if defined(PORT_GRAPHIC_BACKEND_EFL)
#include <Elementary.h>
Evas_Object* g_imgBufferForScreehShot;
#elif defined(PORT_GRAPHIC_BACKEND_GENERAL_BUFFER)
unsigned char* g_imgBufferForScreehShot;
#endif
#endif

namespace StarFish {

PlatformWindow::PlatformWindow(StarFish* starFish)
    : m_starFish(starFish)
    , m_webView(nullptr)
#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
    , m_isButtonOfVirtualCursorClicked(false)
    , m_virtualCursorX(0)
    , m_virtualCursorY(0)
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
    webView()->mainBrowsingContext()->pause();
}

void PlatformWindow::resume()
{
    webView()->mainBrowsingContext()->resume();
}

void PlatformWindow::close()
{
    webView()->close();
}

void PlatformWindow::dispatchTouchEvent(TouchEventKind kind, TouchData* touches,
                                        size_t touchCount)
{
    webView()->mainBrowsingContext()->dispatchTouchEvent(kind, touches,
                                                         touchCount);
}

void PlatformWindow::dispatchMouseEvent(MouseEventKind kind, MouseData data)
{
    webView()->mainBrowsingContext()->dispatchMouseEvent(kind, data);
}

void PlatformWindow::dispatchKeyEvent(KeyEventKind kind, KeyboardData data)
{
#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
    const int virtualCursorSpeed = 10;
    MouseEventKind eventKind = MouseEventMove;
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
                  m_virtualCursorX, m_virtualCursorY));
    if (KeyEventDown == kind) {
        if (data.keyCode() == 37) {
            // left
            m_virtualCursorX -= virtualCursorSpeed;
            DO_REDRAW_DISPATCH()
        } else if (data.keyCode() == 38) {
            // up
            m_virtualCursorY -= virtualCursorSpeed;
            DO_REDRAW_DISPATCH()
        } else if (data.keyCode() == 39) {
            // right
            m_virtualCursorX += virtualCursorSpeed;
            DO_REDRAW_DISPATCH()
        } else if (data.keyCode() == 40) {
            // down
            m_virtualCursorY += virtualCursorSpeed;
            DO_REDRAW_DISPATCH()
        } else if (data.keyCode() == 32 || data.keyCode() == 13 ||
                   data.keyCode() == 8) {
            // click
            eventKind = MouseEventDown;
            m_isButtonOfVirtualCursorClicked = true;
            DO_REDRAW_DISPATCH()
        } else if (data.keyCode() == 48) {
            if (m_isButtonOfVirtualCursorClicked) {
                eventKind = MouseEventUp;
            } else {
                eventKind = MouseEventDown;
            }
            DO_REDRAW_DISPATCH()
            m_isButtonOfVirtualCursorClicked =
                !m_isButtonOfVirtualCursorClicked;
        }
    } else {
        if (data.keyCode() == 32 || data.keyCode() == 13 ||
            data.keyCode() == 8) {
            // click
            eventKind = MouseEventUp;
            m_isButtonOfVirtualCursorClicked = false;
            DO_REDRAW_DISPATCH()
        }
    }
#undef DO_REDRAW_DISPATCH
#endif
    webView()->mainBrowsingContext()->dispatchKeyEvent(kind, data);
}

void PlatformWindow::rendering()
{
    webView()->rendering();
}

void PlatformWindow::paintWindowBackground(Canvas* canvas)
{
    webView()->mainBrowsingContext()->paintWindowBackground(canvas);
}

#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
void PlatformWindow::paintVirtualCursor(Canvas* canvas)
{
    if (!m_virtualCursorImageData) {
        m_virtualCursorImageData = ImageData::create(
            (const char*)g_virtualCursorPNGData, g_virtualCursorPNGDataSize);
    }

    canvas->drawImage(m_virtualCursorImageData,
                      Unit::Rect(m_virtualCursorX, m_virtualCursorY, 25, 36));
}
#endif

void PlatformWindow::onResize()
{
#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
    m_virtualCursorX = width() / 2;
    m_virtualCursorY = height() / 2;
    webView()->mainBrowsingContext()->window()->resize(width(), height());
#endif
}

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
}
