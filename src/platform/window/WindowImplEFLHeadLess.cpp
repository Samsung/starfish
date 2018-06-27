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

#if defined(PORT_WINDOW_BACKEND_EFL_HEADLESS)
#include "StarFish.h"

#include "core/dom/Element.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/TouchEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "platform/window/PlatformWindow.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/profiling/Profiling.h"
#include "core/dom/CompositionEvent.h"

#include <Ecore.h>

namespace StarFish {

class WindowImplEFLHeadLess : public PlatformWindow {
public:
    WindowImplEFLHeadLess(StarFish* sf, int width, int height)
        : PlatformWindow(sf)
    {
        m_renderingAnimator = nullptr;
        m_isMouseLbuttonDown = false;
        m_isKeyDown = false;
        m_lastClickedTimestamp = 0;
        m_clickedCount = 0;
        m_inRendering = false;
        m_lastKeyPressedTimestamp = 0;
        m_width = width;
        m_height = height;

        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                STARFISH_LOG_INFO(
                    "WindowImplEFLHeadLess::~WindowImplEFLHeadLess\n");
            },
            NULL, NULL, NULL);
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
        m_width = w;
        m_height = h;
    }

    virtual void* unwrap()
    {
        return nullptr;
    }

    virtual void clearResources();
    virtual Canvas* preparePainting();
    virtual Compositor* prepareCompositor();

    virtual void showSoftwareKeyboardIfPossible()
    {
    }
    virtual void hideSoftwareKeyboardIfPossible()
    {
    }

    virtual bool isIMEEnabledNow()
    {
        return true;
    }

    virtual void onIdle()
    {
        PlatformWindow::onIdle();
    }

    virtual bool rendering() override
    {
        m_inRendering = true;
        bool ret = PlatformWindow::rendering();
        m_inRendering = false;
        return ret;
    }

    uintptr_t m_handle;

    float m_lastMouseX, m_lastMouseY;
    bool m_isMouseLbuttonDown;
    bool m_isKeyDown;
    bool m_canRendering;
    bool m_inRendering;
    bool m_isEvasFlushed;
    uint32_t m_lastClickedTimestamp;
    uint32_t m_clickedCount;
    uint32_t m_lastKeyPressedTimestamp;
    uint64_t m_lastRenderingTime;
    int m_width;
    int m_height;
    Ecore_Animator* m_renderingAnimator;
};

const uint32_t CLICK_REFRESH_DELAY = 400;

PlatformWindow* PlatformWindow::create(StarFish* sf, void* win, int width,
                                       int height)
{
    auto wnd = new WindowImplEFLHeadLess(sf, width, height);
    wnd->m_starFish = sf;
    return wnd;
}

void WebView::setNeedsRendering()
{
    m_needsRendering = true;
    WindowImplEFLHeadLess* wnd =
        (WindowImplEFLHeadLess*)starFish()->platformWindow();

    // refresh rendering animator
    if (wnd->m_renderingAnimator) {
        ecore_animator_freeze(wnd->m_renderingAnimator);
        ecore_animator_del(wnd->m_renderingAnimator);
    }

    wnd->m_renderingAnimator = ecore_animator_add(
        [](void* data) -> Eina_Bool {
            WindowImplEFLHeadLess* wnd = (WindowImplEFLHeadLess*)data;
            StarFishEnterer enter(wnd->starFish());
            wnd->m_renderingAnimator = nullptr;
            return ECORE_CALLBACK_CANCEL;
        },
        wnd);
}

Canvas* WindowImplEFLHeadLess::preparePainting()
{
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

Compositor* WindowImplEFLHeadLess::prepareCompositor()
{
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

void WindowImplEFLHeadLess::clearResources()
{
    if (m_renderingAnimator) {
        ecore_animator_freeze(m_renderingAnimator);
        ecore_animator_del(m_renderingAnimator);
    }
}
}
#endif
