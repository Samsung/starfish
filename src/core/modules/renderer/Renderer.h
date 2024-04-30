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

#ifndef __StarfishRenderer__
#define __StarfishRenderer__

#include "core/page/RenderResult.h"
#include "core/event/EventModifierData.h"
#include "platform/canvas/gl/GL.h"

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
class GL;
class Starfish;
class String;

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

struct TransformationMatrix {
    static const TransformationMatrix& identityMatrix();

    double scaleX;
    double skewX;
    double translateX;
    double skewY;
    double scaleY;
    double translateY;
    double perspectiveX;
    double perspectiveY;
    double perspectiveScale;
};

class Renderer : public gc {
public:
    virtual ~Renderer(){};
    static Renderer* create(Starfish* starfish, uint32_t width,
                            uint32_t height);

    virtual uint32_t width() = 0;
    virtual uint32_t height() = 0;
    virtual void resizeTo(uint32_t w, uint32_t h)
    {
        STARFISH_LOG_INFO("Renderer::resizeTo %d %d", w, h);
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
        return nullptr;
    }
    virtual void updateDrawingBufferAddress(void* buf, uint32_t stride)
    {
    }

    virtual bool makeCurrent()
    {
        return true;
    }

    virtual void swapBuffers()
    {
    }

    virtual void mayNeedsSync()
    {
    }

    virtual uintptr_t createSharedContext()
    {
        return UINTPTR_MAX;
    }

    virtual bool destroyContext(uintptr_t context)
    {
        return true;
    }

    virtual bool clearCurrentContext()
    {
        return true;
    }

    // FIXME: get a context handle on context changes so that `0` only means the
    // handle is empty.
    static constexpr uintptr_t kEmptyContextOrUnknown = 0;
    virtual uintptr_t getCurrentContext()
    {
        return kEmptyContextOrUnknown;
    }

    virtual bool makeCurrentWithContext(uintptr_t context)
    {
        return true;
    }

    virtual void* getProcAddress(const char* name)
    {
        return nullptr;
    }

    virtual bool isSupportedExtension(const char* extension)
    {
        return false;
    }

    virtual TransformationMatrix screenMatrix()
    {
        return TransformationMatrix::identityMatrix();
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

    void registerOnMakeCurrent(
        const std::function<void(Renderer* renderer)>& cb)
    {
        m_onMakeCurrent = cb;
    }

    void registerOnSwapBuffers(
        const std::function<void(Renderer* renderer, bool mayNeedsSync)>& cb)
    {
        m_onSwapBuffer = cb;
    }

    void registerOnCreateSharedContext(
        const std::function<uintptr_t(Renderer* renderer)>& cb)
    {
        m_onCreateSharedContext = cb;
    }

    void registerOnDestroyContext(
        const std::function<bool(Renderer* renderer, uintptr_t)>& cb)
    {
        m_onDestroyContext = cb;
    }

    void registerOnClearCurrentContext(
        const std::function<bool(Renderer* renderer)>& cb)
    {
        m_onClearCurrentContext = cb;
    }

    void registerOnMakeCurrentWithContext(
        const std::function<bool(Renderer* renderer, uintptr_t)>& cb)
    {
        m_onMakeCurrentWithContext = cb;
    }

    void registerOnGetProcAddress(
        const std::function<void*(Renderer* renderer, const char*)>& cb)
    {
        m_onGetProcAddress = cb;
    }

    void registerOnIsSupportedExtension(
        const std::function<bool(Renderer* renderer, const char*)>& cb)
    {
        m_onIsSupportedExtension = cb;
    }

    void registerGetScreenMatrix(
        const std::function<TransformationMatrix(Renderer* renderer)>& cb)
    {
        m_getScreenMatrix = cb;
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
        const std::function<void(Renderer* renderer)>& cb)
    {
        m_setNeedsRenderingCallback = cb;
    }

    void registerCallbackHandler(WindowHandlerKind handlerKind,
                                 const std::function<void(void*)>& handler);
    void callHandler(WindowHandlerKind handlerKind, void* param);

    void registerCanRenderingCallback(
        const std::function<bool(Renderer* renderer)>& cb);
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
                                  Nullable<Node*> node);

    virtual RenderResult rendering();
    virtual void pause();
    virtual void resume();
    virtual void destroy();
    void setDevicePixelRatio(float dpr);
    float getDevicePixelRatio();

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
#if !defined(STARFISH_DALI) && !defined(STARFISH_EFL_HEADLESS)
    GL* gl();
#endif
protected:
    Renderer(Starfish* starfish);
    void clearNativeHandlers();

    Starfish* m_starfish;
    WebView* m_webView;
    size_t m_renderingAnimator;
#if !defined(STARFISH_DALI) && !defined(STARFISH_EFL_HEADLESS)
    std::unique_ptr<GL> m_gl;
#endif
    CompositorContext* m_compostiorContext;
    EventModifierData m_eventModifierData;
    float m_lastMouseMoveX;
    float m_lastMouseMoveY;
    bool m_isDestroyed;

    std::function<void(Renderer* renderer)> m_setNeedsRenderingCallback;

    std::function<RenderInfo(void)> m_renderingPrepareCallback;
    std::function<void(const RenderResult& renderResult)>
        m_renderingFinishedCallback;

    std::function<void()> m_showSoftwareKeyboardIfPossibleCallback;
    std::function<void()> m_hideSoftwareKeyboardIfPossibleCallback;

    std::function<void(Renderer* renderer)> m_onMakeCurrent;
    std::function<void(Renderer* renderer, bool)> m_onSwapBuffer;
    std::function<uintptr_t(Renderer* renderer)> m_onCreateSharedContext;
    std::function<bool(Renderer* renderer, uintptr_t context)>
        m_onDestroyContext;
    std::function<bool(Renderer* renderer)> m_onClearCurrentContext;
    std::function<bool(Renderer* renderer, uintptr_t context)>
        m_onMakeCurrentWithContext;
    std::function<void*(Renderer* renderer, const char*)> m_onGetProcAddress;
    std::function<bool(Renderer* renderer, const char*)>
        m_onIsSupportedExtension;

    std::function<TransformationMatrix(Renderer* renderer)> m_getScreenMatrix;

    std::function<bool(Renderer* renderer)> m_canRenderingCallback;
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
