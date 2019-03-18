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
#include "StarfishConfig.h"
#include "core/dom/EventTarget.h"
#include "core/dom/Scrolling.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/TouchEvent.h"
#include "core/dom/TouchList.h"
#include "core/layout/FrameBlockBox.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/message_loop/Timer.h"

#define STARFISH_SCROLL_START_THRESHOLD 10
#define STARFISH_SCROLL_START_FLING_THRESHOLD 300
#define STARFISH_SCROLL_FLING_LENGTH_MULTIPLY_BASE \
    (STARFISH_SCROLL_START_FLING_THRESHOLD * 5)
#define STARFISH_SCROLL_FLING_BASE_TIME_IN_MS 1000

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
        if (event->isMouseEvent()) {
            if (event->type()->equals("mousedown")) {
                isPointingDownEvent = true;
                shouldProcess = true;
                x = event->asMouseEvent()->screenX();
                y = event->asMouseEvent()->screenY();
            } else if (event->type()->equals("mousemove")) {
                shouldProcess = true;
                x = event->asMouseEvent()->screenX();
                y = event->asMouseEvent()->screenY();
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
            } else if (event->type()->equals("touchmove")) {
                shouldProcess = true;
                x = event->asTouchEvent()->touches()->at(0)->screenX();
                y = event->asTouchEvent()->touches()->at(0)->screenY();
            } else if (event->type()->equals("touchend")) {
                shouldProcess = true;
                isPointingUpEvent = true;
            }
        }
        if (shouldProcess) {
            if (isPointingDownEvent) {
                m_pointingEventX = x;
                m_pointingEventY = y;
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

                if (m_inVerticalScrolling || m_inHorizontalScrolling) {
                    window->browsingContext()
                        ->webView()
                        ->addGlobalPointingEventInterceptListener(m_target);
                    m_isScrollTarget = true;
                    m_pointingEventX = m_lastPointingEventX = x;
                    m_pointingEventY = m_lastPointingEventY = y;
                    m_target->webView()->timer()->requestAnimationFrame(
                        m_target->window(), onAnimationFrameHandler, this);
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
        return;
    }

    // do fling
    if (self->m_inHorizontalFling || self->m_inVerticalFling) {
        auto currentTime = longTickCount();
        auto flingLength = STARFISH_SCROLL_FLING_BASE_TIME_IN_MS;

        if (STARFISH_SCROLL_FLING_LENGTH_MULTIPLY_BASE <
            std::abs(self->m_flingStartSpeed)) {
            flingLength *= std::abs(self->m_flingStartSpeed /
                                    STARFISH_SCROLL_FLING_LENGTH_MULTIPLY_BASE);
        }

        bool shouldExitFling =
            currentTime - self->m_flingStartTime > uint64_t(flingLength * 1000);

        float progress =
            shouldExitFling
                ? 1
                : ((currentTime - self->m_flingStartTime) / 1000.f) /
                      flingLength;
        progress = flingInterpolationFunction(progress);
        if (progress >= 0.95) {
            shouldExitFling = true;
        }

        if (shouldExitFling) {
            // end
            self->stopFling();
            self->stopScrolling();
            return;
        }
        float speed = self->m_flingStartSpeed * (1 - progress);
        float distance = speed * ((currentTime - self->m_flingProcessingTime) /
                                  (1000.f * 1000.f));

        bool isScrollEffective = false;
        if (self->m_target->isElement()) {
            if (self->m_inVerticalScrolling) {
                isScrollEffective = self->m_target->asElement()->setScrollTop(
                    self->m_target->asElement()->scrollTop() + distance);
            } else if (self->m_inHorizontalScrolling) {
                isScrollEffective = self->m_target->asElement()->setScrollLeft(
                    self->m_target->asElement()->scrollLeft() + distance);
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
        }
    } else {
        float dx = self->m_lastPointingEventX - self->m_pointingEventX;
        float dy = self->m_lastPointingEventY - self->m_pointingEventY;

        auto currentTime = longTickCount();
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
            if (currentTime - self->m_lastScrollingData.front().first <
                1000 * 100) {
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
    }

    self->m_target->webView()->timer()->requestAnimationFrame(
        self->m_target->window(), onAnimationFrameHandler, self);
}

void Scrolling::onGlobalPointingEvent(float x, float y,
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
            float speed =
                m_lastScrollingData[i].second / (td / (1000.f * 1000.f));
            if (speed > 0) {
                postiveAverage += speed;
            } else {
                negativeAverage += speed;
            }
            t = m_lastScrollingData[i].first;
        }

        if (m_lastScrollingData.size() > 1) {
            postiveAverage /= (float)(m_lastScrollingData.size() - 1);
            negativeAverage /= (float)(m_lastScrollingData.size() - 1);
        }

        if (postiveAverage >= STARFISH_SCROLL_START_FLING_THRESHOLD ||
            -negativeAverage >= STARFISH_SCROLL_START_FLING_THRESHOLD) {
            userWantsFling = true;
        }

        if (userWantsFling) {
            m_inHorizontalFling = m_inHorizontalScrolling;
            m_inVerticalFling = m_inVerticalScrolling;
            m_flingProcessingTime = m_flingStartTime = longTickCount();
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
    } else if (kind == EventTarget::GlobalPointingEventKindDown) {
        if (m_inHorizontalFling || m_inVerticalFling) {
            m_pointingEventX = m_lastPointingEventX = x;
            m_pointingEventY = m_lastPointingEventY = y;
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
    m_target->document()
        ->browsingContext()
        ->webView()
        ->removeGlobalPointingEventInterceptListener(m_target);
}

void Scrolling::stopFling()
{
    m_inHorizontalFling = m_inVerticalFling = false;
}

template <typename T>
void Scrolling::paintScrollbars(T canvas, FrameBlockBox* frame,
                                OverflowValue ox, OverflowValue oy)
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
    canvas->save();
    bool hasVerticalScroll = frame->hasBiggerContentThanFrameHeight() &&
                             oy >= OverflowValue::AutoOverflow &&
                             frame->height();
    bool hasHorizontalScroll = frame->hasBiggerContentThanFrameWidth() &&
                               ox >= OverflowValue::AutoOverflow &&
                               frame->width();

    if (hasVerticalScroll) {
        canvas->setColor(Unit::Color(64, 64, 64, 192));
        float scrollMoveRatio = ((float)frame->scrollTop() /
                                 (frame->scrollHeight() -
                                  (frame->height() - frame->borderHeight())));
        LayoutUnit scrollMovableArea = frame->height() - frame->borderHeight();
        LayoutUnit scrollBarHeight =
            scrollMovableArea *
            ((frame->height() - frame->borderHeight()) / frame->scrollHeight());
        LayoutUnit scrollBarWidth = STARFISH_SCROLLBAR_THICKNESS;
        ;
        if (hasHorizontalScroll) {
            scrollMovableArea -= scrollBarWidth;
        }
        LayoutRect rr(0, 0, 0, 0);

        rr.setWidth(scrollBarWidth);
        rr.setHeight(scrollBarHeight);

        if (frame->style()->direction() == DirectionValue::LtrDirectionValue) {
            rr.setX(frame->width() - scrollBarWidth - frame->borderRight());
        } else {
            rr.setX(frame->borderLeft());
        }

        rr.setY(frame->borderTop() +
                scrollMoveRatio * (scrollMovableArea - scrollBarHeight));

        canvas->drawRect(rr);
    }
    if (hasHorizontalScroll) {
        canvas->setColor(Unit::Color(64, 64, 64, 192));
        float scrollMoveRatio =
            ((float)frame->scrollLeft() /
             (frame->scrollWidth() - (frame->width() - frame->borderWidth())));
        LayoutUnit scrollMovableArea = frame->width() - frame->borderWidth();
        LayoutUnit scrollBarHeight = STARFISH_SCROLLBAR_THICKNESS;
        LayoutUnit scrollBarWidth =
            scrollMovableArea *
            ((frame->width() - frame->borderWidth()) / frame->scrollWidth());
        if (hasVerticalScroll) {
            scrollMovableArea -= scrollBarHeight;
        }

        LayoutRect rr(0, 0, 0, 0);

        rr.setWidth(scrollBarWidth);
        rr.setHeight(scrollBarHeight);

        rr.setY(frame->height() - frame->borderBottom() - scrollBarHeight);
        rr.setX(frame->borderLeft() +
                scrollMoveRatio * (scrollMovableArea - scrollBarWidth));

        canvas->drawRect(rr);
    }
    canvas->restore();
}

template void Scrolling::paintScrollbars<Canvas*>(Canvas*, FrameBlockBox*,
                                                  OverflowValue, OverflowValue);
template void Scrolling::paintScrollbars<Compositor*>(Compositor*,
                                                      FrameBlockBox*,
                                                      OverflowValue,
                                                      OverflowValue);
}
