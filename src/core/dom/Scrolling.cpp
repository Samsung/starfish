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

namespace Starfish {

void* Scrolling::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(Scrolling));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(Scrolling)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(Scrolling, m_target));
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

    if (m_isScrollTarget ||
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
                m_isScrollTarget = true;
                m_pointingEventX = x;
                m_pointingEventY = y;
                return true;
            } else if (isPointingUpEvent) {
                m_isScrollTarget = false;
            } else if (m_isScrollTarget) {
#define STARFISH_SCROLL_THRESHOLD 10
                unsigned t = STARFISH_SCROLL_THRESHOLD;

                bool inScrolling =
                    m_inVerticalScrolling || m_inHorizontalScrolling;
                if (!inScrolling) {
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
                        m_lastPointingEventX = x;
                        m_lastPointingEventY = y;
                        m_target->webView()->timer()->requestAnimationFrame(
                            m_target->window(), onAnimationFrameHandler, this);
                    }
                }
                return true;
            }
        }
    }
    return false;
}

void Scrolling::onAnimationFrameHandler(Window* window, void* data)
{
    Scrolling* self = (Scrolling*)data;

    if (!self->m_isScrollTarget) {
        return;
    }

    float dx = self->m_lastPointingEventX - self->m_pointingEventX;
    float dy = self->m_lastPointingEventY - self->m_pointingEventY;

    if (self->m_inVerticalScrolling) {
        if (dy > 0) {
            self->m_inVerticalScrollingDown = true;
            self->m_inVerticalScrollingUp = false;
        } else {
            self->m_inVerticalScrollingDown = false;
            self->m_inVerticalScrollingUp = true;
        }
    }
    if (self->m_inHorizontalScrolling) {
        if (dy > 0) {
            self->m_inHorizontalScrollingLeft = true;
            self->m_inHorizontalScrollingRight = false;
        } else {
            self->m_inHorizontalScrollingLeft = false;
            self->m_inHorizontalScrollingRight = true;
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
        m_target->document()
            ->browsingContext()
            ->webView()
            ->removeGlobalPointingEventInterceptListener(m_target);
        m_isScrollTarget = false;
        m_inHorizontalScrolling = false;
        m_inVerticalScrolling = false;
    } else if (kind == EventTarget::GlobalPointingEventKindMove) {
        m_pointingEventX = x;
        m_pointingEventY = y;
    }
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
