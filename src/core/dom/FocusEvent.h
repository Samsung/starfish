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

#ifndef __StarFishFocusEvent__
#define __StarFishFocusEvent__

#include "core/dom/UIEvent.h"

namespace StarFish {

struct FocusEventInit : UIEventInit {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    FocusEventInit()
        : UIEventInit()
        , m_relatedTarget(nullptr)
    {
    }

    EventTarget* relatedTarget() const
    {
        return m_relatedTarget;
    }
    void setRelatedTarget(EventTarget* relatedTarget)
    {
        m_relatedTarget = relatedTarget;
    }

private:
    EventTarget* m_relatedTarget;
};

class FocusEvent : public UIEvent {
public:
    FocusEvent(Document* document, String* eventType)
        : UIEvent(document, eventType)
        , m_relatedTarget(nullptr)
    {
    }
    FocusEvent(Document* document, String* eventType,
               const FocusEventInit& init)
        : UIEvent(document, eventType, init)
        , m_relatedTarget(init.relatedTarget())
    {
    }

    EventTarget* relatedTarget() const
    {
        return m_relatedTarget;
    }
    void setRelatedTarget(EventTarget* relatedTarget)
    {
        m_relatedTarget = relatedTarget;
    }
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isFocusEvent() const override;

private:
    EventTarget* m_relatedTarget;
};
}

#endif
