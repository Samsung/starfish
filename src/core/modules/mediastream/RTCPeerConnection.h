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
#include "core/modules/mediastream/PeerConnectionClient.h"

#include "api/peer_connection_interface.h"
#include "api/media_stream_interface.h"
#include "api/rtp_receiver_interface.h"

namespace Starfish {
class ExecutionContext;
class Event;
class OperationQueue;
class RTCRtpSender;
class RTCPeerConnection;

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

// FIXME: remove webrtc::CreateSessionDescriptionObserver after removing
// TestPeerConnectionObserver
class PeerConnectionObserver : public webrtc::PeerConnectionObserver,
                               public webrtc::CreateSessionDescriptionObserver {
public:
    const std::string m_stun = "stun:stun.l.google.com:19302";

    PeerConnectionObserver();
    PeerConnectionObserver(RTCPeerConnection* peerConnection);
    virtual ~PeerConnectionObserver(){};

    void OnSignalingChange(
        webrtc::PeerConnectionInterface::SignalingState new_state) override{};
    void OnAddStream(
        rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override{};
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
        const webrtc::IceCandidateInterface* candidate) override{};
    void OnIceCandidatesRemoved(
        const std::vector<cricket::Candidate>& candidates) override{};
    void OnIceConnectionReceivingChange(bool receiving) override{};
    void OnInterestingUsage(int usage_pattern) override{};

    // CreateSessionDescriptionObserver implementation.
    void OnSuccess(webrtc::SessionDescriptionInterface* desc) override{};
    void OnFailure(webrtc::RTCError error) override{};

protected:
    RTCPeerConnection* m_peerConnection;
};

class CreateSessionDescriptionObserver
    : public webrtc::CreateSessionDescriptionObserver {
public:
    static CreateSessionDescriptionObserver* create(
        RTCPeerConnection* peerConnection, Promise* promise);

    void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
    void OnFailure(webrtc::RTCError error) override;
    void setPromise(Promise* promise)
    {
        m_promise = promise;
    }

private:
    RTCPeerConnection* m_peerConnection{ nullptr };
    Promise* m_promise{ nullptr };
};

class SetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
public:
    static SetSessionDescriptionObserver* create(
        RTCPeerConnection* peerConnection, Promise* promise);

    virtual void OnSuccess() override;
    virtual void OnFailure(webrtc::RTCError error) override;
    void setPromise(Promise* promise)
    {
        m_promise = promise;
    }

private:
    RTCPeerConnection* m_peerConnection{ nullptr };
    Promise* m_promise{ nullptr };
};

#if defined(STARFISH_ENABLE_TEST)
class TestPeerConnectionObserver : public PeerConnectionObserver,
                                   public PeerConnectionClientObserver {
public:
    // Names used for a IceCandidate JSON object.
    const std::string kCandidateSdpMidName = "sdpMid";
    const std::string kCandidateSdpMlineIndexName = "sdpMLineIndex";
    const std::string kCandidateSdpName = "candidate";

    // Names used for a SessionDescription JSON object.
    const std::string kSessionDescriptionTypeName = "type";
    const std::string kSessionDescriptionSdpName = "sdp";

    TestPeerConnectionObserver();
    TestPeerConnectionObserver(RTCPeerConnection* peerConnection);
    virtual ~TestPeerConnectionObserver(){};

    void OnSignedIn() override;
    void OnDisconnected() override;
    void OnPeerConnected(int id, const std::string& name) override;
    void OnPeerDisconnected(int id) override;
    void OnMessageFromPeer(int peer_id, const std::string& message) override;
    void OnMessageSent(int err) override;
    void OnServerConnectionFailure() override;

    // CreateSessionDescriptionObserver implementation.
    void OnSuccess(webrtc::SessionDescriptionInterface* desc) override{};
    void OnFailure(webrtc::RTCError error) override{};

    static void* runSocketServer(void* arg);
    static rtc::Thread* socketThread()
    {
        return m_socketThread;
    }

private:
    static rtc::Thread* m_socketThread;
    std::unique_ptr<PeerConnectionClient> m_client =
        std::unique_ptr<PeerConnectionClient>(new PeerConnectionClient());
    int m_peerId{ -1 };
    std::string m_server;
    bool m_loopback{ false };

    bool reinitializePeerConnectionForLoopback();
    void startLogin(const std::string& server, int port);
    void deletePeerConnection();
};
#endif

class RTCPeerConnection : public EventTarget {
    friend class PeerConnectionObserver;
    friend class TestPeerConnectionObserver;

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

    String* signalingState();
    String* iceGatheringState();
    String* iceConnectionState();
    String* connectionState();

    RTCConfiguration& getConfiguration();
    void setConfiguration(RTCConfiguration& configuration);

    void close();

    GCVector<RTCRtpSender*> getSenders();
    RTCRtpSender* addTrack(MediaStreamTrack* track,
                           GCVector<MediaStream*>& streams);

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

    OperationQueue* m_operationQueue;

    RTCConfiguration m_configuration;

#if defined(STARFISH_ENABLE_TEST)
    rtc::scoped_refptr<TestPeerConnectionObserver> m_peerConnectionObserver;
#else
    rtc::scoped_refptr<PeerConnectionObserver> m_peerConnectionObserver;
#endif
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> m_backend;

    rtc::scoped_refptr<CreateSessionDescriptionObserver>
        m_createOfferSessionObserver;
    rtc::scoped_refptr<CreateSessionDescriptionObserver>
        m_createAnswerSessionObserver;
    rtc::scoped_refptr<SetSessionDescriptionObserver> m_setLocalSessionObserver;
    rtc::scoped_refptr<SetSessionDescriptionObserver>
        m_setRemoteSessionObserver;

    bool isClosed();
    void deletePeerConnection();
};
}

#endif
#endif
