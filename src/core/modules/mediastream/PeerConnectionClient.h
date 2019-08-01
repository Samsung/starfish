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
 *  Copyright 2011 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#if defined(STARFISH_ENABLE_WEBRTC) && defined(STARFISH_ENABLE_TEST)

#ifndef __StarfishPeerConnectionClient__
#define __StarfishPeerConnectionClient__

#include <map>
#include <memory>
#include <string>

#include "rtc_base/net_helpers.h"
#include "rtc_base/physical_socket_server.h"
#include "rtc_base/signal_thread.h"
#include "rtc_base/third_party/sigslot/sigslot.h"
#include "rtc_base/arraysize.h"

namespace Starfish {

typedef std::map<int, std::string> Peers;

class PeerConnectionClientObserver {
public:
    virtual void OnSignedIn() = 0;
    virtual void OnDisconnected() = 0;
    virtual void OnPeerConnected(int id, const std::string& name) = 0;
    virtual void OnPeerDisconnected(int peer_id) = 0;
    virtual void OnMessageFromPeer(int peer_id, const std::string& message) = 0;
    virtual void OnMessageSent(int err) = 0;
    virtual void OnServerConnectionFailure() = 0;

protected:
    virtual ~PeerConnectionClientObserver(){};
};

class PeerConnectionClient : public sigslot::has_slots<>,
                             public rtc::MessageHandler {
public:
    const std::string kAudioLabel = "audio_label";
    const std::string kVideoLabel = "video_label";
    const std::string kStreamId = "stream_id";
    const uint16_t m_defaultServerPort = 8888;
    const std::string m_stun = "stun:stun.l.google.com:19302";
    const std::string m_server = "localhost";
    const std::string m_username = "user";

    const std::string m_byeMessage = "BYE";
    // Delay between server connection retries, in milliseconds
    const int m_reconnectDelay = 2000;

    enum State {
        NOT_CONNECTED,
        RESOLVING,
        SIGNING_IN,
        CONNECTED,
        SIGNING_OUT_WAITING,
        SIGNING_OUT,
    };

    PeerConnectionClient();
    ~PeerConnectionClient();

    int id() const;
    bool isConnected() const;
    const Peers& peers() const;

    void registerObserver(PeerConnectionClientObserver* callback);

    void connect(const std::string& server, int port,
                 const std::string& client_name);

    bool sendToPeer(int peer_id, const std::string& message);
    bool sendHangUp(int peer_id);
    bool isSendingMessage();

    bool signOut();

    // implements the MessageHandler interface
    void OnMessage(rtc::Message* msg) override;

    std::string peerConnectionString()
    {
        return m_stun;
    }

    std::string defaultServerName()
    {
        return m_server;
    }

    std::string peerName()
    {
        return m_username;
    }

private:
    rtc::AsyncSocket* createClientSocket(int family);
    void doConnect();
    void close();
    void initSocketSignals();
    bool connectControlSocket();
    void onConnect(rtc::AsyncSocket* socket);
    void onHangingGetConnect(rtc::AsyncSocket* socket);
    void onMessageFromPeer(int peer_id, const std::string& message);

    // Quick and dirty support for parsing HTTP header values.
    bool getHeaderValue(const std::string& data, size_t eoh,
                        const char* header_pattern, size_t* value);

    bool getHeaderValue(const std::string& data, size_t eoh,
                        const char* header_pattern, std::string* value);

    // Returns true if the whole response has been read.
    bool readIntoBuffer(rtc::AsyncSocket* socket, std::string* data,
                        size_t* content_length);

    void onRead(rtc::AsyncSocket* socket);

    void onHangingGetRead(rtc::AsyncSocket* socket);

    // Parses a single line entry in the form "<name>,<id>,<connected>"
    bool parseEntry(const std::string& entry, std::string* name, int* id,
                    bool* connected);

    int getResponseStatus(const std::string& response);

    bool parseServerResponse(const std::string& response, size_t content_length,
                             size_t* peer_id, size_t* eoh);

    void onClose(rtc::AsyncSocket* socket, int err);

    void onResolveResult(rtc::AsyncResolverInterface* resolver);

    PeerConnectionClientObserver* m_callback{ nullptr };
    rtc::SocketAddress m_serverAddress;
    std::unique_ptr<rtc::AsyncResolver> m_resolver;
    std::unique_ptr<rtc::AsyncSocket> m_controlSocket;
    std::unique_ptr<rtc::AsyncSocket> m_hangingGet;
    std::string m_onconnectData;
    std::string m_controlData;
    std::string m_notificationData;
    std::string m_clientName;
    Peers m_peers;
    State m_state{ NOT_CONNECTED };
    int m_myId{ -1 };
};
} // namespace Starfish

#endif
#endif
