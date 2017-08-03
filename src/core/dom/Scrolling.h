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

#ifndef __StarFishScrolling__
#define __StarFishScrolling__

#include "core/dom/EventTarget.h"
#include "core/style/Style.h"

namespace StarFish {

class Event;
class FrameBlockBox;

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
