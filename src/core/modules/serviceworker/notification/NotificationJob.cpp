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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include "StarfishConfig.h"
#include "binding/ScriptWrappable.h"
#include "platform/loader/ResourceURL.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/modules/serviceworker/ServiceWorkerAgent.h"
#include "core/modules/serviceworker/notification/NotificationService.h"
#include "core/modules/serviceworker/notification/NotificationJob.h"

namespace Starfish {

NotificationJob::NotificationJob(ExecutionContext* exectionContext,
                                 String* title, NotificationOptions options)
    : m_executionContext(exectionContext)
    , m_permission(NotificationPermission::Granted)
    , m_options(options)
{
    STARFISH_ASSERT(exectionContext != nullptr);
    STARFISH_ASSERT(title != nullptr);

    m_options.setOrigin(exectionContext->baseURL()->baseURI());
    m_options.setTitle(title);
}

ExecutionContext* NotificationJob::executionContext() const
{
    return m_executionContext;
}

NotificationOptions& NotificationJob::options()
{
    return m_options;
}

void NotificationJob::runNotification(Promise* promise)
{
    STARFISH_ASSERT(promise != nullptr);
    if (m_permission != NotificationPermission::Granted) {
        auto exception = new DOMException(executionContext(),
                                          DOMException::Code::SCRIPT_TYPE_ERR);
        promise->reject(exception->scriptValue());
        return;
    }
    // TODO: Fetch step
    showNotification(promise);
}

void NotificationJob::showNotification(Promise* promise)
{
    STARFISH_ASSERT(promise != nullptr);
    auto service = ServiceWorkerAgent::instance()->notificationService();
    STARFISH_ASSERT(service != nullptr);
    if (service->replaceNotification(m_options) == false) {
        service->appendNotification(m_options);
    }

    // TODO: request notification to agent
    promise->fulfill(scriptUndefined());
}
}
#endif /* STARFISH_ENABLE_SERVICE_WORKER */
