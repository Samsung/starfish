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
#ifndef __StarfishSharedWorkerMessagePortConnection__
#define __StarfishSharedWorkerMessagePortConnection__

#include "core/modules/sharedworker/IPCConnection.h"

namespace Starfish {

class PerProcess;
class MessagePort;

class SharedWorkerMessagePortConnection final : public IPCConnection {
public:
    SharedWorkerMessagePortConnection(PerProcess* perProcess,
                                      MessagePort* m_messagePort,
                                      uint32_t clientID, uint32_t pid,
                                      const std::string& ipcAddress);

    ~SharedWorkerMessagePortConnection();

    void onReceived(Socket* socket, const char* data, size_t size) override;

    IMessageLoop* messageLoop() override;

    DEFINE_GETTER(uint32_t, clientID);
    DEFINE_GETTER(uint32_t, pid);

private:
    MessagePort* m_messagePort;
    uint32_t m_clientID;
    uint32_t m_pid;
};

} // namespace Starfish

#endif
#endif
