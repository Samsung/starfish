/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishPromiseRejectionEvent__
#define __StarfishPromiseRejectionEvent__

#include "core/dom/DOMException.h"
#include "core/dom/Event.h"

namespace Starfish {

struct PromiseRejectionEventInit : EventInit {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    DEFINE_GETTER_SETTER(ScriptObject, promise, Promise);
    DEFINE_GETTER_SETTER(ScriptValue, reason, Reason);

private:
    ScriptObject m_promise{ nullptr };
    ScriptValue m_reason{ scriptUndefined() };
};

class PromiseRejectionEvent : public Event {
public:
    PromiseRejectionEvent(ExecutionContext* context, String* type,
                          const PromiseRejectionEventInit& init)
        : Event(context, type, init)
        , m_promise(init.promise())
        , m_reason(init.reason())
    {
        // The bindings don't enforce required dictionary members.
        if (!m_promise) {
            throw new DOMException(context, DOMException::SCRIPT_TYPE_ERR,
                                   "Required member promise is undefined");
        }
    }

    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    virtual bool isPromiseRejectionEvent() const;

    ScriptObject promise() const
    {
        return m_promise;
    }

    ScriptValue reason() const
    {
        return m_reason;
    }

private:
    ScriptObject m_promise;
    ScriptValue m_reason;
};

} // namespace Starfish

#endif
