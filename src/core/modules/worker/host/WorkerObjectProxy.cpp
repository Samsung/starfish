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

#if defined(STARFISH_ENABLE_WORKER)

#include "StarfishConfig.h"

#include "core/page/WebBase.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/worker/Worker.h"
#include "core/modules/worker/host/WorkerObjectProxy.h"

namespace Starfish {

WorkerObjectProxy::WorkerObjectProxy(ExecutionContext* executionContext,
                                     Worker* worker, WorkerThread* workerThread)
    : WorkerProxy(executionContext, workerThread)
    , m_workerObject(worker)
{
}

MessageLoop* WorkerObjectProxy::targetMessageLoop()
{
    return m_workerObject->executionContext()->webBase()->messageLoop();
}

ExecutionContext* WorkerObjectProxy::targetExecutionContext()
{
    return m_workerObject->executionContext();
}

String* WorkerObjectProxy::workerName() const
{
    return m_workerObject->workerOptions().name();
}

} // namespace Starfish

#endif
