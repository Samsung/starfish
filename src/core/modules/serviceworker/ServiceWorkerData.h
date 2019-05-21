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
    !defined(__StarfishServiceWorkerData__)
#define __StarfishServiceWorkerData__

namespace Starfish {

enum class ServiceWorkerRunningState {
    Running,
    Terminating,
    NotRunning,
};

class ServiceWorkerData : public Archivable {
public:
    String* scriptURL{ String::emptyString };
    ServiceWorkerState state{ ServiceWorkerState::Installing };
    ServiceWorkerRegistrationId registrationId;

    // serialize/deserialize
    const char* archiveId() const override;
    void archive(Archiver& ar) override;

    // NOTE: consider seperating ServiceWorker model shared
    // on both client and host.
    DEFINE_GETTER_SETTER(bool, hasPendingEvents, HasPendingEvents);
    DEFINE_GETTER_SETTER(ServiceWorkerRunningState, runningState, RunningState);

private:
    ServiceWorkerRunningState m_runningState{
        ServiceWorkerRunningState::NotRunning
    };
    bool m_hasPendingEvents{ false };
};

} // namespace Starfish

#endif
