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
    !defined(__StarfishServiceWorkerServerInterface__)
#define __StarfishServiceWorkerServerInterface__

namespace Starfish {

class IServiceWorkerClientConnection;
class ServiceWorkerHostJobHandler;

class ServiceWorkerServerInterface {
public:
    virtual void getConnections(
        GCVector<IServiceWorkerClientConnection*>& connections) = 0;
    virtual ServiceWorkerHostJobHandler* jobHandler() = 0;
    virtual bool tryTerminate() = 0;
    virtual bool isTerminating() = 0;
};

class ServiceWorkerServerClient {
public:
    virtual void onSWServerTerminated() = 0;
};

} // namespace Starfish
#endif
