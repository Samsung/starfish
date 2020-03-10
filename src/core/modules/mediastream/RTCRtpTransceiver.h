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

#ifndef __StarfishRTCRtpTransceiver__
#define __StarfishRTCRtpTransceiver__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "api/peer_connection_interface.h"
#include "api/rtp_transceiver_interface.h"

namespace Starfish {

enum RTCRtpTransceiverDirection {
    Sendrecv,
    Sendonly,
    Recvonly,
    Inactive,
    Stopped
};

struct RTCRtpTransceiverInit {
    DEFINE_GETTER_SETTER(String*, direction, Direction);
    webrtc::RtpTransceiverInit toRtpTransceiverInit();

    String* m_direction{ String::createASCIIString("sendrecv") };
};

class RTCRtpSender;
class RTCRtpReceiver;
class RTCRtpTransceiver : public ScriptWrappable {
public:
    RTCRtpTransceiver(
        ExecutionContext* executionContext, RTCPeerConnection* peerConnection,
        rtc::scoped_refptr<webrtc::RtpTransceiverInterface> rptTransceiver);
    virtual ~RTCRtpTransceiver();
    void dispose();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(RTCRtpTransceiver)

    String* mid();
    RTCRtpSender* sender();
    RTCRtpReceiver* receiver();

    RTCRtpTransceiverDirection direction();
    void setDirection(RTCRtpTransceiverDirection direction);

    String* directionStr();
    void setDirectionStr(String* direction);
    Nullable<String*> currentDirection();

    bool stopped();
    bool sentBefore();

    RTCPeerConnection* peerConnection()
    {
        return m_peerConnection;
    }

    bool canSend();

    rtc::scoped_refptr<webrtc::RtpTransceiverInterface> backend()
    {
        return m_backend;
    }

private:
    ExecutionContext* m_executionContext{ nullptr };
    RTCPeerConnection* m_peerConnection{ nullptr };
    rtc::scoped_refptr<webrtc::RtpTransceiverInterface> m_backend;

    RTCRtpSender* m_sender{ nullptr };
    RTCRtpReceiver* m_receiver{ nullptr };
    bool m_sentBefore{ false };
};
} // namespace Starfish
#endif
#endif
