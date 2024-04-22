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

#if defined(STARFISH_ENABLE_SHARED_WORKER) && defined(STARFISH_WEBWORKER_HOST)

#ifndef __StarfishSharedWorkerAgent__
#define __StarfishSharedWorkerAgent__

#include "core/modules/worker/host/WorkerAgent.h"

namespace Starfish {

class SharedWorkerAgent;
class SharedWorkerAgentServer;
class Starfish;
class WorkerIPCAddress;

class SharedWorkerAgent final : public WorkerAgent {
    friend class WorkerAgent;

public:
    static SharedWorkerAgent* instance();

    void start() override;

    void destroy() override;

private:
    SharedWorkerAgent(Starfish* starfish);

    static SharedWorkerAgent* m_instance;
    SharedWorkerAgentServer* m_server;
    WorkerIPCAddress* m_ipcAddress;
};
} // namespace Starfish

#endif
#endif
