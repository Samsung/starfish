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

#if defined(STARFISH_ENABLE_WEBRTC)

#ifndef __StarfishRTCPeerConnection__
#define __StarfishRTCPeerConnection__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "core/modules/mediastream/RTCConfiguration.h"
#include "core/modules/mediastream/RTCSessionDescription.h"

namespace Starfish {
class ExecutionContext;
class OperationQueue;

enum class RTCSignalingState {
    Stable,
    HaveLocalOffer,
    HaveRemoteOffer,
    HaveLocalPranswer,
    HaveRemotePranswer,
    Closed
};

enum class RTCIceGatheringState {
    New,
    Gathering,
    Complete,
};

enum class RTCPeerConnectionState {
    Closed,
    Failed,
    Disconnected,
    New,
    Connecting,
    Connected
};

enum class RTCIceConnectionState {
    Closed,
    Failed,
    Disconnected,
    New,
    Checking,
    Completed,
    Connected
};

struct RTCOfferAnswerOptions {
    DEFINE_GETTER_SETTER(bool, voiceActivityDetection, VoiceActivityDetection)

    bool m_voiceActivityDetection{ true };
};

struct RTCOfferOptions : public RTCOfferAnswerOptions {
    DEFINE_GETTER_SETTER(bool, iceRestart, IceRestart)
    DEFINE_GETTER_SETTER(bool, offerToReceiveAudio, OfferToReceiveAudio)
    DEFINE_GETTER_SETTER(bool, offerToReceiveVideo, OfferToReceiveVideo)

    bool m_iceRestart{ false };
    bool m_offerToReceiveAudio{ false };
    bool m_offerToReceiveVideo{ false };
};

struct RTCAnswerOptions : public RTCOfferAnswerOptions {
};

class RTCPeerConnection : public EventTarget {
public:
    RTCPeerConnection(ExecutionContext* executionContext,
                      RTCConfiguration configuration = RTCConfiguration());
    virtual ~RTCPeerConnection();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(RTCPeerConnection)
    virtual ExecutionContext* executionContext() const override;

    Promise* createOffer(RTCOfferOptions options = RTCOfferOptions());
    Promise* setLocalDescription(RTCSessionDescriptionInit& description);

    NULLABLE RTCSessionDescription* localDescription();
    NULLABLE DEFINE_GETTER(RTCSessionDescription*, currentLocalDescription);
    NULLABLE DEFINE_GETTER(RTCSessionDescription*, pendingLocalDescription);

    NULLABLE RTCSessionDescription* remoteDescription();
    NULLABLE DEFINE_GETTER(RTCSessionDescription*, currentRemoteDescription);
    NULLABLE DEFINE_GETTER(RTCSessionDescription*, pendingRemoteDescription);

    String* signalingState();
    String* iceGatheringState();
    String* iceConnectionState();
    String* connectionState();

    RTCConfiguration& getConfiguration();
    void setConfiguration(RTCConfiguration& configuration);

    void close();

private:
    ScriptObject createSessionDescriptionInitObject(RTCSdpType type,
                                                    String* sdp);

    ExecutionContext* m_executionContext;

    OperationQueue* m_operationQueue;

    RTCSessionDescription* m_localDescription{ nullptr };
    RTCSessionDescription* m_currentLocalDescription{ nullptr };
    RTCSessionDescription* m_pendingLocalDescription{ nullptr };

    RTCSessionDescription* m_remoteDescription{ nullptr };
    RTCSessionDescription* m_currentRemoteDescription{ nullptr };
    RTCSessionDescription* m_pendingRemoteDescription{ nullptr };

    RTCSignalingState m_signalingState{ RTCSignalingState::Stable };
    RTCIceGatheringState m_iceGatheringState{ RTCIceGatheringState::New };
    RTCIceConnectionState m_iceConnectionState{ RTCIceConnectionState::New };
    RTCPeerConnectionState m_connectionState{ RTCPeerConnectionState::New };

    RTCConfiguration m_configuration;
    bool m_isClosed{ false };
    bool m_negotiationNeeded{ false };

    String* m_lastCreatedOffer{ String::emptyString };
    String* m_lastCreatedAnswer{ String::emptyString };
};
}

#endif
#endif
