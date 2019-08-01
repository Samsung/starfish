/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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
/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#if defined(STARFISH_ENABLE_WEBRTC) && defined(STARFISH_ENABLE_TEST)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/mediastream/PeerConnectionClient.h"

#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/net_helpers.h"

#include "core/modules/mediastream/RTCPeerConnection.h"

namespace Starfish {

rtc::AsyncSocket* PeerConnectionClient::createClientSocket(int family)
{
    // Calling posix-specific API
    rtc::Thread* thread = TestPeerConnectionObserver::socketThread();
    STARFISH_ASSERT(thread != NULL);
    return thread->socketserver()->CreateAsyncSocket(family, SOCK_STREAM);
}

PeerConnectionClient::PeerConnectionClient()
{
}

PeerConnectionClient::~PeerConnectionClient()
{
}

void PeerConnectionClient::initSocketSignals()
{
    STARFISH_ASSERT(m_controlSocket.get() != NULL);
    STARFISH_ASSERT(m_hangingGet.get() != NULL);
    m_controlSocket->SignalCloseEvent.connect(this,
                                              &PeerConnectionClient::onClose);
    m_hangingGet->SignalCloseEvent.connect(this,
                                           &PeerConnectionClient::onClose);
    m_controlSocket->SignalConnectEvent.connect(
        this, &PeerConnectionClient::onConnect);
    m_hangingGet->SignalConnectEvent.connect(
        this, &PeerConnectionClient::onHangingGetConnect);
    m_controlSocket->SignalReadEvent.connect(this,
                                             &PeerConnectionClient::onRead);
    m_hangingGet->SignalReadEvent.connect(
        this, &PeerConnectionClient::onHangingGetRead);
}

int PeerConnectionClient::id() const
{
    return m_myId;
}

bool PeerConnectionClient::isConnected() const
{
    return m_myId != -1;
}

const Peers& PeerConnectionClient::peers() const
{
    return m_peers;
}

void PeerConnectionClient::registerObserver(
    PeerConnectionClientObserver* callback)
{
    STARFISH_ASSERT(!m_callback);
    m_callback = callback;
}

void PeerConnectionClient::connect(const std::string& server, int port,
                                   const std::string& clientName)
{
    if (m_state != NOT_CONNECTED) {
        STARFISH_LOG_WARN(
            "The client must not be connected before you can call Connect()\n");
        m_callback->OnServerConnectionFailure();
        return;
    }

    if (server.empty() || clientName.empty()) {
        m_callback->OnServerConnectionFailure();
        return;
    }

    if (port <= 0) {
        port = m_defaultServerPort;
    }

    m_serverAddress.SetIP(server);
    m_serverAddress.SetPort(port);
    m_clientName = clientName;

    if (m_serverAddress.IsUnresolvedIP()) {
        m_state = RESOLVING;
        m_resolver =
            std::unique_ptr<rtc::AsyncResolver>(new rtc::AsyncResolver());
        m_resolver->SignalDone.connect(this,
                                       &PeerConnectionClient::onResolveResult);
        m_resolver->Start(m_serverAddress);
    } else {
        doConnect();
    }
}

void PeerConnectionClient::onResolveResult(
    rtc::AsyncResolverInterface* resolver)
{
    if (m_resolver->GetError() != 0) {
        m_callback->OnServerConnectionFailure();
        m_resolver->Destroy(false);
        m_resolver.reset(nullptr);
        m_state = NOT_CONNECTED;
    } else {
        m_serverAddress = m_resolver->address();
        doConnect();
    }
}

void PeerConnectionClient::doConnect()
{
    m_controlSocket.reset(
        createClientSocket(m_serverAddress.ipaddr().family()));
    m_hangingGet.reset(createClientSocket(m_serverAddress.ipaddr().family()));
    initSocketSignals();
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "GET /sign_in?%s HTTP/1.0\r\n\r\n",
             m_clientName.c_str());
    m_onconnectData = buffer;

    bool ret = connectControlSocket();
    if (ret) {
        m_state = SIGNING_IN;
    } else {
        m_callback->OnServerConnectionFailure();
    }
}

bool PeerConnectionClient::sendToPeer(int peerId, const std::string& message)
{
    if (m_state != CONNECTED) {
        return false;
    }

    STARFISH_ASSERT(isConnected());
    STARFISH_ASSERT(m_controlSocket->GetState() == rtc::Socket::CS_CLOSED);
    if (!isConnected() || peerId == -1) {
        return false;
    }

    char headers[1024];
    snprintf(headers, sizeof(headers),
             "POST /message?peer_id=%i&to=%i HTTP/1.0\r\n"
             "Content-Length: %zu\r\n"
             "Content-Type: text/plain\r\n"
             "\r\n",
             m_myId, peerId, message.length());
    m_onconnectData = headers;
    m_onconnectData += message;
    return connectControlSocket();
}

bool PeerConnectionClient::sendHangUp(int peerId)
{
    return sendToPeer(peerId, PeerConnectionClient::m_byeMessage);
}

bool PeerConnectionClient::isSendingMessage()
{
    return m_state == CONNECTED &&
           m_controlSocket->GetState() != rtc::Socket::CS_CLOSED;
}

bool PeerConnectionClient::signOut()
{
    if (m_state == NOT_CONNECTED || m_state == SIGNING_OUT) {
        return true;
    }

    if (m_hangingGet->GetState() != rtc::Socket::CS_CLOSED) {
        m_hangingGet->Close();
    }

    if (m_controlSocket->GetState() == rtc::Socket::CS_CLOSED) {
        m_state = SIGNING_OUT;

        if (m_myId != -1) {
            char buffer[1024];
            snprintf(buffer, sizeof(buffer),
                     "GET /sign_out?peer_id=%i HTTP/1.0\r\n\r\n", m_myId);
            m_onconnectData = buffer;
            return connectControlSocket();
        } else {
            // Can occur if the app is closed before we finish connecting.
            return true;
        }
    } else {
        m_state = SIGNING_OUT_WAITING;
    }

    return true;
}

void PeerConnectionClient::close()
{
    m_controlSocket->Close();
    m_hangingGet->Close();
    m_onconnectData.clear();
    m_peers.clear();
    if (m_resolver != nullptr) {
        m_resolver->Destroy(false);
        m_resolver.reset(nullptr);
    }
    m_myId = -1;
    m_state = NOT_CONNECTED;
}

bool PeerConnectionClient::connectControlSocket()
{
    STARFISH_ASSERT(m_controlSocket->GetState() == rtc::Socket::CS_CLOSED);
    int err = m_controlSocket->Connect(m_serverAddress);
    if (err == SOCKET_ERROR) {
        close();
        return false;
    }
    return true;
}

void PeerConnectionClient::onConnect(rtc::AsyncSocket* socket)
{
    STARFISH_ASSERT(!m_onconnectData.empty());
    size_t sent =
        socket->Send(m_onconnectData.c_str(), m_onconnectData.length());
    STARFISH_ASSERT(sent == m_onconnectData.length());
    m_onconnectData.clear();
}

void PeerConnectionClient::onHangingGetConnect(rtc::AsyncSocket* socket)
{
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "GET /wait?peer_id=%i HTTP/1.0\r\n\r\n",
             m_myId);
    int len = static_cast<int>(strlen(buffer));
    int sent = socket->Send(buffer, len);
    STARFISH_ASSERT(sent == len);
}

void PeerConnectionClient::onMessageFromPeer(int peerId,
                                             const std::string& message)
{
    if (message.length() == (sizeof(m_byeMessage) - 1) &&
        message.compare(m_byeMessage) == 0) {
        m_callback->OnPeerDisconnected(peerId);
    } else {
        m_callback->OnMessageFromPeer(peerId, message);
    }
}

bool PeerConnectionClient::getHeaderValue(const std::string& data, size_t eoh,
                                          const char* headerPattern,
                                          size_t* value)
{
    STARFISH_ASSERT(value != NULL);
    size_t found = data.find(headerPattern);
    if (found != std::string::npos && found < eoh) {
        *value = atoi(&data[found + strlen(headerPattern)]);
        return true;
    }
    return false;
}

bool PeerConnectionClient::getHeaderValue(const std::string& data, size_t eoh,
                                          const char* headerPattern,
                                          std::string* value)
{
    STARFISH_ASSERT(value != NULL);
    size_t found = data.find(headerPattern);
    if (found != std::string::npos && found < eoh) {
        size_t begin = found + strlen(headerPattern);
        size_t end = data.find("\r\n", begin);
        if (end == std::string::npos) {
            end = eoh;
        }
        value->assign(data.substr(begin, end - begin));
        return true;
    }
    return false;
}

bool PeerConnectionClient::readIntoBuffer(rtc::AsyncSocket* socket,
                                          std::string* data,
                                          size_t* contentLength)
{
    char buffer[0xffff];
    do {
        int bytes = socket->Recv(buffer, sizeof(buffer), nullptr);
        if (bytes <= 0) {
            break;
        }
        data->append(buffer, bytes);
    } while (true);

    bool ret = false;
    size_t i = data->find("\r\n\r\n");
    if (i != std::string::npos) {
        STARFISH_LOG_INFO("Headers received\n");
        if (getHeaderValue(*data, i, "\r\nContent-Length: ", contentLength)) {
            size_t totalResponseSize = (i + 4) + *contentLength;
            if (data->length() >= totalResponseSize) {
                ret = true;
                std::string shouldClose;
                const char kConnection[] = "\r\nConnection: ";
                if (getHeaderValue(*data, i, kConnection, &shouldClose) &&
                    shouldClose.compare("close") == 0) {
                    socket->Close();
                    // Since we closed the socket, there was no notification
                    // delivered to us.  Compensate by letting ourselves know.
                    onClose(socket, 0);
                }
            } else {
                // We haven't received everything.  Just continue to accept
                // data.
            }
        } else {
            STARFISH_LOG_ERROR(
                "No content length field specified by the server.\n");
        }
    }
    return ret;
}

void PeerConnectionClient::onRead(rtc::AsyncSocket* socket)
{
    size_t contentLength = 0;
    if (readIntoBuffer(socket, &m_controlData, &contentLength)) {
        size_t peerId = 0, eoh = 0;
        bool ok =
            parseServerResponse(m_controlData, contentLength, &peerId, &eoh);
        if (ok) {
            if (m_myId == -1) {
                // First response.  Let's store our server assigned ID.
                STARFISH_ASSERT(m_state == SIGNING_IN);
                m_myId = static_cast<int>(peerId);
                STARFISH_ASSERT(m_myId != -1);

                // The body of the response will be a list of already connected
                // peers.
                if (contentLength) {
                    size_t pos = eoh + 4;
                    while (pos < m_controlData.size()) {
                        size_t eol = m_controlData.find('\n', pos);
                        if (eol == std::string::npos)
                            break;
                        int id = 0;
                        std::string name;
                        bool connected;
                        if (parseEntry(m_controlData.substr(pos, eol - pos),
                                       &name, &id, &connected) &&
                            id != m_myId) {
                            m_peers[id] = name;
                            m_callback->OnPeerConnected(id, name);
                        }
                        pos = eol + 1;
                    }
                }
                STARFISH_ASSERT(isConnected());
                m_callback->OnSignedIn();
            } else if (m_state == SIGNING_OUT) {
                close();
                m_callback->OnDisconnected();
            } else if (m_state == SIGNING_OUT_WAITING) {
                signOut();
            }
        }

        m_controlData.clear();

        if (m_state == SIGNING_IN) {
            STARFISH_ASSERT(m_hangingGet->GetState() == rtc::Socket::CS_CLOSED);
            m_state = CONNECTED;
            m_hangingGet->Connect(m_serverAddress);
        }
    }
}

void PeerConnectionClient::onHangingGetRead(rtc::AsyncSocket* socket)
{
    STARFISH_LOG_INFO("%s\n", __func__);
    size_t contentLength = 0;
    if (readIntoBuffer(socket, &m_notificationData, &contentLength)) {
        size_t peerId = 0, eoh = 0;
        bool ok = parseServerResponse(m_notificationData, contentLength,
                                      &peerId, &eoh);

        if (ok) {
            // Store the position where the body begins.
            size_t pos = eoh + 4;

            if (m_myId == static_cast<int>(peerId)) {
                // A notification about a new member or a member that just
                // disconnected.
                int id = 0;
                std::string name;
                bool connected = false;
                if (parseEntry(m_notificationData.substr(pos), &name, &id,
                               &connected)) {
                    if (connected) {
                        m_peers[id] = name;
                        m_callback->OnPeerConnected(id, name);
                    } else {
                        m_peers.erase(id);
                        m_callback->OnPeerDisconnected(id);
                    }
                }
            } else {
                onMessageFromPeer(static_cast<int>(peerId),
                                  m_notificationData.substr(pos));
            }
        }

        m_notificationData.clear();
    }

    if (m_hangingGet->GetState() == rtc::Socket::CS_CLOSED &&
        m_state == CONNECTED) {
        m_hangingGet->Connect(m_serverAddress);
    }
}

bool PeerConnectionClient::parseEntry(const std::string& entry,
                                      std::string* name, int* id,
                                      bool* connected)
{
    STARFISH_ASSERT(name != nullptr);
    STARFISH_ASSERT(id != nullptr);
    STARFISH_ASSERT(connected != nullptr);
    STARFISH_ASSERT(!entry.empty());

    *connected = false;
    size_t separator = entry.find(',');
    if (separator != std::string::npos) {
        *id = atoi(&entry[separator + 1]);
        name->assign(entry.substr(0, separator));
        separator = entry.find(',', separator + 1);
        if (separator != std::string::npos) {
            *connected = atoi(&entry[separator + 1]) ? true : false;
        }
    }
    return !name->empty();
}

int PeerConnectionClient::getResponseStatus(const std::string& response)
{
    int status = -1;
    size_t pos = response.find(' ');
    if (pos != std::string::npos) {
        status = atoi(&response[pos + 1]);
    }
    return status;
}

bool PeerConnectionClient::parseServerResponse(const std::string& response,
                                               size_t contentLength,
                                               size_t* peerId, size_t* eoh)
{
    int status = getResponseStatus(response.c_str());
    if (status != 200) {
        STARFISH_LOG_ERROR("Received error from server\n");
        close();
        m_callback->OnDisconnected();
        return false;
    }

    *eoh = response.find("\r\n\r\n");
    STARFISH_ASSERT(*eoh != std::string::npos);
    if (*eoh == std::string::npos) {
        return false;
    }

    *peerId = -1;

    // See comment in peer_channel.cc for why we use the Pragma header and
    // not e.g. "X-Peer-Id".
    getHeaderValue(response, *eoh, "\r\nPragma: ", peerId);

    return true;
}

void PeerConnectionClient::onClose(rtc::AsyncSocket* socket, int err)
{
    STARFISH_LOG_INFO("%s\n", __func__);

    socket->Close();

    if (err != ECONNREFUSED) { // WSAECONNREFUSED when WIN32
        if (socket == m_hangingGet.get()) {
            if (m_state == CONNECTED) {
                m_hangingGet->Close();
                m_hangingGet->Connect(m_serverAddress);
            }
        } else {
            m_callback->OnMessageSent(err);
        }
    } else {
        if (socket == m_controlSocket.get()) {
            STARFISH_LOG_WARN("Connection refused; retrying in 2 seconds\n");
            rtc::Thread::Current()->PostDelayed(RTC_FROM_HERE, m_reconnectDelay,
                                                this, 0);
        } else {
            close();
            m_callback->OnDisconnected();
        }
    }
}

void PeerConnectionClient::OnMessage(rtc::Message* msg)
{
    // ignore msg; there is currently only one supported message ("retry")
    doConnect();
}
} // namespace Starfish

#endif
