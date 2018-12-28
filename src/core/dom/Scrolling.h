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

#ifndef __StarfishScrolling__
#define __StarfishScrolling__

#include "core/dom/EventTarget.h"
#include "core/style/Style.h"

namespace Starfish {

class Event;
class FrameBlockBox;
class Canvas;
class Compositor;

class Scrolling : public gc {
public:
    Scrolling(EventTarget* target)
        : m_isScrollTarget(false)
        , m_inVerticalScrolling(false)
        , m_inVerticalScrollingUp(false)
        , m_inVerticalScrollingDown(false)
        , m_inHorizontalScrolling(false)
        , m_inHorizontalScrollingLeft(false)
        , m_inHorizontalScrollingRight(false)
        , m_pointingEventX(0)
        , m_pointingEventY(0)
        , m_lastPointingEventX(0)
        , m_lastPointingEventY(0)
        , m_target(target)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    bool handleDefaultEvent(Event* event, Window* window, FrameBlockBox* frame,
                            OverflowValue ox, OverflowValue oy);
    void onGlobalPointingEvent(float x, float y,
                               EventTarget::GlobalPointingEventKind kind);

    template <typename T>
    void paintScrollbars(T canvas, FrameBlockBox* frame, OverflowValue ox,
                         OverflowValue oy);

    static void onAnimationFrameHandler(Window* window, void* data);

    bool inVerticalScrollingUp()
    {
        return m_inVerticalScrollingUp;
    }

    bool inVerticalScrollingDown()
    {
        return m_inVerticalScrollingDown;
    }

    bool inHorizontalScrollingLeft()
    {
        return m_inHorizontalScrollingLeft;
    }

    bool inHorizontalScrollingRight()
    {
        return m_inHorizontalScrollingRight;
    }

protected:
    bool m_isScrollTarget : 1;
    bool m_inVerticalScrolling : 1;
    bool m_inVerticalScrollingUp : 1;
    bool m_inVerticalScrollingDown : 1;
    bool m_inHorizontalScrolling : 1;
    bool m_inHorizontalScrollingLeft : 1;
    bool m_inHorizontalScrollingRight : 1;
    float m_pointingEventX;
    float m_pointingEventY;
    float m_lastPointingEventX;
    float m_lastPointingEventY;

    EventTarget* m_target;
};
}

#endif
