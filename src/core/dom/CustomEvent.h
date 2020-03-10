/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishCustomEvent__
#define __StarfishCustomEvent__

#include "core/dom/Event.h"

namespace Starfish {

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
    CustomEvent(ExecutionContext* executionContext)
        : Event(executionContext)
        , m_detail(scriptNull())
    {
    }
    CustomEvent(ExecutionContext* executionContext, String* eventType)
        : Event(executionContext, eventType)
        , m_detail(scriptNull())
    {
    }
    CustomEvent(ExecutionContext* executionContext, String* eventType,
                const CustomEventInit& init)
        : Event(executionContext, eventType, init)
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
