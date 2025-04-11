/*
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishHashChangeEvent__
#define __StarfishHashChangeEvent__

#include "core/dom/Event.h"

namespace Starfish {

struct HashChangeEventInit : EventInit {
    HashChangeEventInit();

    DEFINE_GETTER_SETTER(String*, oldURL, OldURL);
    DEFINE_GETTER_SETTER(String*, newURL, NewURL);

private:
    String* m_oldURL;
    String* m_newURL;
};

class HashChangeEvent : public Event {
public:
    HashChangeEvent(ExecutionContext* executionContext);
    HashChangeEvent(ExecutionContext* executionContext, String* type);
    HashChangeEvent(ExecutionContext* executionContext, String* type,
                    const HashChangeEventInit& hashChnageEventInit);

    // Define for generated code
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHashChangeEvent() const override;

    String* oldURL() const
    {
        return m_oldURL;
    }

    String* newURL() const
    {
        return m_newURL;
    }

private:
    String* m_oldURL;
    String* m_newURL;
};

} // namespace Starfish

#endif
