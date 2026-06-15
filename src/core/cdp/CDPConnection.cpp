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
#include "CDPConnection.h"
#include "CDPServer.h"
#include "CDPDispatcher.h"
#include "Sha1.h"
#include "Base64.h"

#include <algorithm>
#include <cctype>
#include <sys/socket.h>
#include <unistd.h>

namespace Starfish {

static const char* kWsGuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

CDPConnection::CDPConnection(CDPServer* server, int fd)
    : m_server(server)
    , m_fd(fd)
    , m_state(State::Handshaking)
{
}

CDPConnection::~CDPConnection()
{
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

void CDPConnection::writeAll(const char* data, size_t len)
{
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(m_fd, data + sent, len - sent, MSG_NOSIGNAL);
        if (n <= 0) {
            return;
        }
        sent += (size_t)n;
    }
}

void CDPConnection::sendHttp(const std::string& body, const char* contentType)
{
    std::string resp = "HTTP/1.1 200 OK\r\n";
    resp += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    resp += "Connection: Close\r\n";
    resp += std::string("Content-Type: ") + contentType + "\r\n\r\n";
    resp += body;
    writeAll(resp.data(), resp.size());
}

static std::string headerValue(const std::string& req, const char* name)
{
    // Case-insensitive header lookup. Returns trimmed value or "".
    std::string lower = req;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    std::string key = name;
    std::transform(key.begin(), key.end(), key.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    size_t pos = lower.find("\r\n" + key + ":");
    if (pos == std::string::npos) {
        return "";
    }
    size_t valStart = pos + 2 + key.size() + 1;
    size_t valEnd = req.find("\r\n", valStart);
    if (valEnd == std::string::npos) {
        return "";
    }
    std::string v = req.substr(valStart, valEnd - valStart);
    size_t b = v.find_first_not_of(" \t");
    size_t e = v.find_last_not_of(" \t");
    if (b == std::string::npos) {
        return "";
    }
    return v.substr(b, e - b + 1);
}

bool CDPConnection::handleHttpDiscovery(const std::string& /*requestLine*/,
                                        const std::string& url)
{
    if (url == "/json/version") {
        std::string port = std::to_string(m_server->port());
        std::string body =
            "{\"Browser\":\"Starfish/1.0\",\"Protocol-Version\":\"1.3\","
            "\"User-Agent\":\"Starfish/1.0\","
            "\"webSocketDebuggerUrl\":\"ws://127.0.0.1:" +
            port + "/\"}";
        sendHttp(body, "application/json; charset=UTF-8");
        return false; // Connection: Close
    }

    if (url == "/json" || url == "/json/list" || url == "/json/list/") {
        std::string port = std::to_string(m_server->port());
        std::string body =
            "[{\"description\":\"\",\"id\":\"TID-0000000001\",\"title\":"
            "\"Starfish\",\"type\":\"page\",\"url\":\"about:blank\","
            "\"webSocketDebuggerUrl\":\"ws://127.0.0.1:" +
            port + "/devtools/page/TID-0000000001\"}]";
        sendHttp(body, "application/json; charset=UTF-8");
        return false;
    }

    // Unknown discovery path.
    std::string body = "{}";
    sendHttp(body, "application/json; charset=UTF-8");
    return false;
}

bool CDPConnection::doHandshake(const std::string& request)
{
    // Parse request line: GET <url> HTTP/1.1
    size_t sp1 = request.find(' ');
    size_t sp2 = (sp1 == std::string::npos) ? std::string::npos
                                            : request.find(' ', sp1 + 1);
    if (sp1 == std::string::npos || sp2 == std::string::npos) {
        return false;
    }
    std::string url = request.substr(sp1 + 1, sp2 - sp1 - 1);

    std::string upgrade = headerValue(request, "Upgrade");
    std::string wsKey = headerValue(request, "Sec-WebSocket-Key");

    std::string upgradeLower = upgrade;
    std::transform(upgradeLower.begin(), upgradeLower.end(),
                   upgradeLower.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });

    bool isWsUpgrade =
        (upgradeLower.find("websocket") != std::string::npos) && !wsKey.empty();

    if (!isWsUpgrade) {
        return handleHttpDiscovery(request, url);
    }

    // Compute Sec-WebSocket-Accept = base64(sha1(key + GUID)).
    std::string toHash = wsKey + kWsGuid;
    uint8_t digest[20];
    cdpSha1((const uint8_t*)toHash.data(), toHash.size(), digest);
    std::string accept = cdpBase64Encode(digest, 20);

    std::string resp = "HTTP/1.1 101 Switching Protocols\r\n";
    resp += "Upgrade: websocket\r\n";
    resp += "Connection: Upgrade\r\n";
    resp += "Sec-WebSocket-Accept: " + accept + "\r\n\r\n";
    writeAll(resp.data(), resp.size());

    m_state = State::Live;
    return true;
}

bool CDPConnection::readFrames()
{
    // Decode as many complete frames as buffered. Returns false to close.
    for (;;) {
        if (m_recvBuf.size() < 2) {
            return true;
        }
        const uint8_t* p = (const uint8_t*)m_recvBuf.data();
        uint8_t b0 = p[0];
        uint8_t b1 = p[1];
        uint8_t opcode = b0 & 0x0F;
        bool masked = (b1 & 0x80) != 0;
        uint64_t payloadLen = b1 & 0x7F;
        size_t headerLen = 2;

        if (payloadLen == 126) {
            if (m_recvBuf.size() < 4) {
                return true;
            }
            payloadLen = ((uint64_t)p[2] << 8) | (uint64_t)p[3];
            headerLen = 4;
        } else if (payloadLen == 127) {
            if (m_recvBuf.size() < 10) {
                return true;
            }
            payloadLen = 0;
            for (int i = 0; i < 8; i++) {
                payloadLen = (payloadLen << 8) | (uint64_t)p[2 + i];
            }
            headerLen = 10;
        }

        size_t maskLen = masked ? 4 : 0;
        size_t totalLen = headerLen + maskLen + (size_t)payloadLen;
        if (m_recvBuf.size() < totalLen) {
            return true; // wait for more bytes
        }

        uint8_t maskKey[4] = { 0, 0, 0, 0 };
        if (masked) {
            for (int i = 0; i < 4; i++) {
                maskKey[i] = p[headerLen + i];
            }
        }

        std::string payload;
        payload.resize((size_t)payloadLen);
        const uint8_t* data = p + headerLen + maskLen;
        for (size_t i = 0; i < (size_t)payloadLen; i++) {
            payload[i] = (char)(masked ? (data[i] ^ maskKey[i % 4]) : data[i]);
        }

        m_recvBuf.erase(0, totalLen);

        if (opcode == 0x1) { // text
            m_server->dispatcher()->onMessageFromIO(std::move(payload));
        } else if (opcode == 0x9) { // ping -> pong
            std::string pong;
            pong.push_back((char)0x8A);
            pong.push_back((char)(payload.size() & 0x7F));
            pong += payload;
            writeAll(pong.data(), pong.size());
        } else if (opcode == 0x8) { // close
            char closeFrame[2] = { (char)0x88, 0x00 };
            writeAll(closeFrame, 2);
            m_state = State::Closed;
            return false;
        }
        // opcode 0x0 (continuation) / 0xA (pong) ignored: MVP single frame.
    }
}

void CDPConnection::sendText(const std::string& utf8json)
{
    std::string frame;
    frame.push_back((char)0x81); // FIN + text

    size_t len = utf8json.size();
    if (len <= 125) {
        frame.push_back((char)len);
    } else if (len < 65536) {
        frame.push_back((char)126);
        frame.push_back((char)((len >> 8) & 0xFF));
        frame.push_back((char)(len & 0xFF));
    } else {
        frame.push_back((char)127);
        for (int i = 7; i >= 0; i--) {
            frame.push_back((char)((len >> (8 * i)) & 0xFF));
        }
    }
    frame += utf8json;
    writeAll(frame.data(), frame.size());
}

bool CDPConnection::pump()
{
    char buf[4096];
    ssize_t n = ::recv(m_fd, buf, sizeof(buf), 0);
    if (n <= 0) {
        return false; // peer closed or error
    }
    m_recvBuf.append(buf, (size_t)n);

    if (m_state == State::Handshaking) {
        size_t end = m_recvBuf.find("\r\n\r\n");
        if (end == std::string::npos) {
            return true; // wait for full request headers
        }
        std::string request = m_recvBuf.substr(0, end + 4);
        m_recvBuf.erase(0, end + 4);
        if (!doHandshake(request)) {
            return false; // HTTP discovery -> close
        }
        // fall through: there may be buffered ws frames already
    }

    if (m_state == State::Live) {
        return readFrames();
    }

    return m_state != State::Closed;
}

} // namespace Starfish

#endif
