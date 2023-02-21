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

#if defined(STARFISH_ENABLE_WEBRTC)

#ifndef __StarfishRTCDataChannel__
#define __StarfishRTCDataChannel__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "rtc_data_channel.h"

namespace Starfish {
class RTCDataChannel;
class WebRtcManager;

struct RTCDataChannelInit {
    DEFINE_GETTER_SETTER(bool, ordered, Ordered)
    DEFINE_GETTER_SETTER_WITH_HASFLAG(uint32_t, maxPacketLifeTime,
                                      MaxPacketLifeTime)
    DEFINE_GETTER_SETTER_WITH_HASFLAG(uint32_t, maxRetransmits, MaxRetransmits)
    DEFINE_GETTER_SETTER(String*, protocol, Protocol)
    DEFINE_GETTER_SETTER(bool, negotiated, Negotiated)
    DEFINE_GETTER_SETTER_WITH_HASFLAG(uint32_t, id, Id)

    bool m_ordered{ true };
    uint32_t m_maxPacketLifeTime{ 0 };
    uint32_t m_maxRetransmits{ 0 };
    String* m_protocol{ String::emptyString };
    bool m_negotiated{ false };
    uint32_t m_id{ 0 };

    bool m_hasMaxPacketLifeTime{ false };
    bool m_hasMaxRetransmits{ false };
    bool m_hasId{ false };
};

class RTCDataChannelObserver : public gc,
                               public libwebrtc::RTCDataChannelObserver {
    friend class RTCDataChannel;
    friend class RTCPeerConnection;

public:
    RTCDataChannelObserver(RTCDataChannel* dataChannel);
    virtual ~RTCDataChannelObserver();
    void OnStateChange(libwebrtc::RTCDataChannelState state) override;
    void OnMessage(const char* buffer, int length, bool binary) override;

private:
    RTCDataChannel* m_dataChannel{ nullptr };
};

class RTCDataChannel : public EventTarget {
    friend class RTCDataChannelObserver;
    friend class RTCPeerConnection;

public:
    RTCDataChannel(
        ExecutionContext* executionContext, RTCPeerConnection* peerConnection,
        RTCDataChannelInit init,
        libwebrtc::scoped_refptr<libwebrtc::RTCDataChannel> rpcSctpTransport);
    virtual ~RTCDataChannel();
    void dispose();

    virtual ExecutionContext* executionContext() const;
    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(RTCDataChannel)

    String* label();
    bool ordered();
    Nullable<uint32_t> maxPacketLifeTime();
    Nullable<uint32_t> maxRetransmits();
    DEFINE_GETTER(String*, protocol);
    bool negotiated();
    Nullable<uint32_t> id();
    DEFINE_GETTER(String*, priority);
    String* readyState();

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(open);
    DECLARE_EVENT_LISTENER(bufferedamountlow);
    DECLARE_EVENT_LISTENER(error);
    DECLARE_EVENT_LISTENER(closing);
    DECLARE_EVENT_LISTENER(close);
    DECLARE_EVENT_LISTENER(message);
#undef VIRTUAL
#undef OVERRIDE

    String* binaryType();
    void setBinaryType(String* binaryType);

    void send(String* data);

    libwebrtc::scoped_refptr<libwebrtc::RTCDataChannel> backend()
    {
        return m_backend;
    }

private:
    ExecutionContext* m_executionContext{ nullptr };
    WebRtcManager* m_webRtcManager{ nullptr };
    RTCPeerConnection* m_peerConnection{ nullptr };
    RTCDataChannelObserver* m_observer{ nullptr };

    String* m_protocol{ String::emptyString };
    String* m_priority{ String::createASCIIString("low") };
    String* m_binaryType{ String::createASCIIString("blob") };

    libwebrtc::scoped_refptr<libwebrtc::RTCDataChannel> m_backend;
};
} // namespace Starfish
#endif
#endif
