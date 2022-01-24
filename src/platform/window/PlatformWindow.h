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
} // namespace std

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
} // namespace Starfish

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
        STARFISH_LOG_INFO("PlatformWindow::resizeTo %d %d", w, h);
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
#if !defined(PORT_WINDOW_BACKEND_GB) && !defined(STARFISH_FLUTTER)
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
#endif
        return nullptr;
    }
    virtual void updateDrawingBufferAddress(void* buf, uint32_t stride)
    {
#if !defined(PORT_WINDOW_BACKEND_GB) && !defined(STARFISH_FLUTTER)
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
#endif
    }

    virtual bool glMakeCurrent()
    {
        // STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return true;
    }
    virtual void glSwapBuffers()
    {
        // STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    virtual void glMayNeedsSync()
    {
        // STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    void registerRenderingPrepareCallback(
        const std::function<RenderInfo(void)>& cb)
    {
        m_renderingPrepareCallback = cb;
    }

    void registerRenderingFinishedCallback(
        const std::function<void(const RenderResult& renderResult)>& cb)
    {
        m_renderingFinishedCallback = cb;
    }

    void registerSurfaceFlushedCallback(
        const std::function<void(bool surfaceFlushCallback)>& cb)
    {
        m_surfaceFlushCallback = cb;
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

    void registerCanRenderingCallback(
        const std::function<bool(PlatformWindow* wnd)>& cb);
    bool canRendering()
    {
        if (m_canRenderingCallback) {
            return m_canRenderingCallback(this);
        }
        return true;
    }

    virtual bool shouldDrawOnEveryRenderingCallback()
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
    void clearNativeHandlers();

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

    virtual void onClearDrawnBuffers()
    {
    }

    EventModifierData eventModifierData()
    {
        return m_eventModifierData;
    }

    CompositorContext* compostiorContext()
    {
        return m_compostiorContext;
    }

protected:
    PlatformWindow(Starfish* starfish);

    Starfish* m_starfish;
    WebView* m_webView;
    size_t m_renderingAnimator;
    CompositorContext* m_compostiorContext;
    EventModifierData m_eventModifierData;
    float m_lastMouseMoveX;
    float m_lastMouseMoveY;
    bool m_isDestroyed;

    std::function<void(PlatformWindow* wnd)> m_setNeedsRenderingCallback;
    std::function<RenderInfo(void)> m_renderingPrepareCallback;
    std::function<void(const RenderResult& renderResult)>
        m_renderingFinishedCallback;
    std::function<void()> m_showSoftwareKeyboardIfPossibleCallback;
    std::function<void()> m_hideSoftwareKeyboardIfPossibleCallback;

    std::function<void(PlatformWindow* wnd)> m_glMakeCurrentCallback;
    std::function<void(PlatformWindow* wnd, bool)> m_glSwapBufferCallback;

    std::function<bool(PlatformWindow* wnd)> m_canRenderingCallback;
    std::function<void(bool needsFlush)> m_surfaceFlushCallback;

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
