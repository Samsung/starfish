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

#ifndef __StarFishCompositionEvent__
#define __StarFishCompositionEvent__

#include "core/dom/UIEvent.h"

namespace StarFish {

class Window;

struct CompositionEventData {
};

struct CompositionEventInit : UIEventInit {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    CompositionEventInit()
        : UIEventInit()
        , m_data(String::emptyString)
    {
    }

    CompositionEventInit(String* data)
        : UIEventInit()
        , m_data(data)
    {
    }

    CompositionEventInit(bool bubbles, bool cancelable)
        : UIEventInit(bubbles, cancelable)
        , m_data(String::emptyString)
    {
    }

    String* data() const
    {
        return m_data;
    }

    void setData(String* data)
    {
        m_data = data;
    }

private:
    String* m_data;
};

class CompositionEvent : public UIEvent {
public:
    CompositionEvent(Document* document)
        : UIEvent(document)
        , m_data(String::emptyString)
    {
    }

    CompositionEvent(Document* document, String* eventType)
        : UIEvent(document, eventType)
        , m_data(String::emptyString)
    {
    }

    CompositionEvent(Document* document, String* eventType,
                     const CompositionEventInit& init)
        : UIEvent(document, eventType, init)
        , m_data(init.data())
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCompositionEvent() const override;

    String* data() const
    {
        return m_data;
    }

    void setData(String* data)
    {
        m_data = data;
    }

private:
    String* m_data;
};
}

#endif
