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
#ifndef __StarfishSharedWorkerClient__
#define __StarfishSharedWorkerClient__

#include "core/modules/sharedworker/IPCConnection.h"

namespace Starfish {

class IPCMessageHandler;
class PerProcess;

class SharedWorkerClient final : public IPCConnection {
public:
    SharedWorkerClient(PerProcess* perProcess, const std::string& ipcAddress);

    ~SharedWorkerClient();

    void start();

    void onReceived(Socket* socket, const char* data, size_t len) override;

    void requestConnection(SharedWorker* sharedWorker);

    void requestClose();

private:
    void initMessageReceiveHandlers();

    void sendMessage(IPCMessage& message, bool force = false);

    void sendPendingMessage();

    IPCMessageHandler* m_messageHandler;

    bool m_requestFlag;
    std::deque<std::pair<void*, size_t>> m_requestMessages;
};

} // namespace Starfish

#endif
#endif
