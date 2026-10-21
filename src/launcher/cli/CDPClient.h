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

#ifndef __StarfishCLICDPClient__
#define __StarfishCLICDPClient__

#include <string>
#include <cstdint>

namespace StarfishCLI {

// Synchronous WebSocket client for CDP.
// Not thread safe.  Call every method from the same thread.
class CDPClient {
public:
    CDPClient();
    ~CDPClient();

    bool connect(const std::string& host, uint16_t port, std::string& error);
    void disconnect();
    bool isConnected() const
    {
        return m_fd >= 0;
    }

    // Gives up a blocking call when the peer of this descriptor closes it.
    // The daemon passes the connection of the client that waits for the
    // answer. Pass -1 to stop watching.
    void setCancelDescriptor(int descriptor)
    {
        m_cancelDescriptor = descriptor;
    }

    // Send a CDP command and block until its response arrives.
    // paramsJson is a JSON object for "params".  Pass "" to send {}.
    // sessionId is the CDP session to use.  Pass "" to send none.
    // On success resultJson holds the "result" value, as a JSON string.
    // On failure it returns false and sets error.
    bool sendCommand(const std::string& method, const std::string& paramsJson,
                     std::string& resultJson, std::string& error,
                     const std::string& sessionId = "");

    // True when the last sendCommand failed because the engine answered
    // with a CDP error. False when it failed to send, timed out, or was
    // cancelled.
    bool lastErrorIsProtocolError() const
    {
        return m_lastErrorIsProtocol;
    }

    // Register the one event that may arrive before its command response.
    // All other events are discarded.
    void expectEvent(const std::string& eventMethod);

    // Wait for a CDP event notification (has "method" but no "id").
    // On success paramsJson holds the "params" value, as a JSON string.
    bool waitForEvent(const std::string& eventMethod, std::string& paramsJson,
                      int timeoutMs = 15000);

    struct PendingMsg {
        bool isResponse = false;
        int id = 0;
        std::string resultJson;
        std::string errorMsg;
        std::string method;     // for events
        std::string paramsJson; // for events
    };

    static PendingMsg parseFrame(const std::string& frame);

private:
    bool wsHandshake(std::string& error);
    bool sendWsFrame(const std::string& text);
    bool recvWsFrame(std::string& text, int timeoutMs);
    // Like recvWsFrame, but returns early with isCancelled set when the
    // cancel descriptor was closed by its peer.
    bool receiveFrame(std::string& text, int timeoutMs, bool& isCancelled);
    bool sendBytes(const void* buffer, size_t size);
    bool readBytes(void* buf, size_t n, int timeoutMs);

    int m_fd = -1;
    int m_nextId = 1;
    int m_cancelDescriptor = -1;
    std::string m_tcpBuf; // TCP receive buffer
    std::string m_expectedEventMethod;
    std::string m_pendingEventParameters;
    bool m_hasPendingEvent = false;
    bool m_lastErrorIsProtocol = false;
};

} // namespace StarfishCLI

#endif
