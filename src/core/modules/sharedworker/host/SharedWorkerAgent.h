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

class SharedWorkerAgentServer;
class SharedWorkerThread;
class SharedWorkerGlobalScope;
class SharedWorkerMessagePortConnection;
class Starfish;
class WorkerIPCAddress;
class MessageLoop;
class Mutex;

namespace SharedWorkerMessage {
    class RequestGetSharedWorker;
}

struct MessagePortConnectionInfo : public gc {
    MessagePortConnectionInfo(uint32_t identifier_, uint32_t clientID_,
                              SharedWorkerThread* thread_)
        : identifier(identifier_)
        , clientID(clientID_)
        , thread(thread_)
    {
    }

    uint32_t identifier;
    uint32_t clientID;
    SharedWorkerThread* thread;
};

class SharedWorkerAgent final : public WorkerAgent {
    friend class WorkerAgent;

public:
    static SharedWorkerAgent* instance();

    void start() override;

    void destroy() override;

    void connectWorkerThread(
        const SharedWorkerMessage::RequestGetSharedWorker& message);

    void didGlobalScopeConnected(SharedWorkerGlobalScope* globalScope,
                                 SharedWorkerMessagePortConnection* connection);

    Nullable<MessagePortConnectionInfo*> getConnectionInfo(uint32_t identifier);

    DEFINE_GETTER(WorkerIPCAddress*, ipcAddress);
    DEFINE_GETTER(MessageLoop*, messageLoop);

private:
    SharedWorkerAgent(Starfish* starfish);

    uint32_t createIdentifier();

    SharedWorkerThread* getWorkerThread(
        const SharedWorkerMessage::RequestGetSharedWorker& message);

    MessagePortConnectionInfo* createConnectionInfo(uint32_t clientID,
                                                    SharedWorkerThread* thread);

    DEFINE_GETTER(SharedWorkerAgentServer*, server);

    static SharedWorkerAgent* m_instance;
    MessageLoop* m_messageLoop;
    SharedWorkerAgentServer* m_server;
    WorkerIPCAddress* m_ipcAddress;
    GCUnorderedMap<size_t, SharedWorkerThread*> m_workerThreads;
    GCVector<MessagePortConnectionInfo*> m_connectionInfos;
    Mutex* m_mutex;
};
} // namespace Starfish

#endif
#endif
