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

#include "core/dom/ExecutionContext.h"
#include "core/util/Id.h"
#include "core/util/Archiver.h"
#include "core/util/Archivable.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ServiceWorkerJobData.h"
#include "core/modules/serviceworker/ServiceWorkerJob.h"

#ifdef STARFISH_ENABLE_SERVICE_WORKER

namespace Starfish {

ServiceWorkerJob::ServiceWorkerJob(ServiceWorkerJobData* data)
{
    if (data != nullptr) {
        m_data = data;
    } else {
        m_data = new ServiceWorkerJobData();
    }
    STARFISH_ASSERT(m_data != nullptr);
}

ServiceWorkerRegistrationKey ServiceWorkerJob::registrationKey()
{
    // TODO: generate an unique key using data attributes
    auto key = m_data->scopeURL->concat(m_data->scriptURL);
    return key;
}
} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
