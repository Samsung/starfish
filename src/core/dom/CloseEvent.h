/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishCloseEvent__
#define __StarfishCloseEvent__

#include "Event.h"

namespace Starfish {

class Window;
struct CloseEventInit : EventInit {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    CloseEventInit()
        : EventInit()
        , m_wasClean(false)
        , m_code(0)
        , m_reason(String::emptyString)
    {
    }

    CloseEventInit(bool bubbles, bool cancelable)
        : EventInit(bubbles, cancelable)
        , m_wasClean(false)
        , m_code(0)
        , m_reason(String::emptyString)
    {
    }

    bool wasClean() const
    {
        return m_wasClean;
    }

    void setWasClean(bool wasClean)
    {
        m_wasClean = wasClean;
    }

    int32_t code() const
    {
        return m_code;
    }

    void setCode(int32_t code)
    {
        m_code = code;
    }

    String* reason() const
    {
        return m_reason;
    }

    void setReason(String* reason)
    {
        m_reason = reason;
    }

private:
    bool m_wasClean;
    int32_t m_code;
    String* m_reason;
};

class CloseEvent : public Event {
public:
    CloseEvent(ExecutionContext* executionContext)
        : Event(executionContext)
        , m_wasClean(false)
        , m_code(0)
        , m_reason(String::emptyString)
    {
    }

    CloseEvent(ExecutionContext* executionContext, String* eventType)
        : Event(executionContext, eventType)
        , m_wasClean(false)
        , m_code(0)
        , m_reason(String::emptyString)
    {
    }

    CloseEvent(ExecutionContext* executionContext, String* eventType,
               const CloseEventInit& init)
        : Event(executionContext, eventType, init)
        , m_wasClean(init.wasClean())
        , m_code(init.code())
        , m_reason(init.reason())
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCloseEvent() const override;

    bool wasClean() const
    {
        return m_wasClean;
    }

    void setWasClean(bool wasClean)
    {
        m_wasClean = wasClean;
    }

    int32_t code() const
    {
        return m_code;
    }

    void setCode(int32_t code)
    {
        m_code = code;
    }

    String* reason() const
    {
        return m_reason;
    }

    void setReason(String* reason)
    {
        m_reason = reason;
    }

private:
    bool m_wasClean;
    int32_t m_code;
    String* m_reason;
};
}

#endif
