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

#ifndef __StarFishFocusEvent__
#define __StarFishFocusEvent__

#include "dom/UIEvent.h"

namespace StarFish {

struct FocusEventInit : UIEventInit {
public:
    FocusEventInit();

    // Constructor for internal use
    FocusEventInit(bool bubbles);
    FocusEventInit(bool bubbles, bool cancelable);

    EventTarget* relatedTarget() const;
    void setRelatedTarget(EventTarget* relatedTarget);

private:
    EventTarget* m_relatedTarget;
};

class FocusEvent : public UIEvent {
public:
    FocusEvent(String* eventType, const FocusEventInit& init = FocusEventInit())
        : UIEvent(eventType, init)
    {
    }

    virtual void init(ScriptBindingInstance* instance) override;
    virtual bool isFocusEvent() const override;

    EventTarget* relatedTarget()
    {
        STARFISH_ASSERT_NOT_REACHED();
    }
};
}

#endif
