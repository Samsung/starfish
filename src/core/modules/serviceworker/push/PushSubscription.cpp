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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include "StarfishConfig.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/serviceworker/push/PushSubscriptionOptions.h"
#include "core/modules/serviceworker/push/PushSubscription.h"

namespace Starfish {

PushSubscription::PushSubscription(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_endpoint(String::emptyString)
    , m_options(new PushSubscriptionOptions(executionContext))
{
    STARFISH_ASSERT(executionContext != nullptr);
}

PushSubscription::PushSubscription(ExecutionContext* executionContext,
                                   String* endpoint,
                                   PushSubscriptionOptions* options)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_endpoint(endpoint)
    , m_options(options)
{
    STARFISH_ASSERT(executionContext != nullptr);
    STARFISH_ASSERT(endpoint != nullptr);
    STARFISH_ASSERT(options != nullptr);
}

ScriptBindingInstance* PushSubscription::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}
}

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
