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

#if defined(STARFISH_ENABLE_WORKER) && defined(STARFISH_WEBWORKER_HOST)

#ifndef __StarfishWorkerAgent__
#define __StarfishWorkerAgent__

namespace Starfish {

class PerProcess;
class Starfish;
class WorkerHostManager;

enum class WorkerAgentState {
    None,
    Terminated,
};

using WorkerAgentStateHandler = std::function<void(WorkerAgentState)>;

class WorkerAgent : public gc {
public:
    static WorkerAgent* create(Starfish* starfish);
    static WorkerAgent* instance();
    static bool isCreated();

    virtual void start() = 0;

    virtual void destroy();

    void registerOnStatusChangedHandler(WorkerAgentStateHandler cb);

    PerProcess* perProcess() const;

protected:
    WorkerAgent(Starfish* starfish);

    static WorkerAgent* g_workerAgentInstance;

    WorkerAgentStateHandler m_clientFunc;
    Starfish* m_starfish;
    WorkerHostManager* m_workerHostManager;
};

} // namespace Starfish

#endif
#endif
