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

#ifdef STARFISH_ENABLE_A11Y_TOUCH_EXPLORATION
#ifndef __StarfishA11yTouchExploration__
#define __StarfishA11yTouchExploration__

#include "binding/WebViewHoldable.h"
#include "core/modules/renderer/Renderer.h"

namespace Starfish {

class Element;
class Node;
class TouchData;
class MouseData;

// Touch-exploration accessibility controller (non-TV Tizen profiles).
//   - single tap     : move accessibility focus to the hit target and speak
//                       its accessible name (aria-label / computed text).
//   - double tap     : activate (synthesize a real tap on) the focused target.
//   - horiz/vert swipe: move accessibility focus to the next/prev target and
//                       speak it.
// Gated at runtime by TTS accessibility mode; when active it consumes pointer
// events so they don't fall through to the normal DOM dispatch. Both touch and
// mouse input are handled: on some devices (e.g. Family Hub) a screen tap is
// delivered as a mouse-down/up rather than a touch event.
class A11yTouchExploration : public gc, public WebViewHoldable {
public:
    explicit A11yTouchExploration(WebView* webView);

    // Called from WebView::dispatch{Touch,Mouse}Event. Returns true if the
    // event was consumed (must not be forwarded to the browsing context).
    bool handleTouchEvent(TouchEventKind kind, TouchData* touches,
                          size_t count);
    bool handleMouseEvent(MouseEventKind kind, MouseData& data);

    bool isEnabled() const;

    // Guards against double-speaking: while true, the focus event hook in
    // Element must not re-speak (this controller already spoke).
    bool isSettingDomFocus() const
    {
        return m_isSettingDomFocus;
    }

    // Shared with the AT-SPI bridge tree source: target predicate, hit test,
    // pre-order traversal, and activation (synthesized tap that bypasses this
    // controller's own interception).
    static bool isA11yFocusable(WebView* webView, Element* element);
    Element* hitTestA11yTarget(double clientX, double clientY);
    Element* nextA11yElement(Element* from, bool forward);
    void activate(Element* target, bool asMouse);
    // Replay an activation tap at the given top-level viewport CSS px
    // (clamped to the viewport). Callers scroll the target on screen and
    // compute frame-aware coordinates themselves.
    void activateAt(double clientX, double clientY, bool asMouse);

#ifdef STARFISH_ENABLE_TEST
    Element* focusedElementForTest() const
    {
        return m_focusedElement;
    }
#endif

private:
    enum class GestureState { Idle, Touching, Swiping };
    enum class PointerKind { Down, Move, Up, Cancel };

    // Shared gesture state machine for both touch and mouse input.
    bool handlePointer(PointerKind kind, double x, double y, double t);

    // Gesture thresholds, in CSS px / ms. Coordinates reaching WebView are
    // already device-pixel-ratio normalized. Values follow Android/ChromeOS
    // touch-exploration defaults (double-tap 300ms, double-tap slop 100px),
    // reusing Starfish's existing 20px tap slop.
    static constexpr double kTapSlopPx = 20.0;
    static constexpr double kDoubleTapSlopPx = 100.0;
    static constexpr double kDoubleTapTimeoutMs = 300.0;
    static constexpr double kSwipeMinDistancePx = 60.0;
    static constexpr double kSwipeMaxDurationMs = 500.0;

    void onSingleTap(double clientX, double clientY);
    void onDoubleTap();
    void onSwipe(bool forward);

    static bool isAriaHiddenSelfOrAncestor(WebView* webView, Node* node);
    void setA11yFocus(Element* element);
    void speak(Element* element);

    // In-class initializers (not just the ctor list) so every member is
    // guaranteed zero-initialized even under aggressive release optimization
    // where the constructor's init list was observed to be skipped on device
    // (m_injectingSyntheticTap read back as garbage, blocking handlePointer).
    GestureState m_state{ GestureState::Idle };
    Element* m_focusedElement{ nullptr };
    // First-touch position/time of the in-progress gesture.
    double m_touchStartX{ 0 };
    double m_touchStartY{ 0 };
    double m_touchStartTime{ 0 };
    // Position/time of the previous completed tap (for double-tap detection).
    double m_lastTapX{ 0 };
    double m_lastTapY{ 0 };
    double m_lastTapTime{ 0 };
    bool m_isSettingDomFocus{ false };
    bool m_injectingSyntheticTap{ false };
    // Whether the last handled gesture came from mouse input; decides how the
    // double-tap activation replays (mouse vs touch).
    bool m_lastPointerWasMouse{ false };
};

} // namespace Starfish

#endif
#endif
