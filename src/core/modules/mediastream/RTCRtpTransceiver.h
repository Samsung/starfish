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

#include "binding/ScriptWrappable.h"
#include "core/modules/mediastream/RTCRtpTransceiverInit.h"

#include "rtc_rtp_transceiver.h"

namespace Starfish {

class RTCRtpSender;
class RTCRtpReceiver;

class RTCRtpTransceiver : public ScriptWrappable {
public:
    RTCRtpTransceiver(
        ExecutionContext* executionContext, RTCPeerConnection* peerConnection,
        libwebrtc::scoped_refptr<libwebrtc::RTCRtpTransceiver> rptTransceiver);

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

    Optional<String*> currentDirection();

    bool stopped();

    bool stopping();

    bool sentBefore();

    void MarkSentBefore()
    {
        m_sentBefore = true;
    }

    void MarkRepresentedInRemoteDescription()
    {
        m_representedInLocalDescription = true;
    }

    void MarkRepresentedInLocalDescription()
    {
        m_representedInRemoteDescription = true;
    }

    RTCPeerConnection* peerConnection()
    {
        return m_peerConnection;
    }

    bool canSend();

    void stop();

    libwebrtc::scoped_refptr<libwebrtc::RTCRtpTransceiver> backend()
    {
        return m_backend;
    }

private:
    ExecutionContext* m_executionContext = nullptr;
    RTCPeerConnection* m_peerConnection = nullptr;
    libwebrtc::scoped_refptr<libwebrtc::RTCRtpTransceiver> m_backend;

    RTCRtpSender* m_sender = nullptr;
    RTCRtpReceiver* m_receiver = nullptr;
    bool m_sentBefore = false;
    bool m_representedInLocalDescription = false;
    bool m_representedInRemoteDescription = false;
};

} // namespace Starfish

#endif
#endif
