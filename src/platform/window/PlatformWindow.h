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

#ifndef __StarFishPlatformWindow__
#define __StarFishPlatformWindow__

namespace StarFish {

class AnimationExecutor;
class Canvas;
class Node;
class WebView;
class ImageData;
class MouseData;
class TouchData;
class KeyboardData;

class PlatformWindow : public gc {
public:
    enum TouchEventKind {
        TouchEventStart,
        TouchEventMove,
        TouchEventEnd,
        TouchEventCancel
    };

    enum KeyEventKind { KeyEventDown, KeyEventUp };

    enum MouseEventKind {
        MouseEventDown,
        MouseEventMove,
        MouseEventUp,
        MouseEventEnter,
        MouseEventOut
    };

    enum CompositionEventKind {
        CompositionEventStart,
        CompositionEventUpdate,
        CompositionEventEnd,
    };

    virtual ~PlatformWindow();

    static PlatformWindow* create(StarFish* starFish, void* win, int width,
                                  int height);

    virtual int32_t width() = 0;
    virtual int32_t height() = 0;
    virtual void resizeTo(int w, int h) = 0;
    virtual void* unwrap() = 0;
    virtual void clearResources() = 0;
    virtual Canvas* preparePainting(bool forPainting) = 0;
    virtual void showSoftwareKeyboardIfPossible()
    {
    }
    virtual void hideSoftwareKeyboardIfPossible()
    {
    }
    virtual bool isIMEEnabledNow()
    {
        return false;
    }

    void dispatchTouchEvent(TouchEventKind kind, TouchData* touches,
                            size_t touchCount);
    void dispatchMouseEvent(MouseEventKind kind, MouseData data);
    void dispatchMouseWheelEvent(
        float screenX, float screenY, int z,
        bool isVerticalWheelEvent); // z : -1(up, left) or 1(down, right)
    void dispatchKeyEvent(KeyEventKind kind, KeyboardData data);
    void dispatchCompositionEvent(CompositionEventKind kind, String* data);

    bool rendering();
    void pause();
    void resume();
    void close();

    void paintWindowBackground(Canvas* canvas);
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

    void registerOrUpdateIdleTimeCleaner();

protected:
    PlatformWindow(StarFish* starFish);

    StarFish* m_starFish;
    WebView* m_webView;
    size_t m_idleCleanerTimerID;

#ifdef STARFISH_ENABLE_VIRTUAL_CURSOR
    bool m_isButtonOfVirtualCursorClicked;
    int m_virtualCursorX;
    int m_virtualCursorY;
    int m_virtualCursorSpeed;
    uint64_t m_virtualCursorMoveingLastTimestamp;
    ImageData* m_virtualCursorImageData;
#endif
};
}

#endif
