/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#include "CDPClient.h"

#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <chrono>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

namespace StarfishCLI {

// The largest WebSocket payload this client will hold. It matches the cap the
// CLI puts on a socket response, and a CDP message from our own engine stays
// far below it.
static constexpr uint64_t kMaximumFramePayload = 16 * 1024 * 1024;

// The longest a single read waits before the loop checks its own deadline.
// It only bounds how quickly a caller notices the timeout, not the timeout.
static constexpr int kReadSliceMs = 2000;

// How long sendCommand waits for the response to the id it sent.
static constexpr int kCommandTimeoutMs = 30000;

// Milliseconds left until deadline, never below zero.
static int millisecondsUntil(
    const std::chrono::steady_clock::time_point& deadline)
{
    auto left = std::chrono::duration_cast<std::chrono::milliseconds>(
                    deadline - std::chrono::steady_clock::now())
                    .count();
    if (left < 0) {
        return 0;
    }
    return static_cast<int>(left);
}

static std::string toJsonString(const rapidjson::Value& v)
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> w(sb);
    v.Accept(w);
    return sb.GetString();
}

static std::string base64Encode(const uint8_t* data, size_t len)
{
    static const char* ch =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3) {
        uint32_t v = static_cast<uint32_t>(data[i]) << 16;
        if (i + 1 < len) {
            v |= static_cast<uint32_t>(data[i + 1]) << 8;
        }
        if (i + 2 < len) {
            v |= static_cast<uint32_t>(data[i + 2]);
        }
        out += ch[(v >> 18) & 63];
        out += ch[(v >> 12) & 63];
        out += (i + 1 < len) ? ch[(v >> 6) & 63] : '=';
        out += (i + 2 < len) ? ch[v & 63] : '=';
    }
    return out;
}

CDPClient::CDPClient()
{
    srand((unsigned)time(nullptr));
}

CDPClient::~CDPClient()
{
    disconnect();
}

bool CDPClient::connect(const std::string& host, uint16_t port,
                        std::string& error)
{
    struct addrinfo hints = {}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    std::string portStr = std::to_string(port);
    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res) != 0) {
        error = "getaddrinfo failed";
        return false;
    }
    m_fd = socket(res->ai_family, res->ai_socktype, 0);
    if (m_fd < 0) {
        freeaddrinfo(res);
        error = "socket() failed";
        return false;
    }
    if (::connect(m_fd, res->ai_addr, res->ai_addrlen) < 0) {
        freeaddrinfo(res);
        close(m_fd);
        m_fd = -1;
        error = "connect() refused";
        return false;
    }
    freeaddrinfo(res);
    return wsHandshake(error);
}

void CDPClient::disconnect()
{
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }
    m_tcpBuf.clear();
    m_expectedEventMethod.clear();
    m_pendingEventParameters.clear();
    m_hasPendingEvent = false;
}

bool CDPClient::readBytes(void* buf, size_t n, int timeoutMs)
{
    auto* dst = static_cast<char*>(buf);
    size_t got = 0;

    // Use the bytes we already read from the socket first.
    size_t fromBuf = std::min(n, m_tcpBuf.size());
    if (fromBuf) {
        memcpy(dst, m_tcpBuf.data(), fromBuf);
        m_tcpBuf.erase(0, fromBuf);
        got += fromBuf;
    }

    while (got < n) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(m_fd, &fds);
        timeval tv;
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;
        int r = select(m_fd + 1, &fds, nullptr, nullptr, &tv);
        if (r < 0 && errno == EINTR) {
            continue;
        }
        if (r <= 0) {
            return false;
        }
        ssize_t nr = recv(m_fd, dst + got, n - got, 0);
        if (nr < 0 && errno == EINTR) {
            continue;
        }
        if (nr <= 0) {
            return false;
        }
        got += static_cast<size_t>(nr);
    }
    return true;
}

bool CDPClient::sendBytes(const void* buffer, size_t size)
{
    const char* bytes = static_cast<const char*>(buffer);
    size_t sent = 0;
    while (sent < size) {
        ssize_t count = send(m_fd, bytes + sent, size - sent, MSG_NOSIGNAL);
        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count <= 0) {
            return false;
        }
        sent += static_cast<size_t>(count);
    }
    return true;
}

bool CDPClient::wsHandshake(std::string& error)
{
    uint8_t keyBytes[16];
    for (int i = 0; i < 16; i++) {
        keyBytes[i] = static_cast<uint8_t>(rand() & 0xff);
    }
    std::string key = base64Encode(keyBytes, 16);

    std::string req =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Sec-WebSocket-Key: " +
        key +
        "\r\n"
        "\r\n";

    if (!sendBytes(req.data(), req.size())) {
        error = "failed to send WS handshake";
        return false;
    }

    // A live CDP endpoint answers the upgrade at once over loopback, so keep
    // this short. The caller retries, so a slow answer costs another try
    // instead of the whole connect budget.
    constexpr int handshakeTimeoutSeconds = 1;

    // Read one byte at a time until the header ends.
    std::string resp;
    char c;
    while (true) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(m_fd, &fds);
        timeval tv = { handshakeTimeoutSeconds, 0 };
        if (select(m_fd + 1, &fds, nullptr, nullptr, &tv) <= 0) {
            error = "WS handshake timeout";
            return false;
        }
        if (recv(m_fd, &c, 1, 0) != 1) {
            error = "WS handshake recv failed";
            return false;
        }
        resp += c;
        if (resp.size() >= 4 &&
            resp.compare(resp.size() - 4, 4, "\r\n\r\n") == 0) {
            break;
        }
        if (resp.size() > 8192) {
            error = "WS handshake response too large";
            return false;
        }
    }

    if (resp.find("101") == std::string::npos) {
        error = "expected HTTP 101, got: " + resp.substr(0, 100);
        return false;
    }
    return true;
}

bool CDPClient::sendWsFrame(const std::string& text)
{
    size_t len = text.size();
    std::string frame;

    frame += static_cast<char>(0x81); // FIN + text opcode

    // Payload length + MASK bit
    if (len < 126) {
        frame += static_cast<char>(len | 0x80);
    } else if (len < 65536) {
        frame += static_cast<char>(126 | 0x80);
        frame += static_cast<char>((len >> 8) & 0xff);
        frame += static_cast<char>(len & 0xff);
    } else {
        frame += static_cast<char>(127 | 0x80);
        for (int i = 7; i >= 0; i--) {
            frame += static_cast<char>((len >> (8 * i)) & 0xff);
        }
    }

    // 4-byte random mask key
    uint8_t mask[4];
    for (int i = 0; i < 4; i++) {
        mask[i] = static_cast<uint8_t>(rand() & 0xff);
        frame += static_cast<char>(mask[i]);
    }

    // Masked payload
    for (size_t i = 0; i < len; i++) {
        frame += static_cast<char>(text[i] ^ mask[i % 4]);
    }

    return sendBytes(frame.data(), frame.size());
}

bool CDPClient::recvWsFrame(std::string& text, int timeoutMs)
{
    uint8_t b0, b1;
    if (!readBytes(&b0, 1, timeoutMs)) {
        return false;
    }
    if (!readBytes(&b1, 1, timeoutMs)) {
        return false;
    }

    uint8_t opcode = b0 & 0x0f;
    bool masked = (b1 & 0x80) != 0;
    uint64_t payloadLen = b1 & 0x7f;

    if (payloadLen == 126) {
        uint8_t ext[2];
        if (!readBytes(ext, 2, timeoutMs)) {
            return false;
        }
        payloadLen = (static_cast<uint16_t>(ext[0]) << 8) | ext[1];
    } else if (payloadLen == 127) {
        uint8_t ext[8];
        if (!readBytes(ext, 8, timeoutMs)) {
            return false;
        }
        payloadLen = 0;
        for (int i = 0; i < 8; i++) {
            payloadLen = (payloadLen << 8) | ext[i];
        }
    }

    // The length arrives from the peer, and the 64 bit form can name more
    // memory than the machine has. Refuse before allocating: a frame that
    // large is a broken peer, not a real CDP message.
    if (payloadLen > kMaximumFramePayload) {
        return false;
    }

    uint8_t maskKey[4] = {};
    if (masked) {
        if (!readBytes(maskKey, 4, timeoutMs)) {
            return false;
        }
    }

    std::string payload(payloadLen, '\0');
    if (payloadLen &&
        !readBytes(&payload[0], static_cast<size_t>(payloadLen), timeoutMs)) {
        return false;
    }

    if (masked) {
        for (size_t i = 0; i < payload.size(); i++) {
            payload[i] ^= maskKey[i % 4];
        }
    }

    // Ping: reply with pong
    if (opcode == 0x9) {
        std::string pong;
        pong += static_cast<char>(0x8A);
        pong += static_cast<char>(payload.size() | 0x80);
        uint8_t zm[4] = { 0, 0, 0, 0 };
        for (int i = 0; i < 4; i++) {
            pong += static_cast<char>(zm[i]);
        }
        pong += payload;
        if (!sendBytes(pong.data(), pong.size())) {
            return false;
        }
        return recvWsFrame(text, timeoutMs);
    }

    if (opcode == 0x8) { // close
        disconnect();
        return false;
    }

    if (opcode == 0x1 || opcode == 0x0) { // text / continuation
        text = std::move(payload);
        return true;
    }

    return recvWsFrame(text, timeoutMs); // skip unknown opcodes
}

CDPClient::PendingMsg CDPClient::parseFrame(const std::string& frame)
{
    PendingMsg msg;
    rapidjson::Document d;
    if (d.Parse(frame.c_str()).HasParseError()) {
        return msg;
    }

    if (d.HasMember("id") && d["id"].IsInt()) {
        msg.isResponse = true;
        msg.id = d["id"].GetInt();
        if (d.HasMember("result")) {
            msg.resultJson = toJsonString(d["result"]);
        } else {
            msg.resultJson = "{}";
        }
        if (d.HasMember("error") && d["error"].IsObject()) {
            auto& e = d["error"];
            msg.errorMsg = e.HasMember("message") && e["message"].IsString()
                               ? e["message"].GetString()
                               : "CDP error";
        }
    } else if (d.HasMember("method") && d["method"].IsString()) {
        msg.isResponse = false;
        msg.method = d["method"].GetString();
        if (d.HasMember("params")) {
            msg.paramsJson = toJsonString(d["params"]);
        } else {
            msg.paramsJson = "{}";
        }
    }
    return msg;
}

void CDPClient::expectEvent(const std::string& eventMethod)
{
    m_expectedEventMethod = eventMethod;
    m_pendingEventParameters.clear();
    m_hasPendingEvent = false;
}

bool CDPClient::sendCommand(const std::string& method,
                            const std::string& paramsJson,
                            std::string& resultJson, std::string& error,
                            const std::string& sessionId)
{
    int id = m_nextId++;

    std::string req = "{\"id\":" + std::to_string(id) + ",\"method\":\"" +
                      method + "\"" +
                      ",\"params\":" + (paramsJson.empty() ? "{}" : paramsJson);
    if (!sessionId.empty()) {
        req += ",\"sessionId\":\"" + sessionId + "\"";
    }
    req += "}";

    if (!sendWsFrame(req)) {
        error = "send failed";
        return false;
    }

    // Measure the real deadline. Deducting a fixed amount per frame let a
    // stream of events run far past the budget, because a frame that took
    // 200 ms only cost 50.
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(kCommandTimeoutMs);
    for (int remaining = kCommandTimeoutMs; remaining > 0;
         remaining = millisecondsUntil(deadline)) {
        std::string frame;
        if (!recvWsFrame(frame, std::min(remaining, kReadSliceMs))) {
            continue;
        }

        PendingMsg p = parseFrame(frame);
        if (p.isResponse && p.id == id) {
            resultJson = p.resultJson;
            error = p.errorMsg;
            return error.empty();
        }
        if (!p.isResponse && !m_expectedEventMethod.empty() &&
            p.method == m_expectedEventMethod && !m_hasPendingEvent) {
            m_pendingEventParameters = std::move(p.paramsJson);
            m_hasPendingEvent = true;
        }
    }

    error = "timeout waiting for response to " + method;
    return false;
}

bool CDPClient::waitForEvent(const std::string& eventMethod,
                             std::string& paramsJson, int timeoutMs)
{
    if (m_expectedEventMethod != eventMethod) {
        expectEvent(eventMethod);
    }
    if (m_hasPendingEvent) {
        paramsJson = std::move(m_pendingEventParameters);
        m_pendingEventParameters.clear();
        m_hasPendingEvent = false;
        m_expectedEventMethod.clear();
        return true;
    }

    // Same reason as sendCommand: track the real deadline, not a per frame
    // deduction.
    auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    for (int remaining = timeoutMs; remaining > 0;
         remaining = millisecondsUntil(deadline)) {
        std::string frame;
        if (!recvWsFrame(frame, std::min(remaining, kReadSliceMs))) {
            continue;
        }

        PendingMsg p = parseFrame(frame);
        if (!p.isResponse && p.method == eventMethod) {
            paramsJson = std::move(p.paramsJson);
            m_expectedEventMethod.clear();
            return true;
        }
    }
    m_expectedEventMethod.clear();
    return false;
}

} // namespace StarfishCLI
