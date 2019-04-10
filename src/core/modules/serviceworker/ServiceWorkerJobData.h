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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && \
    !defined(__StarfishServiceWorkerJobData__)
#define __StarfishServiceWorkerJobData__

namespace Starfish {

class ServiceWorkerJobData : public gc, public MessageParam {
public:
    ServiceWorkerJobId id;
    ServiceWorkerContextId contextId;
    ServiceWorkerJobType type{ ServiceWorkerJobType::Register };
    String* scopeURL{ nullptr };
    String* scriptURL{ nullptr };
    String* referrerURL{ nullptr };
    String* origin{ nullptr };
    WorkerType workerType{ WorkerType::Classic };
    ServiceWorkerUpdateViaCache updateViaCacheMode{
        ServiceWorkerUpdateViaCache::None
    };

    // serialize/deserialize
    const char* paramType() const override;
    void archive(Archiver& ar) override;
};

} // namespace Starfish

#endif
