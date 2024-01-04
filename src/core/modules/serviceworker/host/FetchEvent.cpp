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

#include "EscargotPublic.h"
#include "StarfishConfig.h"
#include "binding/ScriptWrappable.h"
#include "core/dom/DOMException.h"
#include "core/fetch/Body.h"
#include "core/fetch/Response.h"
#include "core/modules/worker/util/Trace.h"
#include "core/modules/serviceworker/host/ServiceWorkerFetchJob.h"
#include "core/modules/serviceworker/host/FetchEvent.h"

namespace Starfish {

Promise* FetchEvent::preloadResponse()
{
    if (!m_preloadResponse) {
        m_preloadResponse = new Promise(scriptBindingInstance());
        m_preloadResponse->fulfill(scriptUndefined());
    }
    return m_preloadResponse;
}

// https://w3c.github.io/ServiceWorker/#fetch-event-respondwith
void FetchEvent::respondWith(Promise* response)
{
    if (!isDispatched()) {
        TRACE_SCOPE(HOST);
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }

    if (respondWithEntered()) {
        TRACE_SCOPE(HOST);
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }

    TRACE_SCOPE(HOST);

    struct Params : public gc {
        Params(FetchEvent* event_, Promise* response_)
            : event(event_)
            , response(response_)
        {
        }
        FetchEvent* event;
        Promise* response;
    };
    Params* p = new Params(this, response);

    auto onRejectedFunction = createScriptFunction(
        scriptBindingInstance(), std::string(""),
        [](Escargot::ExecutionStateRef* state, ScriptValue thisValue,
           size_t argc, ScriptValue* argv,
           bool isNewExPression) -> ScriptValue {
            TRACE_SCOPE(HOST);
            auto onPromiseCallback = state->resolveCallee().value();
            STARFISH_ASSERT(onPromiseCallback->isFunctionObject() &&
                            onPromiseCallback->asFunctionObject()->extraData());
            auto p = static_cast<Params*>(
                onPromiseCallback->asFunctionObject()->extraData());
            auto event = p->event;

            event->setRespondWithError(true);
            event->setWaitToRespond(false);

            if (event->fetchJob()) {
                event->fetchJob()->onCompleteFetch(event);
            }

            return scriptUndefined();
        },
        1, true, false);

    onRejectedFunction->asFunctionObject()->setExtraData(p);

    auto onFulfilledFunction = createScriptFunction(
        scriptBindingInstance(), std::string(""),
        [](Escargot::ExecutionStateRef* state, ScriptValue thisValue,
           size_t argc, ScriptValue* argv,
           bool isNewExPression) -> ScriptValue {
            TRACE_SCOPE(HOST);
            auto onPromiseCallback = state->resolveCallee().value();
            STARFISH_ASSERT(onPromiseCallback->isFunctionObject() &&
                            onPromiseCallback->asFunctionObject()->extraData());
            auto p = static_cast<Params*>(
                onPromiseCallback->asFunctionObject()->extraData());
            auto event = p->event;
            auto responseResult =
                toScriptWrappable(p->response->promiseResult());

            if (!responseResult->isResponse()) {
                event->setRespondWithError(true);
            } else {
                TRACE_SCOPE(HOST);
                auto response = responseResult->asResponse();
                event->fetchJob()->setResponse(response);
                auto potentialResponse = response->cloneWithoutBody();

                if (response->bodyInit().hasValue()) {
                    // 10-2-5-7 Set potentialResponse’s body to a new body whose
                    // stream is newStream.
                }

                event->setPotentialResponse(potentialResponse);
            }

            event->setWaitToRespond(false);

            if (event->fetchJob()) {
                event->fetchJob()->onCompleteFetch(event);
            }

            return scriptUndefined();
        },
        1, true, false);

    onFulfilledFunction->asFunctionObject()->setExtraData(p);

    response->then(onFulfilledFunction, onRejectedFunction);

    m_extendLifetimePromises.push_back(response);
    stopPropagation();
    stopImmediatePropagation();

    m_respondWithEntered = true;
    m_waitToRespond = true;
}

} // namespace Starfish

#endif
