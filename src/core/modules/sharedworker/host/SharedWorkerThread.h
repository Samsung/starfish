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
#ifndef __StarfishSharedWorkerThread__
#define __StarfishSharedWorkerThread__

#include "core/modules/worker/WorkerThread.h"

namespace Starfish {

class Starfish;
class MessageLoop;
class WorkerGlobalScope;
class WebWorker;
class WorkerHost;
class SharedWorkerGlobalScope;
class MessagePortConnectionInfo;
struct WorkerHostInitData;

class SharedWorkerThread : public WorkerThread {
public:
    SharedWorkerThread(Starfish* starfish, MessageLoop* messageLoop,
                       const std::string& name,
                       const WorkerHostInitData& workerHostInitData);

    ~SharedWorkerThread();

    WorkerGlobalScope* createWorkerGlobalScope(WebWorker* webWorker,
                                               WorkerHost* workerHost) override;

    void startWithIdentifier(uint32_t identifier);

    void requestSharedWorkerConnection(MessagePortConnectionInfo* info);

    DEFINE_GETTER(SharedWorkerGlobalScope*, globalScope);

private:
    SharedWorkerGlobalScope* m_globalScope;
    const std::string m_name;
    uint32_t m_initialIdentifier;
};

} // namespace Starfish

#endif
#endif
