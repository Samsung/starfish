/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishSecurityPolicyViolationEvent__
#define __StarfishSecurityPolicyViolationEvent__

#include "core/dom/Event.h"

namespace Starfish {

struct SecurityPolicyViolationEventInit : EventInit {
public:
    STARFISH_MAKE_STACK_ALLOCATED();

    SecurityPolicyViolationEventInit()
        : EventInit(true, false)
        , m_violatedDirective(String::emptyString)
        , m_blockedURI(String::emptyString)
    {
    }

    String* violatedDirective() const
    {
        return m_violatedDirective;
    }
    void setViolatedDirective(String* violatedDirective)
    {
        m_violatedDirective = violatedDirective;
    }

    String* blockedURI() const
    {
        return m_blockedURI;
    }
    void setBlockedURI(String* blockedURI)
    {
        m_blockedURI = blockedURI;
    }

protected:
    String* m_violatedDirective;
    String* m_blockedURI;
};

class SecurityPolicyViolationEvent : public Event {
public:
    SecurityPolicyViolationEvent(ExecutionContext* executionContext)
        : Event(executionContext)
    {
        initSecurityPolicyViolationEvent();
    }
    SecurityPolicyViolationEvent(ExecutionContext* executionContext,
                                 String* eventType)
        : Event(executionContext, eventType)
        , m_violatedDirective(String::emptyString)
        , m_blockedURI(String::emptyString)
    {
        initSecurityPolicyViolationEvent();
    }
    SecurityPolicyViolationEvent(ExecutionContext* executionContext,
                                 String* eventType,
                                 const SecurityPolicyViolationEventInit& init)
        : Event(executionContext, eventType, init)
        , m_violatedDirective(init.violatedDirective())
        , m_blockedURI(init.blockedURI())
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSecurityPolicyViolationEvent() const override;

    void initSecurityPolicyViolationEvent(bool bubbles = true,
                                          bool cancelable = false)
    {
        setBubbles(bubbles);
        setCancelable(cancelable);
    }

    String* violatedDirective() const
    {
        return m_violatedDirective;
    }
    void setViolatedDirective(String* violatedDirective)
    {
        m_violatedDirective = violatedDirective;
    }

    String* blockedURI() const
    {
        return m_blockedURI;
    }
    void setBlockedURI(String* blockedURI)
    {
        m_blockedURI = blockedURI;
    }

protected:
    String* m_violatedDirective;
    String* m_blockedURI;
};
}

#endif
