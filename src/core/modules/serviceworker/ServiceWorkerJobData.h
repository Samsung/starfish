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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && \
    !defined(__StarfishServiceWorkerJobData__)
#define __StarfishServiceWorkerJobData__

#include "core/modules/serviceworker/ServiceWorkerRegistration.h"
#include "core/modules/serviceworker/ServiceWorkerUpdateViaCache.h"

namespace Starfish {

class ServiceWorkerJobData : public Archivable {
public:
    ServiceWorkerJobId id;
    ServiceWorkerContextId contextId;
    ServiceWorkerJobType type{ ServiceWorkerJobType::Register };
    String* scopeURL{ String::emptyString };
    String* scriptURL{ String::emptyString };
    String* referrerURL{ String::emptyString };
    String* clientOrigin{ String::emptyString };
    WorkerType workerType{ WorkerType::Classic };
    ServiceWorkerUpdateViaCache updateViaCacheMode{
        ServiceWorkerUpdateViaCache::None
    };

    // serialize/deserialize
    const char* archiveId() const override;
    void archive(Archiver& ar) override;
};

} // namespace Starfish

#endif
