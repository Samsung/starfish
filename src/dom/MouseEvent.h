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

#ifndef __StarFishMouseEvent__
#define __StarFishMouseEvent__

#include "UIEvent.h"

namespace StarFish {
struct MouseEventInit : EventModifierInit {
public:
    MouseEventInit();

    // Constructor for internal use
    MouseEventInit(bool bubbles);
    MouseEventInit(bool bubbles, bool cancelable);

    int32_t screenX() const;
    void setScreenX(int32_t screenX);

    int32_t screenY() const;
    void setScreenY(int32_t screenY);

    int32_t clientX() const;
    void setClientX(int32_t clientX);

    int32_t clientY() const;
    void setClientY(int32_t clientY);

    short button() const;
    void setButton(short button);

    unsigned short buttons() const;
    void setButtons(unsigned short buttons);

    EventTarget* relatedTarget() const;
    void setRelatedTarget(EventTarget* relatedTarget);

private:
    int32_t m_screenX;
    int32_t m_screenY;
    int32_t m_clientX;
    int32_t m_clientY;
    short m_button;
    unsigned short m_buttons;
    EventTarget* m_relatedTarget;
};

class MouseEvent : public UIEvent {
public:
    MouseEvent(String* eventType, const MouseEventInit& init = MouseEventInit())
        : UIEvent(eventType, init)
    {
    }

    virtual void init(ScriptBindingInstance* instance) override;
    virtual bool isMouseEvent() const override;
};
}

#endif
