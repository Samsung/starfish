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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPServer__)
#define __StarfishCDPServer__

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

namespace Starfish {

class WebView;
class Thread;
class CDPDispatcher;
class CDPConnection;

// Owns the accept loop. WebView -> CDPServer(1) -> CDPConnection(0..1).
// GC: not inherited (holds no GC objects; main-thread state reached via
// WebView).
class CDPServer {
public:
    CDPServer(WebView* webView, uint16_t port);
    ~CDPServer();

    void start(); // main thread: spin up accept thread
    void stop();  // main thread: terminate loop + join

    CDPDispatcher* dispatcher()
    {
        return m_dispatcher;
    }
    WebView* webView()
    {
        return m_webView;
    }
    uint16_t port() const
    {
        return m_port;
    }

    // Called on main thread to write a (already JSON) text frame to the live
    // connection. Guarded by m_connAlive (read=IO, write=main, see design 2.6).
    void sendText(const std::string& utf8json);

private:
    static void* acceptLoop(void* self); // IO thread entry

    WebView* m_webView;
    CDPDispatcher* m_dispatcher; // non-GC, main-thread handlers
    CDPConnection* m_conn;       // current single connection, non-GC (IO owned)
    Thread* m_ioThread;
    int m_listenFd;
    uint16_t m_port;
    volatile bool m_isRunning;
    std::atomic<bool> m_connAlive;
    // Guards m_conn against the IO thread tearing it down while the main thread
    // is writing via sendText() (read=IO, write=main; see design 2.6).
    std::mutex m_connMutex;
};

} // namespace Starfish

#endif
