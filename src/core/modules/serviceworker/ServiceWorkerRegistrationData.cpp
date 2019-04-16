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

#include "StarfishConfig.h"

#include "core/util/Id.h"
#include "core/util/Archiver.h"
#include "core/util/Archivable.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"

#include "core/modules/serviceworker/ServiceWorkerJobData.h"
#include "core/modules/serviceworker/ServiceWorkerData.h"

#ifdef STARFISH_ENABLE_SERVICE_WORKER

namespace Starfish {

const char* ServiceWorkerRegistrationData::archiveId() const
{
    return "ServiceWorkerRegistrationData";
}

void ServiceWorkerRegistrationData::archive(Archiver& ar)
{
    ar.Member("scope") & scope;
    ar.MemberArchivable("installingWorker", (Archivable*&)installingWorker);
    ar.MemberArchivable("waitingWorker", (Archivable*&)waitingWorker);
    ar.MemberArchivable("activeWorker", (Archivable*&)activeWorker);
    ar.MemberEnum("updateViaCache", updateViaCache);
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
