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

#ifndef __StarFishInputEvent__
#define __StarFishInputEvent__

#include "UIEvent.h"

namespace StarFish {

struct InputEventInit : UIEventInit {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    InputEventInit(String* data = String::emptyString,
                   String* type = String::emptyString)
        : UIEventInit()
        , m_data(data)
        , m_inputType(type)
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

    String* inputType() const
    {
        return m_inputType;
    }

    void setInputType(String* type)
    {
        m_inputType = type;
    }

private:
    String* m_data;
    String* m_inputType;
};

class InputEvent : public UIEvent {
public:
    InputEvent(Document* document)
        : UIEvent(document)
        , m_data(Nullable<String*>())
        , m_inputType(String::emptyString)
    {
    }

    InputEvent(Document* document, String* eventType)
        : UIEvent(document, eventType)
        , m_data(Nullable<String*>())
        , m_inputType(String::emptyString)
    {
    }

    InputEvent(Document* document, String* eventType,
               const InputEventInit& init)
        : UIEvent(document, eventType, init)
        , m_data(init.data())
        , m_inputType(init.inputType())
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isInputEvent() const override;

    Nullable<String*> data() const
    {
        return m_data;
    }

    void setData(Nullable<String*> data)
    {
        m_data = data;
    }

    String* inputType() const
    {
        return m_inputType;
    }

    void setInputType(String* type)
    {
        m_inputType = type;
    }

private:
    Nullable<String*> m_data;
    String* m_inputType;
};
}

#endif
