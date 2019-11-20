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
#include "core/modules/mediastream/RTCIceCandidate.h"
#include "core/modules/mediastream/RTCSctpTransport.h"
#include "core/modules/mediastream/RTCDataChannel.h"
#include "core/modules/mediastream/RTCRtpTransceiver.h"
#include "core/modules/mediastream/MediaStream.h"
#include "core/modules/mediastream/MediaStreamTrack.h"

#include "binding/DOMStringOrMediaStreamTrackUnion.h"

#include "api/peer_connection_interface.h"
#include "api/media_stream_interface.h"
#include "api/rtp_receiver_interface.h"

#include <EscargotPublic.h>

#define STARFISH_WEBRTC_DEBUG

#ifdef STARFISH_WEBRTC_DEBUG
#include <sys/types.h>
#include <sys/syscall.h>
#include <pthread.h>

#define WEBRTC_LOGI(STR, ...) \
    STARFISH_LOG_INFO(        \
        "[WEBRTC_LOG|%ld] "   \
        "" STR,               \
        syscall(SYS_gettid), ##__VA_ARGS__);
#define WEBRTC_LOGE(...) WEBRTC_LOGI(__VA_ARGS__)
#else
#define WEBRTC_LOGI(...)
#define WEBRTC_LOGE(...)
#endif

namespace Starfish {
class ExecutionContext;
class Event;
class RTCRtpSender;
class RTCPeerConnection;

// FIXME: The binding generator does not generate the following code, so they
// are manually included here. They are used in RTCPeerConnectionBinding.cpp
extern DOMStringOrMediaStreamTrack toDOMStringOrMediaStreamTrackFromValueRef(
    Escargot::ExecutionStateRef* state, Escargot::ValueRef* from);
extern Escargot::ValueRef* toValueRefFromDOMStringOrMediaStreamTrack(
    Escargot::ExecutionStateRef* state,
    const DOMStringOrMediaStreamTrack& from);

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

struct RTCDataChannelInit {
    DEFINE_GETTER_SETTER(bool, ordered, Ordered)
    DEFINE_GETTER_SETTER_WITH_HASFLAG(uint32_t, maxPacketLifeTime,
                                      MaxPacketLifeTime)
    DEFINE_GETTER_SETTER_WITH_HASFLAG(uint32_t, maxRetransmits, MaxRetransmits)
    DEFINE_GETTER_SETTER(String*, protocol, Protocol)
    DEFINE_GETTER_SETTER(bool, negotiated, Negotiated)
    DEFINE_GETTER_SETTER_WITH_HASFLAG(uint32_t, id, Id)

    bool m_ordered{ true };
    uint32_t m_maxPacketLifeTime;
    uint32_t m_maxRetransmits;
    String* m_protocol{ String::emptyString };
    bool m_negotiated{ false };
    uint32_t m_id;

    bool m_hasMaxPacketLifeTime{ false };
    bool m_hasMaxRetransmits{ false };
    bool m_hasId{ false };
};

class PeerConnectionObserver : public webrtc::PeerConnectionObserver {
public:
    const std::string m_stun = "stun:stun.l.google.com:19302";

    PeerConnectionObserver();
    PeerConnectionObserver(RTCPeerConnection* peerConnection);
    virtual ~PeerConnectionObserver(){};

    void OnSignalingChange(
        webrtc::PeerConnectionInterface::SignalingState new_state) override;
    void OnAddStream(
        rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
    void OnRemoveStream(
        rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override{};
    void OnAddTrack(
        rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
        const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
            streams) override{};
    void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface>
                     transceiver) override;
    void OnRemoveTrack(
        rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override{};
    void OnDataChannel(
        rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override{};
    void OnRenegotiationNeeded() override{};
    void OnIceConnectionChange(
        webrtc::PeerConnectionInterface::IceConnectionState new_state)
        override{};
    void OnStandardizedIceConnectionChange(
        webrtc::PeerConnectionInterface::IceConnectionState new_state)
        override{};
    void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState
                                new_state) override{};
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState
                                  new_state) override{};
    void OnIceCandidate(
        const webrtc::IceCandidateInterface* candidate) override;
    void OnIceCandidatesRemoved(
        const std::vector<cricket::Candidate>& candidates) override{};
    void OnIceConnectionReceivingChange(bool receiving) override{};
    void OnInterestingUsage(int usage_pattern) override{};

protected:
    // The pointer is always valid as: scope(PeerConnectionObserver) <=
    // scope(RTCPeerConnection)
    RTCPeerConnection* m_peerConnection;
};

class CreateOfferAnswerObserver
    : public webrtc::CreateSessionDescriptionObserver {
public:
    CreateOfferAnswerObserver(RTCPeerConnection* peerConnection)
        : m_peerConnection(peerConnection)
    {
    }

    virtual void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
    virtual void OnFailure(webrtc::RTCError error) override;

    virtual bool isCreateOffer()
    {
        return false;
    }

    virtual bool isCreateAnswer()
    {
        return false;
    }

protected:
    // The pointer is always valid as: scope(PeerConnectionObserver) <=
    // scope(RTCPeerConnection)
    RTCPeerConnection* m_peerConnection{ nullptr };
};

class CreateOfferObserver : public CreateOfferAnswerObserver {
public:
    CreateOfferObserver(RTCPeerConnection* peerConnection)
        : CreateOfferAnswerObserver(peerConnection)
    {
    }

    bool isCreateOffer() override
    {
        return true;
    }
};

class CreateAnswerObserver : public CreateOfferAnswerObserver {
public:
    CreateAnswerObserver(RTCPeerConnection* peerConnection)
        : CreateOfferAnswerObserver(peerConnection)
    {
    }

    bool isCreateAnswer() override
    {
        return true;
    }
};

class SetLocalRemoteDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
public:
    SetLocalRemoteDescriptionObserver(RTCPeerConnection* peerConnection)
        : m_peerConnection(peerConnection)
    {
    }

    virtual void OnSuccess() override;
    virtual void OnFailure(webrtc::RTCError error) override;

    virtual bool isLocalDescription()
    {
        return false;
    }

    virtual bool isRemoteDescription()
    {
        return false;
    }

protected:
    // The pointer is always valid as: scope(PeerConnectionObserver) <=
    // scope(RTCPeerConnection)
    RTCPeerConnection* m_peerConnection{ nullptr };
};

class SetLocalDescriptionObserver : public SetLocalRemoteDescriptionObserver {
public:
    SetLocalDescriptionObserver(RTCPeerConnection* peerConnection)
        : SetLocalRemoteDescriptionObserver(peerConnection)
    {
    }

    bool isLocalDescription() override
    {
        return true;
    }
};

class SetRemoteDescriptionObserver : public SetLocalRemoteDescriptionObserver {
public:
    SetRemoteDescriptionObserver(RTCPeerConnection* peerConnection)
        : SetLocalRemoteDescriptionObserver(peerConnection)
    {
    }

    bool isRemoteDescription() override
    {
        return true;
    }
};

template <typename T>
class PcObserver : public gc {
    friend class RTCPeerConnection;

public:
    PcObserver(RTCPeerConnection* peerConnection)
        : m_observer(new rtc::RefCountedObject<T>(peerConnection))
        , m_promise(nullptr)
    {
    }

    Promise* promise()
    {
        return m_promise;
    }

    void setPromise(Promise* promise)
    {
        m_promise = promise;
    }

private:
    rtc::scoped_refptr<T> m_observer;
    Promise* m_promise{ nullptr };
};

class RTCPeerConnection : public EventTarget {
    friend class PeerConnectionObserver;
    friend class CreateOfferAnswerObserver;
    friend class SetLocalRemoteDescriptionObserver;

public:
    const std::string m_stun = "stun:stun.l.google.com:19302";

    RTCPeerConnection(ExecutionContext* executionContext,
                      RTCConfiguration configuration = RTCConfiguration());
    virtual ~RTCPeerConnection();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(RTCPeerConnection)
    virtual ExecutionContext* executionContext() const override;

    Promise* createOffer(RTCOfferOptions options = RTCOfferOptions());
    Promise* createAnswer(RTCAnswerOptions options = RTCAnswerOptions());

    Promise* setLocalDescription(RTCSessionDescriptionInit& description);
    Promise* setRemoteDescription(RTCSessionDescriptionInit& description);

    NULLABLE RTCSessionDescription* localDescription();
    NULLABLE RTCSessionDescription* currentLocalDescription();
    NULLABLE RTCSessionDescription* pendingLocalDescription();

    NULLABLE RTCSessionDescription* remoteDescription();
    NULLABLE RTCSessionDescription* currentRemoteDescription();
    NULLABLE RTCSessionDescription* pendingRemoteDescription();

    Promise* addIceCandidate(
        RTCIceCandidateInit candidate = RTCIceCandidateInit());

    String* signalingState();
    String* iceGatheringState();
    String* iceConnectionState();
    String* connectionState();

    static GCVector<RTCIceServer> getDefaultIceServers();

    RTCConfiguration& getConfiguration();
    void setConfiguration(RTCConfiguration& configuration);

    void close();

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(negotiationneeded);
    DECLARE_EVENT_LISTENER(icecandidate);
    DECLARE_EVENT_LISTENER(icecandidateerror);
    DECLARE_EVENT_LISTENER(signalingstatechange);
    DECLARE_EVENT_LISTENER(iceconnectionstatechange);
    DECLARE_EVENT_LISTENER(icegatheringstatechange);
    DECLARE_EVENT_LISTENER(connectionstatechange);
    DECLARE_EVENT_LISTENER(datachannel);
#undef VIRTUAL
#undef OVERRIDE

    RTCSctpTransport* sctp();
    RTCDataChannel* createDataChannel(
        String* label,
        RTCDataChannelInit dataChannelDict = RTCDataChannelInit());

    GCVector<RTCRtpSender*> getSenders();
    GCVector<RTCRtpReceiver*> getReceivers();
    GCVector<RTCRtpTransceiver*> getTransceivers();

    RTCRtpSender* addTrack(MediaStreamTrack* track,
                           GCVector<MediaStream*>& streams);
    void removeTrack(RTCRtpSender* sender);
    RTCRtpTransceiver* addTransceiver(
        DOMStringOrMediaStreamTrack trackOrKind,
        RTCRtpTransceiverInit init = RTCRtpTransceiverInit());

    rtc::scoped_refptr<webrtc::PeerConnectionInterface> backend();
    bool initializePeerConnection();
    bool initializePeerConnection(RTCConfiguration& configuration);

    ScriptObject createSessionDescriptionInitObject(RTCSdpType type,
                                                    String* sdp);
    RTCSdpType toRtcSdpType(webrtc::SdpType type);
    webrtc::SdpType toSdpType(RTCSdpType type);
    DOMException* toDomException(webrtc::RTCError error);

private:
    ExecutionContext* m_executionContext;

    RTCConfiguration m_configuration;
    std::unique_ptr<PeerConnectionObserver> m_peerConnectionObserver;
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> m_backend;

    PcObserver<CreateOfferObserver>* m_createOfferObserver;
    PcObserver<CreateAnswerObserver>* m_createAnswerObserver;
    PcObserver<SetLocalDescriptionObserver>* m_setLocalDescriptionObserver;
    PcObserver<SetRemoteDescriptionObserver>* m_setRemoteDescriptionObserver;

    std::string m_lastCreatedOffer;
    std::string m_lastCreatedAnswer;

    bool isClosed();
    void deletePeerConnection();

    bool isValidRemoteState(RTCSdpType type);
    Promise* setRtcSessionDescription(RTCSessionDescriptionInit description,
                                      Promise* promise, bool isRemote);
};
}

#endif
#endif
