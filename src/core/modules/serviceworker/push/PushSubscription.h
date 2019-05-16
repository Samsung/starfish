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

#if defined(STARFISH_ENABLE_SERVICE_WORKER)
#ifndef __StarfishPushSubscription__
#define __StarfishPushSubscription__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class PushSubscription : public ScriptWrappable {
public:
    PushSubscription(ExecutionContext* executionContext);
    PushSubscription(ExecutionContext* executionContext, String* endpoint,
                     PushSubscriptionOptions* pushSubscriptionOptions);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(PushSubscription)

    DEFINE_GETTER_SETTER(String*, endpoint, Endpoint);
    DEFINE_GETTER_SETTER(PushSubscriptionOptions*, options, Options);

private:
    ExecutionContext* m_executionContext;
    String* m_endpoint;
    PushSubscriptionOptions* m_options;
};
}

#endif
#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
