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

#ifndef __StarFishCustomEvent__
#define __StarFishCustomEvent__

#include "core/dom/Event.h"

namespace StarFish {

struct CustomEventInit : EventInit {
public:
    STARFISH_MAKE_STACK_ALLOCATED();

    CustomEventInit()
        : EventInit()
        , m_detail(nullptr)
    {
    }
    ScriptValue detail() const
    {
        return m_detail;
    }
    void setDetail(ScriptValue detail)
    {
        m_detail = detail;
    }

protected:
    ScriptValue m_detail;
};

class CustomEvent : public Event {
public:
    CustomEvent(Document* document)
        : Event(document)
    {
    }
    CustomEvent(Document* document, String* eventType)
        : Event(document, eventType)
    {
    }
    CustomEvent(Document* document, String* eventType,
                const CustomEventInit& init)
        : Event(document, eventType, init)
        , m_detail(init.detail())
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCustomEvent() const override;

    ScriptValue detail() const
    {
        return m_detail;
    }
    void setDetail(ScriptValue detail)
    {
        m_detail = detail;
    }
    void initCustomEvent(String* type, bool bubbles, bool cancelable,
                         ScriptValue detail)
    {
        initEvent(type, bubbles, cancelable);
        m_detail = detail;
    }

protected:
    ScriptValue m_detail;
};
}

#endif
