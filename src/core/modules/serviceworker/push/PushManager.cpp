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
#include "core/page/WebBase.h"
#include "core/page/GlobalScope.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/util/Id.h"
#include "platform/process/base/ProcessType.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/serviceworker/ServiceWorkerRegistration.h"
#include "core/modules/serviceworker/push/PushSubscriptionOptions.h"
#include "core/modules/serviceworker/push/PushSubscription.h"
#if defined(STARFISH_ENABLE_SERVICE_WORKER) && !defined(STARFISH_WEBWORKER_HOST)
#include "core/modules/serviceworker/client/ServiceWorkerProcessManager.h"
#endif
#include "core/modules/serviceworker/push/PushServiceAgent.h"

#include "core/modules/serviceworker/push/PushManager.h"

namespace Starfish {

PushManager::PushManager(ExecutionContext* executionContext,
                         ServiceWorkerRegistration* registration)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_registration(registration)
    , m_applicationServerKey(String::emptyString)
    , m_pushSubscription(nullptr)
    , m_pushManagerId(executionContext->globalScope()->uid())
{
    STARFISH_ASSERT(executionContext != nullptr);
    STARFISH_ASSERT(registration != nullptr);
}

ScriptBindingInstance* PushManager::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

Promise* PushManager::subscribe()
{
    Promise* promise = new Promise(m_executionContext->scriptBindingInstance());
#if defined(STARFISH_ENABLE_SERVICE_WORKER) && !defined(STARFISH_WEBWORKER_HOST)
    m_executionContext->webBase()->messageLoop()->addIdler(
        m_executionContext->globalScope(),
        [](size_t, void* data, void* data1) {
            PushManager* pushManager = castTo<PushManager*>(data);
            Promise* promise = castTo<Promise*>(data1);
            // TODO: Check active state of registration

            NULLABLE PushSubscription* subscription = nullptr;
            auto pushServiceAgent =
                ServiceWorkerProcessManager::instance()->pushServiceAgent();
            STARFISH_ASSERT(pushServiceAgent != nullptr);

            bool canSubscribe = pushServiceAgent->requestSubscripbe(
                pushManager->pushManagerId(), subscription,
                pushManager->optionsInit(), pushManager->executionContext());

            if ((canSubscribe == false) && (subscription == nullptr)) {
                auto exception =
                    new DOMException(pushManager->executionContext(),
                                     DOMException::Code::NOT_ALLOWED_ERROR);
                promise->reject(exception->scriptValue());
                return;
            } else if ((canSubscribe == false) && (subscription != nullptr)) {
                if (pushManager->pushSubscription() != subscription) {
                    auto exception =
                        new DOMException(pushManager->executionContext(),
                                         DOMException::Code::INVALID_STATE_ERR);
                    promise->reject(exception->scriptValue());
                    return;
                }
                // Be already subscribed.
                promise->fulfill(subscription->scriptValue());
                return;
            } else {
                pushManager->setPushSubscription(subscription);
                promise->fulfill(subscription->scriptValue());
                return;
            }
        },
        this, promise);
#else
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
#endif
    return promise;
}

Promise* PushManager::subscribe(PushSubscriptionOptionsInit& options)
{
    m_optionsInit = options;
    return subscribe();
}

Promise* PushManager::getSubscription()
{
    Promise* promise = new Promise(m_executionContext->scriptBindingInstance());

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && !defined(STARFISH_WEBWORKER_HOST)
    m_executionContext->webBase()->messageLoop()->addIdler(
        m_executionContext->globalScope(),
        [](size_t, void* data, void* data1) {
            PushManager* pushManager = castTo<PushManager*>(data);
            Promise* promise = castTo<Promise*>(data1);

            if (pushManager->pushSubscription() == nullptr) {
                promise->fulfill(scriptNull());
                return;
            }

            auto pushServiceAgent =
                ServiceWorkerProcessManager::instance()->pushServiceAgent();
            STARFISH_ASSERT(pushServiceAgent != nullptr);

            NULLABLE PushSubscription* subscription =
                pushServiceAgent->findSubscription(
                    pushManager->pushManagerId());

            if (subscription == nullptr) {
                auto exception =
                    new DOMException(pushManager->executionContext(),
                                     DOMException::Code::ABORT_ERR);
                promise->reject(exception->scriptValue());
            } else {
                promise->fulfill(subscription->scriptValue());
            }
        },
        this, promise);
#else
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
#endif
    return promise;
}

void PushManager::setPushSubscription(PushSubscription* pushSubscription)
{
    STARFISH_ASSERT(pushSubscription != nullptr);
    m_pushSubscription = pushSubscription;
}
} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
