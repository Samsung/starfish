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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include "StarfishConfig.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/serviceworker/push/PushSubscriptionOptions.h"

namespace Starfish {

PushSubscriptionOptions::PushSubscriptionOptions(
    ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_userVisibleOnly(false)
    , m_applicationServerKey(String::emptyString)
{
    STARFISH_ASSERT(executionContext != nullptr);
}

ScriptBindingInstance* PushSubscriptionOptions::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

void PushSubscriptionOptions::setPushSubscriptionOptions(
    PushSubscriptionOptionsInit& optionsInit)
{
    m_userVisibleOnly = optionsInit.userVisibleOnly();

    if (optionsInit.applicationServerKey().hasValue()) {
        auto applicationServerKey =
            optionsInit.applicationServerKey().getValue();
        if (applicationServerKey.isDOMStringValue()) {
            m_applicationServerKey = applicationServerKey.getDOMStringValue();
        } else {
            // TODO: Support other types
            STARFISH_LOG_WARN(
                "Push Service: Unsupported type of application server key\n");
        }
    }
}

ScriptArrayBuffer PushSubscriptionOptions::applicationServerKeyScriptValue()
{
    auto str = m_applicationServerKey->toUTF8NonGCString();
    void* buffer = calloc(1, str.length());
    memcpy(buffer, str.data(), str.length());
    return createScriptArrayBuffer(scriptBindingInstance(), buffer,
                                   m_applicationServerKey->length());
}
}

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
