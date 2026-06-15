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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPConnection__)
#define __StarfishCDPConnection__

#include <cstdint>
#include <string>

namespace Starfish {

class CDPServer;

// One socket: HTTP discovery -> WS upgrade -> frame decode/encode.
// GC: not inherited (IO-thread only, raw bytes).
class CDPConnection {
public:
    enum class State { Handshaking, Live, Closed };

    CDPConnection(CDPServer* server, int fd);
    ~CDPConnection();

    // Called on IO loop. recv -> process. Returns false to close connection.
    bool pump();

    // Encode a server->client text frame (no masking) and write it.
    // Called on main thread (see design 2.6).
    void sendText(const std::string& utf8json);

    State state() const
    {
        return m_state;
    }
    int fd() const
    {
        return m_fd;
    }

private:
    bool doHandshake(const std::string& request);
    bool handleHttpDiscovery(const std::string& requestLine,
                             const std::string& url);
    bool readFrames();
    void writeAll(const char* data, size_t len);
    void sendHttp(const std::string& body, const char* contentType);

    CDPServer* m_server;
    int m_fd;
    State m_state;
    std::string m_recvBuf;
};

} // namespace Starfish

#endif
