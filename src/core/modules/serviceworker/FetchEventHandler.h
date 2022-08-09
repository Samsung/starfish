/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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
    !defined(__StarfishFetchEventHandler__)
#define __StarfishFetchEventHandler__

#include "StarfishConfig.h"

namespace Starfish {

class FetchEventData;
class ServiceWorkerClientConnection;

class FetchEventHandler : public gc {
public:
    void addFetch(FetchEventData* data);
    void start(ServiceWorkerClientConnection* connection);

private:
    GCVector<FetchEventData*> m_eventDatas;
    bool m_isStarted{ false };
    ServiceWorkerClientConnection* m_connection{ nullptr };
};
} // namespace Starfish
#endif
