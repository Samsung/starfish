/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include "StarfishConfig.h"

#include "core/modules/serviceworker/WorkerConfig.h"

#include "core/modules/threading/IRunnable.h"
#include "core/modules/serviceworker/IORunnable.h"
#include "core/modules/serviceworker/Connection.h"

#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/pubsub.h>
#include <nanomsg/reqrep.h>

#include "core/modules/serviceworker/SocketNN.h"

namespace Starfish {

#define RECV_TIMEOUT 1000
#define COLOR_SEND "\033[0;36m"
#define COLOR_RECV "\033[0;32m"
#define COLOR_RESET "\033[0m"

Connection::Connection()
    : m_socket(new SocketNN(AF_SP, NN_PAIR))
{
    STARFISH_ASSERT(m_socket != nullptr);
}

void Connection::send(const char* data, size_t len)
{
    STARFISH_ASSERT(data != nullptr);
    STARFISH_ASSERT(len > 0);

    m_socket->send(data, len, SCK_DONTWAIT);

    TRACEF(CONNECTION, COLOR_SEND "[SEND] %zu byte(s)" COLOR_RESET, len);
}

Socket* Connection::socket()
{
    STARFISH_ASSERT(m_socket != nullptr);
    return m_socket;
}

void Connection::onReceived(Socket* socket, const char* data, size_t len)
{
    STARFISH_ASSERT(socket != nullptr);
    STARFISH_ASSERT(data != nullptr);

    // Note: guarantee that a null terminator exists to enable treating this
    // recv buffer like a string.
    STARFISH_ASSERT(memchr(data, '\0', len));

    TRACEF(CONNECTION, COLOR_RECV "[RECV] %zu byte(s)\n%s" COLOR_RESET, len,
           data);
}

void Connection::onStopped()
{
}

// Config

std::string Connection::Config::s_handlePath;

void Connection::Config::setHandleDir(std::string path)
{
    TRACE_SCOPE(CONFIG, path);
    s_handlePath = path;
}

std::string Connection::Config::getHandleDir()
{
    return s_handlePath;
}

std::string Connection::Config::getHandlePath(const std::string& last)
{
    std::stringstream ss;
    STARFISH_ASSERT(s_handlePath.length() > 0);

    ss << s_handlePath << "/";

    // Appends more parts.
#ifdef SERVICE_WORKER_USE_SINGLE_HOST_CONNECTION
    ss << "host";
#else
    ss << last;
#endif
    return ss.str();
}

std::string Connection::Config::createAddress(const std::string& last)
{
    std::stringstream ss;

#ifdef SERVICE_WORKER_USE_SEPARATE_PROCESS
    // For Inter-Process Communication
    ss << "ipc://" << getHandlePath(last);
#else
    // For In-Process Communication
    ss << "inproc://sw/";
#endif

    TRACE(CONFIG, ss.str());
    return ss.str();
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
