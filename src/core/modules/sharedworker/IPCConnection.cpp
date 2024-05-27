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

#include "StarfishConfig.h"

#include "core/modules/worker/PerProcess.h"
#include "core/modules/worker/util/Trace.h"
#include "core/modules/worker/util/network/SocketNN.h"
#include "core/modules/sharedworker/IPCConnection.h"

namespace Starfish {

IPCConnection::IPCConnection(PerProcess* perProcess,
                             const std::string& ipcAddress, int protocol)
    : Connection(protocol)
    , m_perProcess(perProcess)
    , m_ipcAddress(ipcAddress)
    , m_state(State::None)
    , m_endpointId(SOCKETNN_INVALID_END_POINT)
{
}

IPCConnection::~IPCConnection() = default;

bool IPCConnection::bind()
{
    STARFISH_ASSERT(m_state == State::None);

    m_perProcess->ioRunnable()->addClient(this);

    try {
        m_endpointId = m_socket->bind(m_ipcAddress.c_str());
        TRACE(IPC, "bind:", m_ipcAddress);
    } catch (const Socket::Exception& e) {
        m_perProcess->ioRunnable()->removeClient(this);

        STARFISH_LOG_ERROR("%s: Cannot bind to %s", e.what(),
                           m_ipcAddress.c_str());
        return false;
    }

    m_state = State::Start;
    return true;
}

bool IPCConnection::connect()
{
    STARFISH_ASSERT(m_state == State::None);

    m_perProcess->ioRunnable()->addClient(this);

    try {
        m_socket->connect(m_ipcAddress.c_str());
        TRACE(IPC, "connect:", m_ipcAddress);
    } catch (const Socket::Exception& e) {
        m_perProcess->ioRunnable()->removeClient(this);

        STARFISH_LOG_ERROR("%s: Cannot connect to %s", e.what(),
                           m_ipcAddress.c_str());
        return false;
    }

    m_state = State::Start;
    return true;
}

void IPCConnection::close()
{
    if (m_state != State::Start) {
        return;
    }

    TRACE(IPC, "close:", m_ipcAddress);

    m_perProcess->ioRunnable()->removeClient(this);

    if (m_endpointId > SOCKETNN_INVALID_END_POINT) {
        m_socket->shutdown(m_endpointId);
    }

    m_state = State::Stop;
}

bool IPCConnection::isRunning()
{
    return m_state == State::Start;
}

} // namespace Starfish

#endif
