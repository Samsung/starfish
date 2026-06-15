/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_CDP)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "CDPServer.h"
#include "CDPConnection.h"
#include "CDPDispatcher.h"
#include "core/page/WebView.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/ThreadPool.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace Starfish {

CDPServer::CDPServer(WebView* webView, uint16_t port)
    : m_webView(webView)
    , m_dispatcher(nullptr)
    , m_conn(nullptr)
    , m_ioThread(nullptr)
    , m_listenFd(-1)
    , m_port(port)
    , m_isRunning(false)
    , m_connAlive(false)
{
    m_dispatcher = new CDPDispatcher(this, webView);
}

CDPServer::~CDPServer()
{
    if (m_isRunning) {
        stop();
    }
    delete m_dispatcher;
    m_dispatcher = nullptr;
}

void CDPServer::start()
{
    STARFISH_ASSERT(isMainThread());
    m_ioThread = new Thread(m_webView->threadPool());
    try {
        m_ioThread->run(m_webView->messageLoop(), CDPServer::acceptLoop, this);
    } catch (...) {
        m_ioThread = nullptr;
    }
}

void CDPServer::stop()
{
    STARFISH_ASSERT(isMainThread());
    m_isRunning = false;

    // Wake a blocking accept() by shutting down the listen socket.
    if (m_listenFd >= 0) {
        ::shutdown(m_listenFd, SHUT_RDWR);
    }

    if (!m_ioThread) {
        return;
    }
    m_ioThread->joinIfNeeds();
    m_ioThread = nullptr;
}

void CDPServer::sendText(const std::string& utf8json)
{
    // Hold the lock across the liveness check and the write so the IO thread
    // cannot delete m_conn between them.
    std::lock_guard<std::mutex> lock(m_connMutex);
    if (m_connAlive.load() && m_conn) {
        m_conn->sendText(utf8json);
    }
}

void* CDPServer::acceptLoop(void* data)
{
    CDPServer* self = (CDPServer*)data;
    self->m_isRunning = true;

    int listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
        STARFISH_LOG_INFO("cdp: socket() failed");
        self->m_isRunning = false;
        return nullptr;
    }

    int opt = 1;
    ::setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(self->m_port);

    if (::bind(listenFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        STARFISH_LOG_INFO("cdp: bind() failed (port in use?)");
        ::close(listenFd);
        self->m_isRunning = false;
        return nullptr;
    }

    if (::listen(listenFd, 1) < 0) {
        STARFISH_LOG_INFO("cdp: listen() failed");
        ::close(listenFd);
        self->m_isRunning = false;
        return nullptr;
    }

    self->m_listenFd = listenFd;
    STARFISH_LOG_INFO("cdp: devtools server listening on port %d",
                      (int)self->m_port);

    while (self->m_isRunning) {
        int fd = ::accept(listenFd, nullptr, nullptr);
        if (fd < 0) {
            break; // shutdown() or error
        }

        CDPConnection* conn = new CDPConnection(self, fd);
        {
            std::lock_guard<std::mutex> lock(self->m_connMutex);
            self->m_conn = conn;
            self->m_connAlive.store(true);
        }

        while (self->m_isRunning && conn->pump()) {
        }

        {
            // Serialize teardown against a concurrent main-thread sendText().
            std::lock_guard<std::mutex> lock(self->m_connMutex);
            self->m_connAlive.store(false);
            self->m_conn = nullptr;
            delete conn;
        }

        // Reset connection-scoped session state on the main thread so the next
        // client gets a fresh attach handshake (otherwise browser.pages() is
        // empty on reconnect). Skip during shutdown.
        if (self->m_isRunning && self->m_dispatcher) {
            self->m_dispatcher->onConnectionClosed();
        }
    }

    ::close(listenFd);
    self->m_listenFd = -1;
    self->m_isRunning = false;
    STARFISH_LOG_INFO("cdp: devtools io thread end");
    return nullptr;
}

} // namespace Starfish

#endif
