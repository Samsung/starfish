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

#ifdef STARFISH_WEBWORKER_HOST

#include "StarfishConfig.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptBindingWorkerInstance.h"
#include "core/modules/worker/host/WebWorker.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/serviceworker/util/Trace.h"
#include "core/modules/serviceworker/util/ParallelTask.h"
#include "core/modules/serviceworker/ServiceWorker.h"
#include "binding/ScriptWrappable.h"
#include "core/modules/serviceworker/host/ServiceWorkerServer.h"
#include "core/modules/serviceworker/ServiceWorkerAgent.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostJobHandler.h"

namespace Starfish {

ServiceWorkerGlobalScope::ServiceWorkerGlobalScope(WebWorker* webWorker,
                                                   ResourceURL* url,
                                                   String* charSet)
    : WorkerGlobalScope(webWorker)
{
    TRACE_SCOPE(HOST);
    STARFISH_ASSERT(webWorker != nullptr);
    STARFISH_ASSERT(url != nullptr);
    STARFISH_ASSERT(charSet != nullptr);

    m_scriptBindingInstance =
        new ScriptBindingWorkerInstance<ServiceWorkerGlobalScope>(
            webWorker->scriptEngineInstance(), this);

    initGlobalScope(url, charSet);
}

ServiceWorker* ServiceWorkerGlobalScope::serviceWorker()
{
    TRACE_SCOPE(HOST);
    // 4.1.3. serviceWorker
    //
    // The serviceWorker getter steps are to return the result of `getting the
    // service worker object` that represents this's service worker in this's
    // relevant settings object.

    // TODO: `getting the service worker object`

    STARFISH_UNIMPLEMENTED();
    return nullptr;
}

void ServiceWorkerGlobalScope::setServiceWorkerData(
    ServiceWorkerData* serviceWorker)
{
    TRACE_SCOPE(HOST);
    m_serviceWorker = serviceWorker;
}

Promise* ServiceWorkerGlobalScope::skipWaiting()
{
    TRACE_SCOPE(HOST);
    // Note: The skipWaiting() method allows this service worker to progress
    // from the registration's waiting position to active even while service
    // worker clients are using the registration.

    // https://www.w3.org/TR/service-workers/#dom-serviceworkerglobalscope-skipwaiting
    // The skipWaiting() method steps are:
    // 1. Let promise be a new promise.
    Promise* promise = new Promise(m_scriptBindingInstance);

    // 2. Run the following substeps in parallel:
    class SkipWaitingTask : public ParallelTask {
    public:
        SkipWaitingTask(ServiceWorkerGlobalScope* s, Promise* p)
            : globalScope_(s)
            , promise_(p)
        {
        }

        void run() override
        {
            TRACE_SCOPE(HOST);
            // TODO: use this serviceWorker
            // auto serviceWorkerRef = globalScope_->serviceWorker();
            ServiceWorkerData* serviceWorker =
                globalScope_->serviceWorkerData();

            STARFISH_ASSERT(serviceWorker != nullptr);

            // 2-1. Set service worker's skip waiting flag.
            serviceWorker->setSkipWaiting(true);

            // 2-2. Invoke Try Activate with service worker's containing
            // service worker registration.
            ServiceWorkerHostJobHandler* jobHander =
                ServiceWorkerAgent::instance()
                    ->serviceWorkerServer()
                    ->jobHandler();

            ServiceWorkerRegistrationData* registration =
                jobHander->getRegistration(serviceWorker->registrationId);
            STARFISH_ASSERT(registration != nullptr);

            jobHander->tryActivate(registration);

            // 2-3. Resolve promise with undefined.
            promise_->fulfill(scriptUndefined());
        }

    private:
        ServiceWorkerGlobalScope* globalScope_{ nullptr };
        Promise* promise_{ nullptr };
    };

    ParallelTask::queue(new SkipWaitingTask(this, promise));

    // 3. Return promise.
    return promise;
}

void* ServiceWorkerGlobalScope::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ServiceWorkerGlobalScope));
    static bool typeInited = false;
    static GC_descr descr;
    if (typeInited == false) {
        GC_word desc[GC_BITMAP_SIZE(ServiceWorkerGlobalScope)] = { 0 };
        ServiceWorkerGlobalScope::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(ServiceWorkerGlobalScope));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}
} // namespace Starfish

#endif /* STARFISH_WEBWORKER_HOST */
