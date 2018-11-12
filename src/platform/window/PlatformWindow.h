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

#ifndef __StarfishPlatformWindow__
#define __StarfishPlatformWindow__

#include "core/page/RenderResult.h"
#include "core/event/EventModifierData.h"

namespace Starfish {
enum WindowHandlerKind {
    WindowHandlerShowDropdownMenu,
    WindowHandlerShowAlert,
    WindowHandlerOnDropdownMenuItemSelected,
};
}
namespace std {
template <>
struct hash<Starfish::WindowHandlerKind> {
    size_t operator()(Starfish::WindowHandlerKind const& x) const
    {
        return std::hash<uint32_t>()((uint32_t)x);
    }
};

template <>
struct equal_to<Starfish::WindowHandlerKind> {
    bool operator()(Starfish::WindowHandlerKind const& a,
                    Starfish::WindowHandlerKind const& b) const
    {
        return a == b;
    }
};
}

namespace Starfish {

class AnimationExecutor;

class Canvas;

class Compositor;

class CompositorContext;

class Node;

class WebView;

class NativeImageData;

class MouseData;

class TouchData;

class PlatformKeyEventData;

enum class TouchEventKind {
    TouchEventStart,
    TouchEventMove,
    TouchEventEnd,
    TouchEventCancel
};

enum class MouseEventKind {
    MouseEventDown,
    MouseEventMove,
    MouseEventUp,
    MouseEventEnter,
    MouseEventOut
};

enum class KeyEventKind { KeyEventDown, KeyEventPress, KeyEventUp };

enum class CompositionEventKind {
    CompositionEventStart,
    CompositionEventUpdate,
    CompositionEventEnd,
};
}

namespace Starfish {

class PlatformWindow : public gc {
public:
    virtual ~PlatformWindow(){};
    static PlatformWindow* create(Starfish* starfish, uint32_t width,
                                  uint32_t height);

    virtual uint32_t width() = 0;
    virtual uint32_t height() = 0;
    virtual void resizeTo(uint32_t w, uint32_t h)
    {
        STARFISH_LOG_INFO("PlatformWindow::resizeTo %d %d\n", w, h);
        onResize();
    }
    virtual void clearResources();
    void setNeedsRendering();
    virtual Canvas* preparePainting() = 0;
    virtual void willCompositing()
    {
    }
    virtual Compositor* prepareCompositor() = 0;
    virtual void showSoftwareKeyboardIfPossible()
    {
        if (m_showSoftwareKeyboardIfPossibleCallback) {
            m_showSoftwareKeyboardIfPossibleCallback();
        }
    }
    virtual void hideSoftwareKeyboardIfPossible()
    {
        if (m_hideSoftwareKeyboardIfPossibleCallback) {
            m_hideSoftwareKeyboardIfPossibleCallback();
        }
    }
    virtual bool isIMEEnabledNow()
    {
        return false;
    }
    virtual void* drawingBufferAddress()
    {
#if !defined(PORT_WINDOW_BACKEND_GB)
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
#endif
        return nullptr;
    }
    virtual void updateDrawingBufferAddress(void* buf, uint32_t width,
                                            uint32_t height, uint32_t stride)
    {
#if defined(PORT_WINDOW_BACKEND_GB)
        resizeTo(width, height);
#else
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
#endif
    }

    virtual void glMakeCurrent()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void glSwapBuffers()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void glEGLImageUpdated()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void glClearEGLImageUpdated()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    void registerRenderingFinishedCallback(
        const std::function<void(const RenderResult& renderResult)>& cb)
    {
        m_renderingFinishedCallback = cb;
    }

    void registerGLMakeCurrentCallback(
        const std::function<void(PlatformWindow* wnd)>& cb)
    {
        m_glMakeCurrentCallback = cb;
    }

    void registerGLSwapBuffersCallback(
        const std::function<void(PlatformWindow* wnd, bool mayNeedsSync)>& cb)
    {
        m_glSwapBufferCallback = cb;
    }

    void registerShowSoftwareKeyboardIfPossibleCallback(
        const std::function<void()>& cb)
    {
        m_showSoftwareKeyboardIfPossibleCallback = cb;
    }
    void registerHideSoftwareKeyboardIfPossibleCallback(
        const std::function<void()>& cb)
    {
        m_hideSoftwareKeyboardIfPossibleCallback = cb;
    }

    void registerSetNeedsRenderingCallback(
        const std::function<void(PlatformWindow* wnd)>& cb)
    {
        m_setNeedsRenderingCallback = cb;
    }

    void registerCallbackHandler(WindowHandlerKind handlerKind,
                                 const std::function<void(void*)>& handler);
    void callHandler(WindowHandlerKind handlerKind, void* param);

    virtual bool canRendering()
    {
        return false;
    }

    void dispatchTouchEvent(TouchEventKind kind, TouchData* touches,
                            size_t touchCount);
    void dispatchMouseEvent(MouseEventKind kind, MouseData data);
    void dispatchMouseWheelEvent(
        float screenX, float screenY, int z,
        bool isVerticalWheelEvent); // z : -1(up, left) or 1(down, right)
    void dispatchKeyEvent(KeyEventKind kind, PlatformKeyEventData data);
    void dispatchCompositionEvent(CompositionEventKind kind, String* data,
                                  Node* node = nullptr);

    virtual RenderResult rendering();
    virtual void pause();
    virtual void resume();
    virtual void destroy();

#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
    template <typename T>
    void paintVirtualCursor(T canvas);
#endif
    WebView* webView()
    {
        return m_webView;
    }

    Starfish* starfish()
    {
        return m_starfish;
    }

    void setWebView(WebView* webView);
    void screenShot(std::string filePath, void (*callback)(void*), void* data);
    virtual void onResize();
    virtual void onIdle()
    {
    }

    EventModifierData eventModifierData()
    {
        return m_eventModifierData;
    }

protected:
    PlatformWindow(Starfish* starfish);

    Starfish* m_starfish;
    WebView* m_webView;
    size_t m_renderingAnimator;
    CompositorContext* m_compostiorContext;
    size_t m_idleCleanerTimerID;
    EventModifierData m_eventModifierData;

    std::function<void(PlatformWindow* wnd)> m_setNeedsRenderingCallback;
    std::function<void(const RenderResult& renderResult)>
        m_renderingFinishedCallback;
    std::function<void()> m_showSoftwareKeyboardIfPossibleCallback;
    std::function<void()> m_hideSoftwareKeyboardIfPossibleCallback;

    std::function<void(PlatformWindow* wnd)> m_glMakeCurrentCallback;
    std::function<void(PlatformWindow* wnd, bool)> m_glSwapBufferCallback;

    std::unordered_map<WindowHandlerKind, std::function<void(void*)>>
        m_handlersToCallbacks;

#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
    bool m_isButtonOfVirtualCursorClicked;
    int m_virtualCursorX;
    int m_virtualCursorY;
    int m_virtualCursorSpeed;
    uint64_t m_virtualCursorMoveingLastTimestamp;
    CanvasSurface* m_virtualCursorCanvasSurface;
#endif
};
} // namespace Starfish

#endif
