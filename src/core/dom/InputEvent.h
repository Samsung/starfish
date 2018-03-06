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
