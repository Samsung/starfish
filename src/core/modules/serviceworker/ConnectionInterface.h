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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && !defined(__ConnectionInterface__)
#define __ConnectionInterface__

#include "StarfishBase.h"
#include "core/modules/serviceworker/ServiceWorkerTypes.h"

namespace Starfish {

class ServiceWorkerData;
class ServiceWorkerJob;
class ServiceWorkerRequest;
class ServiceWorkerRegistrationData;
class Archivable;
class ExceptionData;
class ContextRequestData;
class FetchEventRequestData;
class FetchEventResponseData;

// IServiceWorkerHostConnection which is used on `Client`

class IServiceWorkerHostConnection {
public:
    virtual ~IServiceWorkerHostConnection() = default;

    // send
    virtual void scheduleJob(ServiceWorkerJob* job) = 0;
    virtual void matchRegistration(ServiceWorkerRequest* request,
                                   String* clientURL) = 0;
    virtual void updateServiceWorkerClient(ContextRequestData* request) = 0;
    virtual void fetchEvent(FetchEventRequestData* FetchEventRequestData) = 0;
    virtual void startServiceWorkerContext(
        ServiceWorkerData* serviceWorkerData) = 0;

    // receive
    // NOTE: For rapid development, declaring receive handler isn't compulsory
    // for now. However, unimplemented message handler is detected in run time.
};

// IServiceWorkerClientConnection which is used on `Host`

class IServiceWorkerClientConnection {
public:
    virtual ~IServiceWorkerClientConnection() = default;

    // send
    virtual void resolveJobPromise(
        ServiceWorkerJob* job, ServiceWorkerRegistrationData* registration) = 0;

    virtual void rejectJobPromise(ServiceWorkerJob* job,
                                  ExceptionData* errorData) = 0;

    virtual void resolveRequest(ServiceWorkerRequest* request,
                                NULLABLE Archivable* registration) = 0;

    virtual void onUpdateRegistrationState(
        ServiceWorkerRegistrationData* registration,
        ServiceWorkerRegistrationState target, ServiceWorkerData* source) = 0;

    virtual void onUpdateWorkerState(String* scriptURL,
                                     ServiceWorkerState target) = 0;

    virtual void fireEventRequest(String* scriptURL, String* eventName) = 0;

    virtual void respondFetchEvent(FetchEventResponseData* data) = 0;

    // receive
    // NOTE: For rapid development, declaring receive handler isn't compulsory
    // for now. However, unimplemented message handler is detected in run time.
};

} // namespace Starfish
#endif
