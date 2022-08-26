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
    !defined(__StarfishMessageServiceWorker__)
#define __StarfishMessageServiceWorker__

#include "core/util/Archivable.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"

namespace Starfish {
class String;
class ServiceWorkerData;
class ServiceWorkerRegistrationData;

class UpdateRegistrationState : public Archivable {
public:
    DEFINE_ARCHIVE_ID_GETTER(UpdateRegistrationState);

    ServiceWorkerRegistrationData* registration;
    ServiceWorkerRegistrationState target;
    ServiceWorkerData* source;

    UpdateRegistrationState() = default;
    UpdateRegistrationState(ServiceWorkerRegistrationData* registration_,
                            ServiceWorkerRegistrationState target_,
                            ServiceWorkerData* source_)
        : registration(registration_)
        , target(target_)
        , source(source_)
    {
    }

    // serialize/deserialize
    void archive(Archiver& ar) override;
};

class UpdateWorkerStateData : public Archivable {
public:
    String* scriptURL;
    ServiceWorkerState state;

    UpdateWorkerStateData() = default;
    UpdateWorkerStateData(String* scriptURL, ServiceWorkerState target);

    // serialize/deserialize
    const char* archiveId() const override;
    void archive(Archiver& ar) override;
};

// ContextRequestData

enum class ServiceWorkerClientRequestType : unsigned {
    Register = 0,
    Unregister,
};

class ContextRequestData : public Archivable {
public:
    DEFINE_ARCHIVE_ID_GETTER(ContextRequestData);

    // data
    ServiceWorkerContextId contextId;
    ServiceWorkerRegistrationId registrationId;
    ServiceWorkerClientRequestType type;

    // constructor
    ContextRequestData() = default;
    ContextRequestData(ServiceWorkerContextId id,
                       ServiceWorkerClientRequestType target,
                       ServiceWorkerRegistrationId regId)
        : contextId(id)
        , registrationId(regId)
        , type(target)
    {
    }

    // serialize/deserialize
    void archive(Archiver& ar) override
    {
        ar.MemberId("contextId", contextId);
        ar.MemberId("registrationId", registrationId);
        ar.MemberEnum("type", type);
    }
};

} // namespace Starfish

#endif
