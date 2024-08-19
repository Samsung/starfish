/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_USE_WORKER_PROCESS)

#ifndef __StarfishWorkerManager__
#define __StarfishWorkerManager__

namespace Starfish {

class WorkerHostManager;
class WorkerClientManager;
class WorkerSettings;
class PerProcess;

class WorkerManager : public gc {
public:
    static WorkerManager* create(Starfish* starfish);

    virtual bool isWorkerHostManager() const
    {
        return false;
    }

    virtual bool isWorkerClientManager() const
    {
        return false;
    }

    virtual void destroy();

    WorkerHostManager* asWorkerHostManager();
    WorkerClientManager* asWorkerClientManager();

    DEFINE_GETTER(WorkerSettings*, workerSettings);
    DEFINE_GETTER(PerProcess*, perProcess);

protected:
    WorkerManager(Starfish* starfish);

    WorkerSettings* m_workerSettings{ nullptr };
    PerProcess* m_perProcess{ nullptr };
};

} // namespace Starfish

#endif
#endif
