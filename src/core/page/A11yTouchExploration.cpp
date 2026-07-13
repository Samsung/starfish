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

#include "StarfishConfig.h"

#ifdef STARFISH_ENABLE_A11Y_TOUCH_EXPLORATION

#include "core/page/A11yTouchExploration.h"

#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/DOMRect.h"
#include "core/dom/Element.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/Node.h"
#include "core/dom/Touch.h"
#include "core/modules/renderer/Renderer.h"
#include "core/modules/tts/TTS.h"
#include "core/modules/tts/TextAlternativeHelper.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"

namespace Starfish {

A11yTouchExploration::A11yTouchExploration(WebView* webView)
    : WebViewHoldable(webView)
    , m_state(GestureState::Idle)
    , m_focusedElement(nullptr)
    , m_touchStartX(0)
    , m_touchStartY(0)
    , m_touchStartTime(0)
    , m_lastTapX(0)
    , m_lastTapY(0)
    , m_lastTapTime(0)
    , m_isSettingDomFocus(false)
    , m_injectingSyntheticTap(false)
    , m_lastPointerWasMouse(false)
{
}

bool A11yTouchExploration::isEnabled() const
{
#ifdef STARFISH_ENABLE_TTS
    TTS* tts = webView()->tts();
    return tts &&
           (tts->isAccessibilityMode() || tts->mode() == LWE::TTSMode::Forced);
#else
    return false;
#endif
}

bool A11yTouchExploration::handleTouchEvent(TouchEventKind kind,
                                            TouchData* touches, size_t count)
{
    // Let our own synthesized activation tap flow through to the DOM.
    if (m_injectingSyntheticTap) {
        return false;
    }
    if (count < 1) {
        return false;
    }
    m_lastPointerWasMouse = false;
    PointerKind pk;
    switch (kind) {
    case TouchEventKind::TouchEventStart:
        pk = PointerKind::Down;
        break;
    case TouchEventKind::TouchEventMove:
        pk = PointerKind::Move;
        break;
    case TouchEventKind::TouchEventEnd:
        pk = PointerKind::Up;
        break;
    default:
        pk = PointerKind::Cancel;
        break;
    }
    return handlePointer(pk, touches[0].clientX(), touches[0].clientY(),
                         touches[0].timeStamp());
}

bool A11yTouchExploration::handleMouseEvent(MouseEventKind kind,
                                            MouseData& data)
{
    if (m_injectingSyntheticTap) {
        return false;
    }
    PointerKind pk;
    switch (kind) {
    case MouseEventKind::MouseEventDown:
        pk = PointerKind::Down;
        break;
    case MouseEventKind::MouseEventMove:
        pk = PointerKind::Move;
        break;
    case MouseEventKind::MouseEventUp:
        pk = PointerKind::Up;
        break;
    default:
        // Enter/Out carry no gesture meaning; leave them to the DOM.
        return false;
    }
    m_lastPointerWasMouse = true;
    return handlePointer(pk, data.clientX(), data.clientY(), data.timeStamp());
}

bool A11yTouchExploration::handlePointer(PointerKind kind, double x, double y,
                                         double t)
{
    // Diagnostic: log the first consumed pointer (and Down events) so device
    // logs show whether this interceptor is eating input.
    if (kind == PointerKind::Down) {
        STARFISH_LOG_INFO("[A11yTouch] consuming pointer down (%d, %d)", (int)x,
                          (int)y);
    }
    switch (kind) {
    case PointerKind::Down:
        m_touchStartX = x;
        m_touchStartY = y;
        m_touchStartTime = t;
        m_state = GestureState::Touching;
        break;

    case PointerKind::Move:
        if (m_state == GestureState::Touching) {
            if (std::abs(x - m_touchStartX) > kTapSlopPx ||
                std::abs(y - m_touchStartY) > kTapSlopPx) {
                m_state = GestureState::Swiping;
            }
        }
        break;

    case PointerKind::Up: {
        GestureState state = m_state;
        m_state = GestureState::Idle;
        if (state == GestureState::Swiping) {
            const double dx = x - m_touchStartX;
            const double dy = y - m_touchStartY;
            const double dist = std::sqrt(dx * dx + dy * dy);
            const double duration = t - m_touchStartTime;
            if (dist >= kSwipeMinDistancePx &&
                duration <= kSwipeMaxDurationMs) {
                // Dominant axis decides direction; down/right = next.
                bool forward;
                if (std::abs(dx) >= std::abs(dy)) {
                    forward = dx > 0;
                } else {
                    forward = dy > 0;
                }
                onSwipe(forward);
            }
        } else if (state == GestureState::Touching) {
            // Tap. A second tap within the double-tap window/slop of the
            // previous tap activates; otherwise it moves focus and speaks.
            bool isDoubleTap = m_focusedElement &&
                               (t - m_lastTapTime) <= kDoubleTapTimeoutMs &&
                               std::abs(x - m_lastTapX) <= kDoubleTapSlopPx &&
                               std::abs(y - m_lastTapY) <= kDoubleTapSlopPx;
            if (isDoubleTap) {
                onDoubleTap();
                // Consume this tap so the next one starts a fresh sequence.
                m_lastTapTime = 0;
            } else {
                onSingleTap(x, y);
                m_lastTapX = x;
                m_lastTapY = y;
                m_lastTapTime = t;
            }
        }
        break;
    }

    case PointerKind::Cancel:
        m_state = GestureState::Idle;
        break;
    }

    return true;
}

// ---- accessibility target predicates --------------------------------------

bool A11yTouchExploration::isAriaHiddenSelfOrAncestor(WebView* webView,
                                                      Node* node)
{
    const QualifiedName& hidden =
        webView->starfish()->staticStrings()->m_ariaHidden;
    Node* n = node;
    while (n) {
        if (n->isElement()) {
            String* v = n->asElement()->getAttributeOrEmpty(hidden);
            if (v->equalsIgnoreCase("true")) {
                return true;
            }
        }
        n = n->parentElement();
    }
    return false;
}

bool A11yTouchExploration::isA11yFocusable(WebView* webView, Element* element)
{
    if (!element) {
        return false;
    }
    // Not rendered / hidden by CSS.
    if (!element->hasFocusableStyle()) {
        return false;
    }
    // aria-hidden on the element or any ancestor removes it from a11y.
    if (isAriaHiddenSelfOrAncestor(webView, element)) {
        return false;
    }

    StaticStrings* ss = webView->starfish()->staticStrings();
    String* role = element->getAttributeOrEmpty(ss->m_role);
    if (role->equalsIgnoreCase("presentation") ||
        role->equalsIgnoreCase("none")) {
        return false;
    }

    // Interactive native controls.
    if (element->isHTMLButtonElement() || element->isHTMLAnchorElement() ||
        element->isHTMLInputElement() || element->isHTMLSelectElement() ||
        element->isHTMLTextAreaElement()) {
        return true;
    }
    // Images only when they carry an accessible name; chromium-efl's
    // IsAccessible (ax_platform_node_efl.cc) rejects nameless non-focusable
    // objects, and a nameless image in the swipe order is just a silent
    // stop.
    if (element->isHTMLImageElement()) {
        return !element->getAttributeOrEmpty(ss->m_alt)->isEmpty() ||
               !element->getAttributeOrEmpty(ss->m_ariaLabel)->isEmpty() ||
               !element->getAttributeOrEmpty(ss->m_ariaLabelledby)->isEmpty();
    }
    // Explicitly focusable (author-set tabindex).
    if (element->tabIndexSetExplicitly() && element->tabIndex() >= 0) {
        return true;
    }
    // Recognized roles (Starfish only maps button/link).
    if (role->equals("button") || role->equalsIgnoreCase("link")) {
        return true;
    }
    // Anything carrying an explicit accessible name.
    if (!element->getAttributeOrEmpty(ss->m_ariaLabel)->isEmpty() ||
        !element->getAttributeOrEmpty(ss->m_ariaLabelledby)->isEmpty()) {
        return true;
    }
    return false;
}

// ---- hit test / traversal -------------------------------------------------

Element* A11yTouchExploration::hitTestA11yTarget(double clientX, double clientY)
{
    BrowsingContext* bc = webView()->mainBrowsingContext();
    if (!bc) {
        return nullptr;
    }
    Window* win = bc->window();
    // hitTest() works in page coordinates (window + scroll offset), mirroring
    // BrowsingContext::dispatchTouchEvent.
    double pageX = clientX + win->scrollX(false);
    double pageY = clientY + win->scrollY(false);
    Node* node = bc->hitTest(pageX, pageY);
    if (!node) {
        return nullptr;
    }
    Element* element =
        node->isElement() ? node->asElement() : node->parentElement();
    // Promote to the nearest ancestor that is an accessibility target. An
    // aria-hidden node fails isA11yFocusable and is skipped past here.
    while (element && !isA11yFocusable(webView(), element)) {
        element = element->parentElement();
    }
    return element;
}

// Pre-order successor, skipping the subtree of an element that is itself
// aria-hidden (its ancestors are already known non-hidden while descending).
static Node* preOrderNext(WebView* webView, Node* node)
{
    const QualifiedName& hidden =
        webView->starfish()->staticStrings()->m_ariaHidden;
    bool selfHidden =
        node->isElement() &&
        node->asElement()->getAttributeOrEmpty(hidden)->equalsIgnoreCase(
            "true");
    if (node->firstChild() && !selfHidden) {
        return node->firstChild();
    }
    Node* n = node;
    while (n) {
        if (n->nextSibling()) {
            return n->nextSibling();
        }
        n = n->parentNode();
    }
    return nullptr;
}

// Reverse pre-order predecessor. Descends into the previous sibling's last
// non-hidden descendant, else walks up to the parent.
static Node* preOrderPrev(WebView* webView, Node* node)
{
    const QualifiedName& hidden =
        webView->starfish()->staticStrings()->m_ariaHidden;
    if (node->previousSibling()) {
        Node* n = node->previousSibling();
        while (true) {
            bool selfHidden =
                n->isElement() &&
                n->asElement()->getAttributeOrEmpty(hidden)->equalsIgnoreCase(
                    "true");
            if (n->lastChild() && !selfHidden) {
                n = n->lastChild();
            } else {
                break;
            }
        }
        return n;
    }
    return node->parentNode();
}

Element* A11yTouchExploration::nextA11yElement(Element* from, bool forward)
{
    BrowsingContext* bc = webView()->mainBrowsingContext();
    if (!bc) {
        return nullptr;
    }
    Document* doc = bc->document();
    if (!doc) {
        return nullptr;
    }
    Node* start = from ? static_cast<Node*>(from) : static_cast<Node*>(doc);
    Node* n = forward ? preOrderNext(webView(), start)
                      : preOrderPrev(webView(), start);
    while (n) {
        if (n->isElement() && isA11yFocusable(webView(), n->asElement())) {
            return n->asElement();
        }
        n = forward ? preOrderNext(webView(), n) : preOrderPrev(webView(), n);
    }
    return nullptr;
}

// ---- actions --------------------------------------------------------------

void A11yTouchExploration::onSingleTap(double clientX, double clientY)
{
    Element* target = hitTestA11yTarget(clientX, clientY);
    if (!target) {
        return;
    }
    setA11yFocus(target);
    speak(target);
}

void A11yTouchExploration::onSwipe(bool forward)
{
    Element* next = nextA11yElement(m_focusedElement, forward);
    if (!next) {
        return;
    }
    next->scrollIntoViewIfNeeded();
    setA11yFocus(next);
    speak(next);
}

void A11yTouchExploration::onDoubleTap()
{
    if (!m_focusedElement) {
        return;
    }
    activate(m_focusedElement, m_lastPointerWasMouse);
}

void A11yTouchExploration::activate(Element* target, bool asMouse)
{
    if (!target) {
        return;
    }
    target->scrollIntoViewIfNeeded();

    DOMRect* rect = target->getBoundingClientRect();
    activateAt(rect->x() + rect->width() / 2.0,
               rect->y() + rect->height() / 2.0, asMouse);
}

void A11yTouchExploration::activateAt(double clientX, double clientY,
                                      bool asMouse)
{
    double cx = clientX;
    double cy = clientY;

    // Clamp to the viewport so the synthesized tap lands on-screen.
    Window* win = webView()->mainBrowsingContext()->window();
    double vw = win->innerWidth();
    double vh = win->innerHeight();
    if (cx < 0)
        cx = 0;
    else if (cx > vw - 1)
        cx = vw - 1;
    if (cy < 0)
        cy = 0;
    else if (cy > vh - 1)
        cy = vh - 1;

    // Replay a real tap through the renderer so the normal press->click path
    // (down/pointerdown/.../click, :active handling) runs. Follows the dpr
    // convention of Window::simulateClick/simulateMouseDown: multiply here, the
    // renderer divides again. Replay matches the input source: on mouse-driven
    // devices a synthesized touch would not reach the click path.
    float dpr = webView()->screenInfo().devicePixelRatio;
    m_injectingSyntheticTap = true;
    if (asMouse) {
        MouseData down(MouseButtonValue::LeftButton,
                       MouseButtonsValue::LeftButtonDown, cx * dpr, cy * dpr,
                       0);
        webView()->renderer()->dispatchMouseEvent(
            MouseEventKind::MouseEventDown, down, true);
        MouseData up(MouseButtonValue::NoButton,
                     MouseButtonsValue::NoButtonDown, cx * dpr, cy * dpr, 0);
        webView()->renderer()->dispatchMouseEvent(MouseEventKind::MouseEventUp,
                                                  up, true);
    } else {
        TouchData data(cx * dpr, cy * dpr);
        webView()->renderer()->dispatchTouchEvent(
            TouchEventKind::TouchEventStart, &data, 1);
        webView()->renderer()->dispatchTouchEvent(TouchEventKind::TouchEventEnd,
                                                  &data, 1);
    }
    m_injectingSyntheticTap = false;
}

void A11yTouchExploration::setA11yFocus(Element* element)
{
    m_focusedElement = element;
    if (element->isFocusable()) {
        // Keep DOM focus in sync for focusable targets. Guard the focus-event
        // speech hook so we don't speak twice.
        m_isSettingDomFocus = true;
        element->focus();
        m_isSettingDomFocus = false;
    }
}

void A11yTouchExploration::speak(Element* element)
{
#ifdef STARFISH_ENABLE_TTS
    TextAlternativeHelper tah(webView());
    String* text = tah.getComputedTextAlternative(element);
    if (text->length() == 0) {
        return;
    }
    webView()->tts()->speech(element, text);
#endif
}

} // namespace Starfish

#endif
