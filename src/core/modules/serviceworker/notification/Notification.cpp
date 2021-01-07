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
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/modules/serviceworker/notification/NotificationJob.h"
#include "core/modules/serviceworker/notification/Notification.h"

namespace Starfish {

Notification::Notification(ExecutionContext* executionContext, String* title,
                           NotificationOptions options)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_job(new NotificationJob(executionContext, title, options))
{
    STARFISH_ASSERT(executionContext != nullptr);
    STARFISH_ASSERT(title != nullptr);

    if (executionContext->hasWorkerGlobalScope()) {
        throw new DOMException(executionContext, DOMException::SCRIPT_TYPE_ERR);
    }

    Promise* promise = new Promise(executionContext->scriptBindingInstance());
    m_job->runNotification(promise);
}

ExecutionContext* Notification::executionContext() const
{
    return m_executionContext;
}

String* Notification::title()
{
    return m_job->options().title();
}

String* Notification::body()
{
    return m_job->options().body();
}

String* Notification::tag()
{
    return m_job->options().tag();
}
} // namespace Starfish
#endif /* STARFISH_ENABLE_SERVICE_WORKER */
