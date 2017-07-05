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

#ifdef STARFISH_ENABLE_TEST
StarFish::CanvasSurface* g_surfaceForScreehShot;
bool g_fireOnloadEvent = false;
bool g_forceRendering = false;
#endif

#if defined(STARFISH_ENABLE_TEST)
#if defined(PORT_GRAPHIC_BACKEND_EFL)
#include <Elementary.h>
Evas_Object* g_imgBufferForScreehShot;
#elif defined(PORT_GRAPHIC_BACKEND_DALI)
unsigned char* g_imgBufferForScreehShot;
#endif
#endif

namespace StarFish {

PlatformWindow::PlatformWindow(StarFish* starFish)
    : m_starFish(starFish)
    , m_webView(nullptr)
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
    webView()->mainBrowsingContext()->close();
}

void PlatformWindow::dispatchTouchEvent(TouchEventKind kind, TouchData* touches,
                                        size_t touchCount)
{
    webView()->mainBrowsingContext()->dispatchTouchEvent(kind, touches,
                                                         touchCount);
}

void PlatformWindow::dispatchMouseEvent(MouseEventKind kind, MouseData& data)
{
    webView()->mainBrowsingContext()->dispatchMouseEvent(kind, data);
}

void PlatformWindow::dispatchKeyEvent(KeyEventKind kind, KeyboardData& data)
{
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
