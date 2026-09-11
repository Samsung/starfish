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

#ifndef __StarfishToggleEvent__
#define __StarfishToggleEvent__

#include "core/dom/Event.h"

namespace Starfish {
struct ToggleEventInit : EventInit {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    DEFINE_GETTER_SETTER(String*, oldState, OldState);
    DEFINE_GETTER_SETTER(String*, newState, NewState);
    DEFINE_GETTER_SETTER(Optional<Element*>, source, Source);

private:
    String* m_oldState{ String::emptyString };
    String* m_newState{ String::emptyString };
    Optional<Element*> m_source;
};

class ToggleEvent : public Event {
public:
    ToggleEvent(ExecutionContext* context, String* type,
                const ToggleEventInit& init = ToggleEventInit())
        : Event(context, type, init)
        , m_oldState(init.oldState())
        , m_newState(init.newState())
        , m_source(init.source())
    {
    }
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isToggleEvent() const override;
    String* oldState()
    {
        return m_oldState;
    }
    String* newState()
    {
        return m_newState;
    }
    Optional<Element*> source();

private:
    String* m_oldState;
    String* m_newState;
    Optional<Element*> m_source;
};
} // namespace Starfish
#endif
