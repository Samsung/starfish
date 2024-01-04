/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && defined(STARFISH_WEBWORKER_HOST)

#include <EscargotPublic.h>

#include "StarfishConfig.h"
#include "binding/ScriptWrappable.h"
#include "core/page/WebBase.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/worker/util/Trace.h"
#include "core/modules/serviceworker/host/ServiceWorkerAgent.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"
#include "core/modules/serviceworker/host/ServiceWorkerGlobalScope.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostJobHandler.h"
#include "core/modules/serviceworker/host/ServiceWorkerServer.h"
#include "core/modules/serviceworker/host/ServiceWorkerServerInterface.h"
#include "core/modules/serviceworker/ServiceWorkerData.h"
#include "core/modules/worker/host/WorkerGlobalScope.h"
#include "core/modules/serviceworker/host/ExtendableEvent.h"

namespace Starfish {

void ExtendableEvent::waitUntil(Promise* promise)
{
    TRACEF(HOST, "%s, %d", type()->toUTF8String().data(), isTrusted());

    if (!isTrusted()) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
        return;
    }

    if (!isEventActive()) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
        return;
    }

    auto onSettledCallback = [](Escargot::ExecutionStateRef* state,
                                ScriptValue thisValue, size_t argc,
                                ScriptValue* argv,
                                bool isNewExPression) -> ScriptValue {
        STARFISH_ASSERT(state->resolveCallee().hasValue());

        auto onPromiseCallback = state->resolveCallee().value();
        auto extendableEvent = static_cast<ExtendableEvent*>(
            onPromiseCallback->asFunctionObject()->extraData());

        extendableEvent->enqueueWaitUntilMicrotask();

        return scriptUndefined();
    };

    auto onSettledFunction =
        createScriptFunction(scriptBindingInstance(), std::string(""),
                             onSettledCallback, 1, true, false);
    onSettledFunction->asFunctionObject()->setExtraData(this);

    promise->then(onSettledFunction, onSettledFunction);

    m_extendLifetimePromises.push_back(promise);
    incrementPendingPromiseCount();
}

void ExtendableEvent::enqueueWaitUntilMicrotask()
{
    TRACE_SCOPE(HOST);

    executionContext()->webBase()->messageLoop()->addIdler(
        executionContext()->globalScope(),
        [](size_t handle, void* data) {
            TRACE_SCOPE(HOST);
            auto extendableEvent = static_cast<ExtendableEvent*>(data);

            extendableEvent->decrementPendingPromiseCount();
            if (extendableEvent->pendingPromisesCount() != 0) {
                return;
            }

            auto jobHandler = ServiceWorkerAgent::instance()
                                  ->serviceWorkerServer()
                                  ->jobHandler();

            // 1. Let registration be the current global object's associated
            // service worker's containing service worker registration.
            auto globalScope = static_cast<ServiceWorkerGlobalScope*>(
                extendableEvent->executionContext()->globalScope());
            auto registration = jobHandler->getRegistration(
                globalScope->serviceWorkerData()->registrationId);

            // 2. If registration is unregistered, invoke Try Clear Registration
            // with registration.
            if (!registration) {
                // TODO: jobHandler->tryClearRegistration(registration);
                STARFISH_UNIMPLEMENTED();
                return;
            }

            // 3. If registration is not null, invoke Try Activate with
            // registration.
            jobHandler->tryActivate(registration);
        },
        this);
}

void ExtendableEvent::incrementPendingPromiseCount()
{
    ++m_pendingPromisesCount;
}

void ExtendableEvent::decrementPendingPromiseCount()
{
    --m_pendingPromisesCount;
}

unsigned int ExtendableEvent::pendingPromisesCount()
{
    return m_pendingPromisesCount;
}

bool ExtendableEvent::isEventActive() const
{
    return m_pendingPromisesCount > 0 || isDispatched();
}

} // namespace Starfish

#endif
