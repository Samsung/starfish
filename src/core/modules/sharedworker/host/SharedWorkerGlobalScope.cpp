/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SHARED_WORKER) && defined(STARFISH_WEBWORKER_HOST)

#include "StarfishConfig.h"
#include "binding/ScriptBindingWorkerInstance.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/worker/host/WebWorker.h"
#include "core/modules/sharedworker/host/SharedWorkerGlobalScope.h"

namespace Starfish {

SharedWorkerGlobalScope::SharedWorkerGlobalScope(WebWorker* webWorker,
                                                 ResourceURL* url,
                                                 String* charSet)
    : WorkerGlobalScope(webWorker)
    , m_name(String::emptyString)
{
    m_scriptBindingInstance =
        new ScriptBindingWorkerInstance<SharedWorkerGlobalScope>(
            webWorker->scriptEngineInstance(), this);

    initGlobalScope(url, charSet);
}

void SharedWorkerGlobalScope::initialize(const std::string& name)
{
    STARFISH_ASSERT(m_executionContext->isContextThread());

    m_name = String::fromUTF8(name.data(), name.size());

    if (loadMainScript()) {
    } else {
        // TODO: terminate worker
    }
}

void SharedWorkerGlobalScope::dispose()
{
    STARFISH_ASSERT(m_executionContext->isContextThread());

    WorkerGlobalScope::dispose();
}

ScriptBindingInstance* SharedWorkerGlobalScope::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

} // namespace Starfish

#endif /* STARFISH_ENABLE_WORKER */
