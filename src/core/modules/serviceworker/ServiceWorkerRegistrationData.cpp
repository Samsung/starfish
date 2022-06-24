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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

namespace Starfish {

#ifdef STARFISH_WEBWORKER_HOST

class Task : public gc {
public:
    virtual void run() = 0;
};

class SendEventTask : public Task {
public:
    SendEventTask(ServiceWorkerData* worker, String* eventName)
        : m_worker(worker)
        , m_eventName(eventName)
    {
    }
    void run() override
    {
        EventTarget* global = m_worker->globalObject();
        if (global) {
            TRACE(HOST, "Dispatch an Event");
            // TODO: Use ExtendableEvent.
            global->dispatchEvent(
                new Event(global->executionContext(), m_eventName));
        } else {
            LOGI(HOST, "CHECK: global != nullptr");
        }
    }

private:
    ServiceWorkerData* m_worker;
    String* m_eventName;
};

#endif

void ServiceWorkerRegistrationData::setInstallingWorker(
    Nullable<ServiceWorkerData*> worker)
{
    TRACE_SCOPE(HOST);

#ifdef STARFISH_WEBWORKER_HOST
    TRACE(HOST, "Add a task");
    // https://w3c.github.io/ServiceWorker/#execution-context-events
    // `install` event is dispated when the service worker's containing
    //  service worker registration’s installing worker changes.
    ServiceWorkerData* w = worker.getValue();
    if (w == nullptr) {
        w = m_installingWorker.getValue();
    }
    if (w) {
        (new MessageLoop())
            ->addIdler(
                nullptr,
                [](size_t handle, void* data) { ((Task*)data)->run(); },
                new SendEventTask(w, String::createASCIIString("install")));
    }
#endif

    m_installingWorker = worker;
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
    ar.MemberArchivable("waitingWorker", (Archivable*&)waitingWorker);
    ar.MemberArchivable("activeWorker", (Archivable*&)activeWorker);
    ar.MemberEnum("updateViaCache", updateViaCache);
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
