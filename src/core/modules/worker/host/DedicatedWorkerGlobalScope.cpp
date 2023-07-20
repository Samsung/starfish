/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_WEBWORKER_HOST

#include "StarfishConfig.h"
#include "Starfish.h"
#include "binding/ScriptBindingWorkerInstance.h"

#include "core/dom/ExecutionContext.h"
#include "core/modules/worker/host/WebWorker.h"
#include "core/modules/worker/host/DedicatedWorkerGlobalScope.h"

namespace Starfish {

DedicatedWorkerGlobalScope::DedicatedWorkerGlobalScope(WebWorker* webWorker,
                                                       ResourceURL* url,
                                                       String* charSet)
    : WorkerGlobalScope(webWorker)
{
    m_scriptBindingInstance =
        new ScriptBindingWorkerInstance<DedicatedWorkerGlobalScope>(
            webWorker->scriptEngineInstance(), this);
    initGlobalScope(url, charSet);
}

void DedicatedWorkerGlobalScope::postMessage(ScriptValue message,
                                             GCVector<ScriptValue>& transfer)
{
    STARFISH_UNIMPLEMENTED();
}

void DedicatedWorkerGlobalScope::dispatchMessageEvent(ScriptValue message)
{
    STARFISH_UNIMPLEMENTED();
}

void DedicatedWorkerGlobalScope::close()
{
    STARFISH_UNIMPLEMENTED();
}

ScriptBindingInstance* DedicatedWorkerGlobalScope::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

DEFINE_EVENT_LISTENER(DedicatedWorkerGlobalScope, message);
DEFINE_EVENT_LISTENER(DedicatedWorkerGlobalScope, messageerror);

} // namespace Starfish

#endif /* STARFISH_WEBWORKER_HOST */
