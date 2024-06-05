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

#if defined(STARFISH_ENABLE_SHARED_WORKER)
#ifndef __StarfishSharedWorkerProcessManager__
#define __StarfishSharedWorkerProcessManager__

namespace Starfish {

class WorkerIPCAddress;
class PerProcess;
class SharedWorker;
class SharedWorkerClient;
class SharedWorkerMessagePortConnection;

namespace SharedWorkerMessage {
    class ResponseGetSharedWorker;
}

class SharedWorkerProcessManager : public gc {
public:
    static SharedWorkerProcessManager* instance();

    void init(PerProcess* perProcess);

    void start();

    void requestConnection(SharedWorker* sharedWorker);

    void closeConnection();

    void destroy();

    void startMessagePortConnection(
        const SharedWorkerMessage::ResponseGetSharedWorker& message);

    DEFINE_GETTER_SETTER(PerProcess*, perProcess, PerProcess);

private:
    SharedWorkerProcessManager();

    void addSharedWorkerObject(SharedWorker* sharedWorker);
    Nullable<SharedWorker*> getSharedWorkerObject(int32_t clientID);

    SharedWorkerMessagePortConnection* createMessagePortConnection(
        SharedWorker* sharedWorker,
        const SharedWorkerMessage::ResponseGetSharedWorker& message);

    static SharedWorkerProcessManager* m_instance;

    PerProcess* m_perProcess;
    WorkerIPCAddress* m_ipcAddress;
    SharedWorkerClient* m_client;
    bool m_isStarted;
    GCUnorderedMap<uint32_t, SharedWorker*> m_sharedWorkers;
    GCVector<SharedWorkerMessagePortConnection*> m_connections;
};

} // namespace Starfish

#endif
#endif
