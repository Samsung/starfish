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
#include "Starfish.h"
#include "core/dom/EventTarget.h"
#include "core/dom/Scrolling.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/TouchEvent.h"
#include "core/dom/TouchList.h"
#include "core/dom/UIEvent.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/StackingContext.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/message_loop/Timer.h"

#define STARFISH_SCROLL_START_THRESHOLD 10
#define STARFISH_SCROLL_START_FLING_THRESHOLD 100
#define STARFISH_SCROLL_FLING_LENGTH_MULTIPLY_BASE 500
#define STARFISH_SCROLL_FLING_LENGTH_MULTIPLY_RATIO 1500
#define STARFISH_SCROLL_FLING_BASE_TIME_IN_MS 1000
#define STARFISH_SCROLL_FLING_SPEED_RATIO 1
#define STARFISH_SCROLL_ACTIVE_TIME_IN_MS 500

namespace Starfish {

void* Scrolling::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(Scrolling));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(Scrolling)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(Scrolling, m_target));
        GC_set_bit(desc, GC_WORD_OFFSET(Scrolling, m_lastScrollingData));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(Scrolling));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

bool Scrolling::handleDefaultEvent(Event* event, Window* window,
                                   FrameBlockBox* frame, OverflowValue ox,
                                   OverflowValue oy)
{
#if defined(STARFISH_DISABLE_OVERFLOW_SCROLL)
    return false;
#endif
    bool horizontalScrollEnabled =
        ox >= OverflowValue::AutoOverflow &&
        frame->asFrameBlockBox()->hasBiggerContentThanFrameWidth();
    bool verticalScrollEnabled =
        oy >= OverflowValue::AutoOverflow &&
        frame->asFrameBlockBox()->hasBiggerContentThanFrameHeight();

    if (!m_isScrollTarget &&
        (horizontalScrollEnabled || verticalScrollEnabled)) {
        bool isPointingDownEvent = false;
        bool isPointingUpEvent = false;
        bool shouldProcess = false;
        float x, y;
        DOMTimeStamp timeStamp = 0;
        if (event->isMouseEvent()) {
            if (event->type()->equals("mousedown")) {
                isPointingDownEvent = true;
                shouldProcess = true;
                x = event->asMouseEvent()->screenX();
                y = event->asMouseEvent()->screenY();
                timeStamp = event->asMouseEvent()->timeStamp();
            } else if (event->type()->equals("mousemove")) {
                shouldProcess = true;
                x = event->asMouseEvent()->screenX();
                y = event->asMouseEvent()->screenY();
                timeStamp = event->asMouseEvent()->timeStamp();
            } else if (event->type()->equals("mouseup")) {
                shouldProcess = true;
                isPointingUpEvent = true;
            }
        } else if (event->isTouchEvent()) {
            if (event->type()->equals("touchstart")) {
                isPointingDownEvent = true;
                shouldProcess = true;
                x = event->asTouchEvent()->touches()->at(0)->screenX();
                y = event->asTouchEvent()->touches()->at(0)->screenY();
                timeStamp = event->asTouchEvent()->timeStamp();
            } else if (event->type()->equals("touchmove")) {
                shouldProcess = true;
                x = event->asTouchEvent()->touches()->at(0)->screenX();
                y = event->asTouchEvent()->touches()->at(0)->screenY();
                timeStamp = event->asTouchEvent()->timeStamp();
            } else if (event->type()->equals("touchend")) {
                shouldProcess = true;
                isPointingUpEvent = true;
            }
        }
        if (shouldProcess) {
            if (isPointingDownEvent) {
                m_pointingEventX = x;
                m_pointingEventY = y;
                m_pointingEventTimeStamp = timeStamp;
                m_gotPointingDownEvent = true;
                return true;
            } else if (isPointingUpEvent) {
                m_gotPointingDownEvent = false;
            } else if (m_gotPointingDownEvent) {
                unsigned t = STARFISH_SCROLL_START_THRESHOLD;

                if (std::abs(m_pointingEventY - y) > t &&
                    verticalScrollEnabled) {
                    m_inVerticalScrolling = true;
                } else if (std::abs(m_pointingEventX - x) > t &&
                           horizontalScrollEnabled) {
                    m_inHorizontalScrolling = true;
                }

                if (!m_isScrollTarget &&
                    (m_inVerticalScrolling || m_inHorizontalScrolling)) {
                    window->browsingContext()
                        ->webView()
                        ->addGlobalPointingEventInterceptListener(m_target);
                    m_isScrollTarget = true;
                    window->browsingContext()
                        ->webView()
                        ->setScrollOccurredDuringGesture(true);
                    m_pointingEventX = m_lastPointingEventX = x;
                    m_pointingEventY = m_lastPointingEventY = y;
                    m_pointingEventTimeStamp = timeStamp;
                    if (!m_inAnimation) {
                        Window* window = m_target->isWindow()
                                             ? m_target->asWindow()
                                             : m_target->asElement()->window();
                        m_inAnimation = true;
                        window->webView()->timer()->requestAnimationFrame(
                            window, onAnimationFrameHandler, this);
                    }

                    window->webView()->activeScrollingSet().insert(this);
                }
                return true;
            }
        }
    }
    return false;
}

static float flingInterpolationFunction(float pos)
{
    return -pow(2, -10 * pos) + 1;
}

void Scrolling::onAnimationFrameHandler(void* data)
{
    Scrolling* self = (Scrolling*)data;

    if (!self->m_isScrollTarget) {
        // set repaint or recomposite
        self->giveDamageToTarget(true);

        auto currentTime = timestamp();
        if (currentTime - self->m_lastActiveTime <
            STARFISH_SCROLL_ACTIVE_TIME_IN_MS) {
            // we can continue
        } else {
            // end animation
            self->m_inAnimation = false;
            return;
        }
    } else if (self->m_inHorizontalFling || self->m_inVerticalFling) {
        auto currentTime = timestamp();
        uint32_t flingLength = STARFISH_SCROLL_FLING_BASE_TIME_IN_MS;

        if (STARFISH_SCROLL_FLING_LENGTH_MULTIPLY_BASE <
            std::abs(self->m_flingStartSpeed)) {
            flingLength *=
                std::abs(self->m_flingStartSpeed /
                         STARFISH_SCROLL_FLING_LENGTH_MULTIPLY_RATIO);
        }

        bool shouldExitFling =
            (currentTime - self->m_flingStartTime) > (uint64_t(flingLength));

        float progress =
            shouldExitFling ? 1
                            : (double)((currentTime - self->m_flingStartTime)) /
                                  flingLength;
        progress = flingInterpolationFunction(progress);
        if (progress >= 0.95) {
            shouldExitFling = true;
        }

        if (shouldExitFling) {
            // end
            self->stopFling();
            self->stopScrolling();
        } else {
            float speed = self->m_flingStartSpeed * (1 - progress);
            float distance =
                speed * ((currentTime - self->m_flingProcessingTime) / 1000.f);

            bool isScrollEffective = false;
            if (self->m_target->isElement()) {
                if (self->m_inVerticalScrolling) {
                    isScrollEffective =
                        self->m_target->asElement()->setScrollTop(
                            self->m_target->asElement()->scrollTop() +
                            distance);
                } else if (self->m_inHorizontalScrolling) {
                    isScrollEffective =
                        self->m_target->asElement()->setScrollLeft(
                            self->m_target->asElement()->scrollLeft() +
                            distance);
                }
            } else {
                if (self->m_inVerticalScrolling) {
                    isScrollEffective = self->m_target->asWindow()->scrollTo(
                        self->m_target->asWindow()->scrollX(),
                        self->m_target->asWindow()->scrollY() + distance);
                } else if (self->m_inHorizontalScrolling) {
                    isScrollEffective = self->m_target->asWindow()->scrollTo(
                        self->m_target->asWindow()->scrollX() + distance,
                        self->m_target->asWindow()->scrollY());
                }
            }

            self->m_flingProcessingTime = currentTime;

            if (!isScrollEffective) {
                self->stopFling();
                self->stopScrolling();
            } else {
                self->m_lastActiveTime = currentTime;
            }
        }
    } else {
        float dx = self->m_lastPointingEventX - self->m_pointingEventX;
        float dy = self->m_lastPointingEventY - self->m_pointingEventY;

        auto currentTime = timestamp();
        if (self->m_inVerticalScrolling) {
            if (dy > 0) {
                self->m_inVerticalScrollingDown = true;
                self->m_inVerticalScrollingUp = false;
            } else {
                self->m_inVerticalScrollingDown = false;
                self->m_inVerticalScrollingUp = true;
            }
            self->m_lastScrollingData.push_back(
                std::make_pair(currentTime, dy));
        }
        if (self->m_inHorizontalScrolling) {
            if (dx > 0) {
                self->m_inHorizontalScrollingLeft = true;
                self->m_inHorizontalScrollingRight = false;
            } else {
                self->m_inHorizontalScrollingLeft = false;
                self->m_inHorizontalScrollingRight = true;
            }
            self->m_lastScrollingData.push_back(
                std::make_pair(currentTime, dx));
        }

        // collect datas within 100ms
        while (self->m_lastScrollingData.size()) {
            if (currentTime - self->m_lastScrollingData.front().first < 100) {
                break;
            } else {
                self->m_lastScrollingData.erase(
                    self->m_lastScrollingData.begin());
            }
        }

        if (self->m_target->isElement()) {
            if (self->m_inVerticalScrolling) {
                self->m_target->asElement()->setScrollTop(
                    self->m_target->asElement()->scrollTop() + dy);
            } else if (self->m_inHorizontalScrolling) {
                self->m_target->asElement()->setScrollLeft(
                    self->m_target->asElement()->scrollLeft() + dx);
            }
        } else {
            if (self->m_inVerticalScrolling) {
                self->m_target->asWindow()->scrollTo(
                    self->m_target->asWindow()->scrollX(),
                    self->m_target->asWindow()->scrollY() + dy);
            } else if (self->m_inHorizontalScrolling) {
                self->m_target->asWindow()->scrollTo(
                    self->m_target->asWindow()->scrollX() + dx,
                    self->m_target->asWindow()->scrollY());
            }
        }

        self->m_lastPointingEventX = self->m_pointingEventX;
        self->m_lastPointingEventY = self->m_pointingEventY;
        self->m_lastActiveTime = currentTime;
    }

    self->m_inAnimation = true;
    Window* window = self->m_target->executionContext()->document()->window();
    window->browsingContext()->webView()->timer()->requestAnimationFrame(
        window, onAnimationFrameHandler, self);
}

void Scrolling::onGlobalPointingEvent(float x, float y, DOMTimeStamp timeStamp,
                                      EventTarget::GlobalPointingEventKind kind)
{
#if defined(STARFISH_DISABLE_OVERFLOW_SCROLL)
    return;
#endif
    if (kind == EventTarget::GlobalPointingEventKindUp) {
        bool userWantsFling = false;
        float postiveAverage = 0;
        float negativeAverage = 0;
        float flingSpeed = 0;

        uint64_t t = 0;
        if (m_lastScrollingData.size()) {
            t = m_lastScrollingData[0].first;
        }
        for (size_t i = 1; i < m_lastScrollingData.size(); i++) {
            auto td = m_lastScrollingData[i].first - t;
            if (td != 0) {
                float speed = m_lastScrollingData[i].second / (td / 1000.f);
                if (speed > 0) {
                    postiveAverage += speed;
                } else {
                    negativeAverage += speed;
                }
                t = m_lastScrollingData[i].first;
            }
        }

        if (m_lastScrollingData.size() > 1) {
            postiveAverage /= (float)(m_lastScrollingData.size() - 1);
            postiveAverage *= STARFISH_SCROLL_FLING_SPEED_RATIO;
            negativeAverage /= (float)(m_lastScrollingData.size() - 1);
            negativeAverage *= STARFISH_SCROLL_FLING_SPEED_RATIO;
        }

        WebView* webView = m_target->executionContext()
                               ->document()
                               ->browsingContext()
                               ->webView();
        const float threshold = STARFISH_SCROLL_START_FLING_THRESHOLD /
                                webView->screenInfo().devicePixelRatio;

        if (postiveAverage >= threshold || -negativeAverage >= threshold) {
            userWantsFling = true;
        }

        if (userWantsFling) {
            m_inHorizontalFling = m_inHorizontalScrolling;
            m_inVerticalFling = m_inVerticalScrolling;
            m_flingProcessingTime = m_flingStartTime = timeStamp;
            m_flingStartSpeed = (postiveAverage > -negativeAverage)
                                    ? postiveAverage
                                    : negativeAverage;
        } else {
            stopScrolling();
        }

        m_lastScrollingData.clear();
    } else if (kind == EventTarget::GlobalPointingEventKindMove) {
        m_pointingEventX = x;
        m_pointingEventY = y;
        m_pointingEventTimeStamp = timeStamp;
    } else if (kind == EventTarget::GlobalPointingEventKindDown) {
        if (m_inHorizontalFling || m_inVerticalFling) {
            m_pointingEventX = m_lastPointingEventX = x;
            m_pointingEventY = m_lastPointingEventY = y;
            m_pointingEventTimeStamp = timeStamp;
            stopFling();
        }
    }
}

void Scrolling::stopScrolling()
{
    m_inHorizontalScrolling = false;
    m_inVerticalScrolling = false;
    m_isScrollTarget = false;
    m_gotPointingDownEvent = false;

    WebView* webView =
        m_target->executionContext()->document()->browsingContext()->webView();
    webView->removeGlobalPointingEventInterceptListener(m_target);
    webView->activeScrollingSet().erase(this);
}

void Scrolling::stopFling()
{
    m_inHorizontalFling = m_inVerticalFling = false;
}

void Scrolling::markAsActive()
{
    if (!m_target->isWindow()) {
        auto frame = m_target->asElement()->frame();
        if (frame) {
            auto ao = frame->appliedOverflow();
            auto ox = ao.first;
            auto oy = ao.second;
            if (ox < OverflowValue::AutoOverflow &&
                oy < OverflowValue::AutoOverflow) {
                return;
            }
        }
    }

    m_lastActiveTime = timestamp();

    if (!m_inAnimation) {
        Window* window = m_target->isWindow() ? m_target->asWindow()
                                              : m_target->asElement()->window();
        m_inAnimation = true;
        window->webView()->timer()->requestAnimationFrame(
            window, onAnimationFrameHandler, this);
    }
}

void Scrolling::dispatchPendingScrollEventIfNeeded()
{
    if (!m_hasPendingScrollEvent) {
        return;
    }
    m_hasPendingScrollEvent = false;

    Window* window = m_target->isWindow() ? m_target->asWindow()
                                          : m_target->asElement()->window();
    ExecutionContext* ec = m_target->executionContext();
    String* type = ec->starfish()->staticStrings()->m_scroll.localName();
    UIEvent* e = new UIEvent(ec, type);
    e->setView(window);
    e->setTarget(m_target->isWindow() ? (EventTarget*)window->document()
                                      : m_target);
    m_target->dispatchEventByUA(e);
}

static const char* repaintingWhenScrollingReasonToString(unsigned reason)
{
    switch (reason) {
    case RepaintingWhenScrollingReasonNoGraphicsBuffer:
        return "NoGraphicsBuffer";
    case RepaintingWhenScrollingReasonBorder:
        return "Border";
    case RepaintingWhenScrollingReasonBoxShadow:
        return "BoxShadow";
    case RepaintingWhenScrollingReasonOutline:
        return "Outline";
    case RepaintingWhenScrollingReasonBackgroundSize:
        return "BackgroundSize";
    default:
        return "None";
    }
}

void Scrolling::logSlowScrollPathIfNeeded(unsigned reason, Node* node)
{
    uint64_t now = timestamp();
    if (reason == m_lastSlowScrollPathLogReason &&
        now - m_lastSlowScrollPathLogTime < 1000) {
        return;
    }
    m_lastSlowScrollPathLogReason = reason;
    m_lastSlowScrollPathLogTime = now;

    if (node && node->isElement()) {
        STARFISH_LOG_INFO(
            "[scroll] slow path (repaint on every scroll frame) on <%s> "
            "id(%s) className(%s), reason: %s",
            node->localName()->toUTF8NonGCString().data(),
            node->asElement()->id()->toUTF8NonGCString().data(),
            node->asElement()->className()->toUTF8NonGCString().data(),
            repaintingWhenScrollingReasonToString(reason));
    } else {
        STARFISH_LOG_INFO(
            "[scroll] slow path (repaint on every scroll frame) on "
            "#document, reason: %s",
            repaintingWhenScrollingReasonToString(reason));
    }
}

void Scrolling::giveDamageToTarget(bool inScrollbarAppearingOrDisappearing)
{
    if (m_target->isWindow()) {
        StackingContext* ctx = m_target->asWindow()
                                   ->document()
                                   ->html()
                                   ->frame()
                                   ->asFrameBox()
                                   ->stackingContext();
        if (ctx && ctx->needsGraphicsBuffer()) {
            RepaintingWhenScrollingReason reason =
                ctx->repaintingWhenScrollingReason();
            if (inScrollbarAppearingOrDisappearing ||
                reason == RepaintingWhenScrollingReasonNone) {
                m_target->asWindow()
                    ->webView()
                    ->markNeedsCompositeConsiderInRendering();

            } else {
                logSlowScrollPathIfNeeded(reason, ctx->owner()->node());
                ctx->owner()->node()->setNeedsPainting();
            }

        } else {
            logSlowScrollPathIfNeeded(
                RepaintingWhenScrollingReasonNoGraphicsBuffer,
                ctx ? ctx->owner()->node() : nullptr);
            m_target->asWindow()->document()->setNeedsPainting();

            if (!m_target->asWindow()
                     ->browsingContext()
                     ->isTopLevelBrowsingContext()) {
                m_target->asWindow()
                    ->browsingContext()
                    ->sourceElement()
                    ->setNeedsPainting();
            }
        }
    } else {
        unsigned slowPathReason = RepaintingWhenScrollingReasonNoGraphicsBuffer;
        if (m_target->asElement()->frame() &&
            m_target->asElement()->frame()->isFrameBox() &&
            m_target->asElement()->frame()->asFrameBox()->stackingContext()) {
            FrameBox* box = m_target->asElement()->frame()->asFrameBox();
            StackingContext* sc = box->stackingContext();

            RepaintingWhenScrollingReason reason =
                sc->repaintingWhenScrollingReason();
            if (inScrollbarAppearingOrDisappearing ||
                reason == RepaintingWhenScrollingReasonNone) {
                m_target->asElement()
                    ->webView()
                    ->markNeedsCompositeConsiderInRendering();
                return;
            }
            slowPathReason = reason;
        }
        logSlowScrollPathIfNeeded(slowPathReason, m_target->asElement());
        m_target->asElement()->markAncestorStackingContextVisibleRectDirty();
        m_target->asElement()
            ->webView()
            ->setNeedsComputeStackingContextProperties();
        m_target->asElement()->setNeedsPainting();
    }
}

template <typename T>
void Scrolling::paintScrollbars(Scrolling* scrolling, T canvas,
                                FrameBlockBox* frame, OverflowValue ox,
                                OverflowValue oy)
{
#if defined(STARFISH_DISABLE_OVERFLOW_SCROLL)
    return;
#endif
#ifndef STARFISH_SCROLLBAR_THICKNESS
#define STARFISH_SCROLLBAR_THICKNESS 4
#endif

#ifdef STARFISH_ENABLE_TEST
    if (getenv("SCREEN_SHOT") && strlen(getenv("SCREEN_SHOT")) > 0) {
        return;
    }
    if (getenv("PIXEL_TEST") && strlen(getenv("PIXEL_TEST")) > 0) {
        return;
    }
#endif

    if (scrolling) {
        WebView* webView = scrolling->target()
                               ->executionContext()
                               ->document()
                               ->browsingContext()
                               ->webView();
        if (!webView->scrollbarVisible()) {
            return;
        }

        bool hasVerticalScroll = frame->hasBiggerContentThanFrameHeight() &&
                                 oy >= OverflowValue::AutoOverflow &&
                                 frame->height();
        bool hasHorizontalScroll = frame->hasBiggerContentThanFrameWidth() &&
                                   ox >= OverflowValue::AutoOverflow &&
                                   frame->width();

        bool needsToDrawScrollbar = false;
        float scrollbarOpacity = 0;
        if (hasVerticalScroll || hasHorizontalScroll) {
            auto currentTime = timestamp();
            if (currentTime - scrolling->m_lastActiveTime <
                STARFISH_SCROLL_ACTIVE_TIME_IN_MS) {
                needsToDrawScrollbar = true;
                scrollbarOpacity =
                    1 - float(currentTime - scrolling->m_lastActiveTime) /
                            float(STARFISH_SCROLL_ACTIVE_TIME_IN_MS);
            }
        }
        if (!needsToDrawScrollbar) {
            return;
        }
        canvas->save();

        LayoutRect rr(0, 0, 0, 0);
        if (hasVerticalScroll && needsToDrawScrollbar) {
            canvas->setFillColor(Unit::Color(64, 64, 64, 255));
            float scrollMoveRatio =
                ((float)frame->scrollTop() /
                 (frame->scrollHeight() -
                  (frame->height() - frame->borderHeight())));
            LayoutUnit scrollMovableArea =
                frame->height() - frame->borderHeight();
            LayoutUnit scrollBarHeight =
                scrollMovableArea * ((frame->height() - frame->borderHeight()) /
                                     frame->scrollHeight());
            LayoutUnit scrollBarWidth = STARFISH_SCROLLBAR_THICKNESS;
            if (hasHorizontalScroll) {
                scrollMovableArea -= scrollBarWidth;
            }

            rr.setWidth(scrollBarWidth);
            rr.setHeight(scrollBarHeight);

            if (frame->style()->direction() ==
                DirectionValue::LtrDirectionValue) {
                rr.setX(frame->width() - scrollBarWidth - frame->borderRight());
            } else {
                rr.setX(frame->borderLeft());
            }

            rr.setY(frame->borderTop() +
                    scrollMoveRatio * (scrollMovableArea - scrollBarHeight));
        }
        if (!rr.isEmpty()) {
            canvas->beginOpacityLayer(scrollbarOpacity * (192 / 255.f), rr);
            canvas->drawRect(rr);
            canvas->endOpacityLayer();
        }
        rr.setSize(LayoutSize());

        if (hasHorizontalScroll && needsToDrawScrollbar) {
            canvas->setFillColor(Unit::Color(64, 64, 64, 255));
            float scrollMoveRatio = ((float)frame->scrollLeft() /
                                     (frame->scrollWidth() -
                                      (frame->width() - frame->borderWidth())));
            LayoutUnit scrollMovableArea =
                frame->width() - frame->borderWidth();
            LayoutUnit scrollBarHeight = STARFISH_SCROLLBAR_THICKNESS;
            LayoutUnit scrollBarWidth =
                scrollMovableArea * ((frame->width() - frame->borderWidth()) /
                                     frame->scrollWidth());
            if (hasVerticalScroll) {
                scrollMovableArea -= scrollBarHeight;
            }

            rr.setWidth(scrollBarWidth);
            rr.setHeight(scrollBarHeight);

            rr.setY(frame->height() - frame->borderBottom() - scrollBarHeight);
            rr.setX(frame->borderLeft() +
                    scrollMoveRatio * (scrollMovableArea - scrollBarWidth));
        }

        if (!rr.isEmpty()) {
            canvas->beginOpacityLayer(scrollbarOpacity * (192 / 255.f), rr);
            canvas->drawRect(rr);
            canvas->endOpacityLayer();
        }
        canvas->restore();
    }
}

template void Scrolling::paintScrollbars<Canvas*>(Scrolling* scrolling, Canvas*,
                                                  FrameBlockBox*, OverflowValue,
                                                  OverflowValue);
template void Scrolling::paintScrollbars<Compositor*>(Scrolling* scrolling,
                                                      Compositor*,
                                                      FrameBlockBox*,
                                                      OverflowValue,
                                                      OverflowValue);
} // namespace Starfish
