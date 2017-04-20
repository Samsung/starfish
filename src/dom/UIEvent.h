/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#ifndef __StarFishUIEvent__
#define __StarFishUIEvent__

#include "Event.h"

namespace StarFish {

class UIEvent : public Event {
protected:
    UIEvent(String* eventType, const EventInit& init = EventInit(false, false))
        : Event(eventType, init)
    {
    }

public:
    virtual bool isUIEvent() const override
    {
        return true;
    }
};
}

#endif
