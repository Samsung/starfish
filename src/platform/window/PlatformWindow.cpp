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

#if defined(PORT_GRAPHIC_BACKEND_EFL) && defined(STARFISH_ENABLE_TEST)
#include <Elementary.h>
Evas_Object* g_imgBufferForScreehShot;
#endif

namespace StarFish {

PlatformWindow::PlatformWindow(StarFish* starFish)
    : m_starFish(starFish)
    , m_webView(nullptr)
    , m_animationExecutor(nullptr)
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

void PlatformWindow::dispatchTouchEvent(float x, float y, TouchEventKind kind,
                                        bool isMobile)
{
    webView()->mainBrowsingContext()->dispatchTouchEvent(x, y, kind, isMobile);
}

void PlatformWindow::dispatchMouseEvent(float x, float y, MouseEventKind kind)
{
    webView()->mainBrowsingContext()->dispatchMouseEvent(x, y, kind);
}

void PlatformWindow::dispatchKeyEvent(String* key, KeyEventKind kind)
{
    webView()->mainBrowsingContext()->dispatchKeyEvent(key, kind);
}

void PlatformWindow::rendering()
{
    webView()->mainBrowsingContext()->rendering();
}

void PlatformWindow::paintWindowBackground(Canvas* canvas)
{
    webView()->mainBrowsingContext()->paintWindowBackground(canvas);
}

void PlatformWindow::screenShot(std::string filePath)
{
    bool oldNeedsPainting = webView()->mainBrowsingContext()->m_needsPainting;
    bool oldOnLoad = g_fireOnloadEvent;
    g_fireOnloadEvent = true;
    g_forceRendering = true;
    webView()->mainBrowsingContext()->setNeedsPainting();
    setenv("SCREEN_SHOT", filePath.data(), 1);
    rendering();
    setenv("SCREEN_SHOT", "", 1);
    g_fireOnloadEvent = oldOnLoad;
    g_forceRendering = false;

    webView()->mainBrowsingContext()->m_needsPainting = oldNeedsPainting;
    webView()->mainBrowsingContext()->setNeedsRendering();
}
}
