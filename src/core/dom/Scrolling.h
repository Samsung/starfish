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

#ifndef __StarFishScrolling__
#define __StarFishScrolling__

#include "core/dom/EventTarget.h"
#include "core/style/Style.h"

namespace StarFish {

class Event;
class FrameBlockBox;
class Canvas;

class Scrolling : public gc {
public:
    Scrolling(EventTarget* target)
        : m_isScrollTarget(false)
        , m_inVerticalScrolling(false)
        , m_inHorizontalScrolling(false)
        , m_pointingEventX(0)
        , m_pointingEventY(0)
        , m_target(target)
    {
    }

    bool handleDefaultEvent(Event* event, Window* window, FrameBlockBox* frame,
                            OverflowValue ox, OverflowValue oy);
    void onGlobalPointingEvent(float x, float y,
                               EventTarget::GlobalPointingEventKind kind);
    void paintScrollbars(Canvas* canvas, FrameBlockBox* frame, OverflowValue ox,
                         OverflowValue oy);

protected:
    bool m_isScrollTarget;
    bool m_inVerticalScrolling;
    bool m_inHorizontalScrolling;
    float m_pointingEventX;
    float m_pointingEventY;

    EventTarget* m_target;
};
}

#endif
