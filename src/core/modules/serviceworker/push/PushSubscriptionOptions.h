/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SERVICE_WORKER)
#ifndef __StarfishPushSubscriptionOptions__
#define __StarfishPushSubscriptionOptions__

#include "binding/BufferSourceOrDOMStringUnion.h"
#include "binding/ScriptWrappable.h"

namespace Starfish {

struct PushSubscriptionOptionsInit {
public:
    PushSubscriptionOptionsInit()
        : m_userVisibleOnly(false)
        , m_applicationServerKey(nullptr)
    {
    }

    DEFINE_GETTER_SETTER(bool, userVisibleOnly, UserVisibleOnly);
    DEFINE_GETTER_SETTER(Nullable<BufferSourceOrDOMString>,
                         applicationServerKey, ApplicationServerKey);

private:
    bool m_userVisibleOnly;
    Nullable<BufferSourceOrDOMString> m_applicationServerKey;
};

class PushSubscriptionOptions : public ScriptWrappable {
public:
    PushSubscriptionOptions(ExecutionContext* executionContext);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(PushSubscriptionOptions)

    void setPushSubscriptionOptions(PushSubscriptionOptionsInit& optionsInit);

    DEFINE_GETTER_SETTER(bool, userVisibleOnly, UserVisibleOnly);

    // Only use in JavaScript binding
    ScriptArrayBuffer applicationServerKeyScriptValue();

    String* applicationServerKey()
    {
        return m_applicationServerKey;
    }

private:
    ExecutionContext* m_executionContext;
    bool m_userVisibleOnly;
    String* m_applicationServerKey;
};
} // namespace Starfish

#endif
#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
