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

#ifndef __StarFishPlatformWindow__
#define __StarFishPlatformWindow__

namespace StarFish {

class AnimationExecutor;
class Canvas;
class Compositor;
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

class PlatformWindow : public gc {
public:
    virtual ~PlatformWindow(){};
    static PlatformWindow* create(StarFish* starFish, void* win, int width,
                                  int height);

    virtual int32_t width() = 0;
    virtual int32_t height() = 0;
    virtual void resizeTo(int w, int h)
    {
        onResize();
    }
    virtual void* unwrap() = 0;
    virtual void clearResources() = 0;
    virtual void setNeedsRendering() = 0;
    virtual Canvas* preparePainting() = 0;
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
#if !defined(PORT_GRAPHIC_BACKEND_GENERAL_BUFFER)
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
#endif
        return nullptr;
    }
    virtual void updateDrawingBufferAddress(void* buf, uint32_t width,
                                            uint32_t height, uint32_t stride)
    {
#if defined(PORT_GRAPHIC_BACKEND_GENERAL_BUFFER)
        resizeTo(width, height);
#else
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
#endif
    }

    void registerRenderingFinishedCallback(const std::function<void()>& cb)
    {
        m_renderingFinishedCallback = cb;
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

    virtual bool rendering();
    virtual void pause();
    virtual void resume();
    virtual void close();

    bool isClosed()
    {
        return m_isClosed;
    }

#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
    void paintVirtualCursor(Canvas* canvas);
#endif
    WebView* webView()
    {
        return m_webView;
    }

    StarFish* starFish()
    {
        return m_starFish;
    }

    void setWebView(WebView* webView);
    void screenShot(std::string filePath);
    void onResize();
    virtual void onIdle()
    {
    }

    void registerOrUpdateIdleTimeCleaner();

protected:
    PlatformWindow(StarFish* starFish);

    bool m_isClosed;
    StarFish* m_starFish;
    WebView* m_webView;
    size_t m_idleCleanerTimerID;

    std::function<void()> m_renderingFinishedCallback;
    std::function<void()> m_showSoftwareKeyboardIfPossibleCallback;
    std::function<void()> m_hideSoftwareKeyboardIfPossibleCallback;

#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
    bool m_isButtonOfVirtualCursorClicked;
    int m_virtualCursorX;
    int m_virtualCursorY;
    int m_virtualCursorSpeed;
    uint64_t m_virtualCursorMoveingLastTimestamp;
    NativeImageData* m_virtualCursorImageData;
#endif
};
}

#endif
