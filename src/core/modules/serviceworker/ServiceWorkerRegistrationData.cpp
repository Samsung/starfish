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

#include "StarfishConfig.h"

#include "core/util/Id.h"
#include "core/util/Archiver.h"
#include "core/util/Archivable.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"

#include "core/modules/serviceworker/ServiceWorkerJobData.h"
#include "core/modules/serviceworker/ServiceWorkerData.h"

#include "core/dom/Event.h"
#include "core/modules/worker/host/WorkerGlobalScope.h"
#include "core/modules/serviceworker/util/Trace.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/serviceworker/util/ParallelTask.h"
#include "core/modules/serviceworker/ExtendableEvent.h"

#ifdef STARFISH_ENABLE_SERVICE_WORKER

namespace Starfish {

void ServiceWorkerRegistrationData::setInstallingWorker(
    ServiceWorkerData* worker)
{
    TRACE_SCOPE(HOST);
    m_installingWorker = worker;
}

void ServiceWorkerRegistrationData::setWaitingWorker(ServiceWorkerData* worker)
{
    TRACE_SCOPE(HOST);
    m_waitingWorker = worker;
}

void ServiceWorkerRegistrationData::setActiveWorker(ServiceWorkerData* worker)
{
    TRACE_SCOPE(HOST);
    m_activeWorker = worker;
}

const char* ServiceWorkerRegistrationData::archiveId() const
{
    return "ServiceWorkerRegistrationData";
}

void ServiceWorkerRegistrationData::archive(Archiver& ar)
{
    ar.MemberId("id", id);
    ar.Member("scope") & scope;
    ar.MemberArchivable("installingWorker", (Archivable**)&m_installingWorker);
    ar.MemberArchivable("waitingWorker", (Archivable**)&m_waitingWorker);
    ar.MemberArchivable("activeWorker", (Archivable**)&m_activeWorker);
    ar.MemberEnum("updateViaCache", updateViaCache);
}

#ifdef STARFISH_WEBWORKER_HOST

void SendEventTask::run()
{
    WorkerGlobalScope* global = WorkerGlobalScope::getCurrent();
    if (global) {
        TRACE(HOST, "Dispatch an Event", CSTR(m_eventName));
        global->dispatchEventByUA(
            new ExtendableEvent(global->executionContext(), m_eventName));
    } else {
        TRACE0(HOST, "global is null");
    }
}

void SendEventTask::enqueueTask(std::string eventname)
{
    TRACE_SCOPE(HOST);
    (new MessageLoop())
        ->addIdler(
            nullptr,
            [](size_t handle, void* data) { ((ParallelTask*)data)->run(); },
            new SendEventTask(String::createASCIIString(eventname.c_str(),
                                                        eventname.length())));
}

#endif

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
