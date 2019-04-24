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
    !defined(__StarfishServiceWorkerRegistrationData__)
#define __StarfishServiceWorkerRegistrationData__

namespace Starfish {

class String;
class ServiceWorkerData;

enum class ServiceWorkerRegistrationState {
    Installing = 0,
    Waiting,
    Active,
};

class ServiceWorkerRegistrationData : public Archivable {
public:
    String* scope{ String::emptyString };
    ServiceWorkerData* installingWorker{ nullptr };
    ServiceWorkerData* waitingWorker{ nullptr };
    ServiceWorkerData* activeWorker{ nullptr };
    ServiceWorkerUpdateViaCache updateViaCache{
        ServiceWorkerUpdateViaCache::None
    };

    bool isValid()
    {
        return (scope != String::emptyString);
    }

    // serialize/deserialize
    const char* archiveId() const override;
    void archive(Archiver& ar) override;
};

} // namespace Starfish

#endif
