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
#include "StarFishConfig.h"
#include "core/dom/Scrolling.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/TouchEvent.h"
#include "core/dom/TouchList.h"
#include "core/layout/FrameBlockBox.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/modules/canvas/Canvas.h"

namespace StarFish {

bool Scrolling::handleDefaultEvent(Event* event, Window* window,
                                   FrameBlockBox* frame, OverflowValue ox,
                                   OverflowValue oy)
{
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
                t /= window->devicePixelRatio();

                if (std::abs(m_pointingEventY - y) > t &&
                    verticalScrollEnabled) {
                    m_inVerticalScrolling = true;
                } else if (std::abs(m_pointingEventX - x) > t &&
                           horizontalScrollEnabled) {
                    m_inHorizontalScrolling = true;
                }

                if (m_inVerticalScrolling || m_inHorizontalScrolling) {
                    window->browsingContext()
                        ->addGlobalPointingEventInterceptListener(m_target);
                    return true;
                }
            }
        }
    }
    return false;
}

void Scrolling::onGlobalPointingEvent(float x, float y,
                                      EventTarget::GlobalPointingEventKind kind)
{
    if (kind == EventTarget::GlobalPointingEventKindUp) {
        m_target->document()
            ->browsingContext()
            ->removeGlobalPointingEventInterceptListener(m_target);
        m_isScrollTarget = false;
        m_inHorizontalScrolling = false;
        m_inVerticalScrolling = false;
    } else if (kind == EventTarget::GlobalPointingEventKindMove) {
        float dx = m_pointingEventX - x;
        float dy = m_pointingEventY - y;
        m_pointingEventX = x;
        m_pointingEventY = y;
        if (m_target->isElement()) {
            if (m_inVerticalScrolling) {
                m_target->asElement()->setScrollTop(
                    m_target->asElement()->scrollTop() + dy);
            } else if (m_inHorizontalScrolling) {
                m_target->asElement()->setScrollLeft(
                    m_target->asElement()->scrollLeft() + dx);
            }
        } else {
            if (m_inVerticalScrolling) {
                m_target->asWindow()->scrollTo(m_target->asWindow()->scrollX(),
                                               m_target->asWindow()->scrollY() +
                                                   dy);
            } else if (m_inHorizontalScrolling) {
                m_target->asWindow()->scrollTo(m_target->asWindow()->scrollX() +
                                                   dx,
                                               m_target->asWindow()->scrollY());
            }
        }
    }
}

void Scrolling::paintScrollbars(Canvas* canvas, FrameBlockBox* frame,
                                OverflowValue ox, OverflowValue oy)
{
#ifndef STARFISH_SCROLLBAR_THICKNESS
#define STARFISH_SCROLLBAR_THICKNESS 4
#endif

#ifdef STARFISH_ENABLE_TEST
    if (getenv("SCREEN_SHOT") && strlen(getenv("SCREEN_SHOT")) > 0) {
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
        LayoutUnit scrollBarWidth = STARFISH_SCROLLBAR_THICKNESS /
                                    frame->node()->window()->devicePixelRatio();
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
        LayoutUnit scrollBarHeight =
            STARFISH_SCROLLBAR_THICKNESS /
            frame->node()->window()->devicePixelRatio();
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
}
