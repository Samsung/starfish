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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

namespace Starfish {

#ifdef STARFISH_WEBWORKER_HOST

class SendEventTask : public ParallelTask {
public:
    SendEventTask(String* eventName)
        : m_eventName(eventName)
    {
    }
    void run() override
    {
        WorkerGlobalScope* global = WorkerGlobalScope::getCurrent();
        if (global) {
            TRACE(HOST, "Dispatch an Event", m_eventName);
            // TODO: Use ExtendableEvent.
            global->dispatchEvent(
                new Event(global->executionContext(), m_eventName));
        } else {
            TRACE0(HOST, "global is null");
        }
    }

private:
    String* m_eventName;
};

#endif

void ServiceWorkerRegistrationData::sendEventTask(std::string eventname)
{
    // TODO: Move this to WorkerGlobalScope
#ifdef STARFISH_WEBWORKER_HOST
    TRACE(HOST, "Add a task");
    (new MessageLoop())
        ->addIdler(
            nullptr,
            [](size_t handle, void* data) { ((ParallelTask*)data)->run(); },
            new SendEventTask(String::createASCIIString(eventname.c_str(),
                                                        eventname.length())));

#endif
}

void ServiceWorkerRegistrationData::setInstallingWorker(
    Nullable<ServiceWorkerData*> worker)
{
    // https://w3c.github.io/ServiceWorker/#execution-context-events
    // `install` event is dispated when the service worker's containing
    //  service worker registration’s installing worker changes.
    TRACE_SCOPE(HOST);
    sendEventTask("install");
    m_installingWorker = worker;
}

void ServiceWorkerRegistrationData::setWaitingWorker(
    Nullable<ServiceWorkerData*> worker)
{
    TRACE_SCOPE(HOST);
    sendEventTask("waiting");
    m_waitingWorker = worker;
}

void ServiceWorkerRegistrationData::setActiveWorker(
    Nullable<ServiceWorkerData*> worker)
{
    TRACE_SCOPE(HOST);
    sendEventTask("active");
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
    ar.MemberArchivable("installingWorker", (Archivable*&)m_installingWorker);
    ar.MemberArchivable("waitingWorker", (Archivable*&)m_waitingWorker);
    ar.MemberArchivable("activeWorker", (Archivable*&)m_activeWorker);
    ar.MemberEnum("updateViaCache", updateViaCache);
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
