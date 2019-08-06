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

#include "api/media_stream_interface.h"
#include "api/peer_connection_interface.h"

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

class PeerConnectionObserver : public webrtc::PeerConnectionObserver,
                               public webrtc::CreateSessionDescriptionObserver {
public:
    const std::string kAudioLabel = "audio_label";
    const std::string kVideoLabel = "video_label";
    const std::string kStreamId = "stream_id";
    const std::string m_stun = "stun:stun.l.google.com:19302";

    class VideoRenderer : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
    public:
        VideoRenderer(webrtc::VideoTrackInterface* trackToRender);
        virtual ~VideoRenderer();

        // VideoSinkInterface implementation
        void OnFrame(const webrtc::VideoFrame& frame) override;

    private:
        void setSize(int width, int height);
        std::unique_ptr<uint8_t[]> m_image;
        int m_width{ 0 };
        int m_height{ 0 };
        rtc::scoped_refptr<webrtc::VideoTrackInterface> m_renderedTrack;
    };

    PeerConnectionObserver();
    virtual ~PeerConnectionObserver(){};

    // PeerConnectionObserver implementation.
    void OnSignalingChange(
        webrtc::PeerConnectionInterface::SignalingState new_state) override{};
    void OnAddTrack(
        rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
        const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
            streams) override{};
    void OnRemoveTrack(
        rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override{};
    void OnDataChannel(
        rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override{};
    void OnRenegotiationNeeded() override{};
    void OnIceConnectionChange(
        webrtc::PeerConnectionInterface::IceConnectionState new_state)
        override{};
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState
                                  new_state) override{};
    void OnIceCandidate(
        const webrtc::IceCandidateInterface* candidate) override{};
    void OnIceConnectionReceivingChange(bool receiving) override{};

    // CreateSessionDescriptionObserver implementation.
    void OnSuccess(webrtc::SessionDescriptionInterface* desc) override{};
    void OnFailure(webrtc::RTCError error) override{};

    virtual void connectToPeer();
    virtual void deletePeerConnection();

protected:
    virtual bool createPeerConnection(bool dtls);
    virtual bool initializePeerConnection();
    virtual bool reinitializePeerConnectionForLoopback();

    virtual void addTracks();

    virtual void startLocalRenderer(webrtc::VideoTrackInterface* localVideo);
    virtual void stopLocalRenderer();

    rtc::scoped_refptr<webrtc::PeerConnectionInterface> m_peerConnection;
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
        m_peerConnectionFactory;
    rtc::SocketAddress m_serverAddress;
    bool m_loopback{ false };
    std::unique_ptr<VideoRenderer> m_localRenderer;
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
    virtual ~TestPeerConnectionObserver(){};

    void OnSignedIn() override;
    void OnDisconnected() override;
    void OnPeerConnected(int id, const std::string& name) override;
    void OnPeerDisconnected(int id) override;
    void OnMessageFromPeer(int peer_id, const std::string& message) override;
    void OnMessageSent(int err) override;
    void OnServerConnectionFailure() override;

    void startLogin(const std::string& server, int port);
    void deletePeerConnection() override;

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
};
#endif

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

#if defined(STARFISH_ENABLE_TEST)
    rtc::scoped_refptr<TestPeerConnectionObserver> m_peerConnectionObserver =
        rtc::scoped_refptr<TestPeerConnectionObserver>(
            new rtc::RefCountedObject<TestPeerConnectionObserver>());
#else
    rtc::scoped_refptr<PeerConnectionObserver> m_peerConnectionObserver =
        rtc::scoped_refptr<PeerConnectionObserver>(
            new rtc::RefCountedObject<PeerConnectionObserver>());
#endif
};
}

#endif
#endif
