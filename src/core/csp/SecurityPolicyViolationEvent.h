/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishSecurityPolicyViolationEvent__
#define __StarfishSecurityPolicyViolationEvent__

#include "core/dom/Event.h"

namespace Starfish {

struct SecurityPolicyViolationEventInit : EventInit {
public:
    STARFISH_MAKE_STACK_ALLOCATED();

    SecurityPolicyViolationEventInit()
        : EventInit()
        , m_violatedDirective(String::emptyString)
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

protected:
    String* m_violatedDirective;
};

class SecurityPolicyViolationEvent : public Event {
public:
    SecurityPolicyViolationEvent(Document* document)
        : Event(document)
    {
    }
    SecurityPolicyViolationEvent(Document* document, String* eventType)
        : Event(document, eventType)
        , m_violatedDirective(String::emptyString)
    {
    }
    SecurityPolicyViolationEvent(Document* document, String* eventType,
                                 const SecurityPolicyViolationEventInit& init)
        : Event(document, eventType, init)
        , m_violatedDirective(init.violatedDirective())
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSecurityPolicyViolationEvent() const override;

    void initSecurityPolicyViolationEvent(String* type, bool bubbles,
                                          bool cancelable, ScriptValue detail)
    {
        initEvent(type, bubbles, cancelable);
    }

    String* violatedDirective() const
    {
        return m_violatedDirective;
    }

    void setViolatedDirective(String* violatedDirective)
    {
        m_violatedDirective = violatedDirective;
    }

protected:
    String* m_violatedDirective;
};
}

#endif
