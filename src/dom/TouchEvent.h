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

#ifndef __StarFishTouchEvent__
#define __StarFishTouchEvent__

#include "binding/ScriptWrappable.h"
#include "UIEvent.h"

namespace StarFish {

class TouchEvent : public UIEvent {
public:
    TouchEvent(String* eventType, const UIEventInit& init = UIEventInit())
        : UIEvent(eventType, init)
    {
    }

    virtual void init(ScriptBindingInstance* instance) override
    {
        scriptObject()->set__proto__(
            fetchData(instance)->fnTouchEvent()->protoType());
    }

    virtual bool isTouchEvent() const override
    {
        return true;
    }
};
}

#endif
