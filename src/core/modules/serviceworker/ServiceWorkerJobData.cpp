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
#include "core/modules/serviceworker/MessageParam.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/ServiceWorkerJobData.h"

#ifdef STARFISH_ENABLE_SERVICE_WORKER

namespace Starfish {

const char* ServiceWorkerJobData::paramType() const
{
    return "ServiceWorkerJobData";
}

void ServiceWorkerJobData::archive(Archiver& ar)
{
    ar.MemberId("id", id);
    ar.MemberId("contextId", contextId);
    ar.MemberEnum("type", type);
    ar.MemberEnum("workerType", workerType);
    ar.MemberEnum("updateViaCacheMode", updateViaCacheMode);
    ar.Member("scopeURL") & scopeURL;
    ar.Member("scriptURL") & scriptURL;
    ar.Member("referrerURL") & referrerURL;
    ar.Member("origin") & origin;
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
