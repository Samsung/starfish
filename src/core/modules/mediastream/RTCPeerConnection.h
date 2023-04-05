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

#include "binding/generated/DOMStringOrMediaStreamTrackUnion.h"

#include "rtc_types.h"
#include "rtc_ice_candidate.h"
#include "rtc_peerconnection.h"

#include <EscargotPublic.h>

#define STARFISH_WEBRTC_DEBUG

#ifdef STARFISH_WEBRTC_DEBUG
#include <sys/types.h>
#include <sys/syscall.h>
#include <pthread.h>

#define WEBRTC_LOGI(STR, ...)
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
class WebRtcManager;
class Mutex;

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

    bool m_voiceActivityDetection = true;
};

struct RTCOfferOptions : public RTCOfferAnswerOptions {
    DEFINE_GETTER_SETTER(bool, iceRestart, IceRestart)
    DEFINE_GETTER_SETTER(bool, offerToReceiveAudio, OfferToReceiveAudio)
    DEFINE_GETTER_SETTER(bool, offerToReceiveVideo, OfferToReceiveVideo)

    bool m_iceRestart = false;
    bool m_offerToReceiveAudio = false;
    bool m_offerToReceiveVideo = false;
};

struct RTCAnswerOptions : public RTCOfferAnswerOptions {
};

class ObserverBase : public gc {
public:
    ObserverBase(RTCPeerConnection* peerConnection)
        : m_peerConnection(peerConnection)
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

    ExecutionContext* executionContext();

protected:
    void PostTask(std::function<void()> task);

    RTCPeerConnection* m_peerConnection = nullptr;
    Promise* m_promise = nullptr;
};

class PeerConnectionObserver : public ObserverBase,
                               public libwebrtc::RTCPeerConnectionObserver {
public:
    PeerConnectionObserver(RTCPeerConnection* peerConnection);

    virtual void OnSignalingState(libwebrtc::RTCSignalingState state) override;

    virtual void OnPeerConnectionState(
        libwebrtc::RTCPeerConnectionState state) override;

    virtual void OnIceGatheringState(
        libwebrtc::RTCIceGatheringState state) override;

    virtual void OnIceConnectionState(
        libwebrtc::RTCIceConnectionState state) override;

    virtual void OnIceCandidate(
        libwebrtc::scoped_refptr<libwebrtc::RTCIceCandidate> candidate);

    virtual void OnAddStream(
        libwebrtc::scoped_refptr<libwebrtc::RTCMediaStream> stream) override;

    virtual void OnRemoveStream(
        libwebrtc::scoped_refptr<libwebrtc::RTCMediaStream> stream) override;

    virtual void OnDataChannel(
        libwebrtc::scoped_refptr<libwebrtc::RTCDataChannel> dataChannel)
        override;

    virtual void OnRenegotiationNeeded() override;

    virtual void OnTrack(libwebrtc::scoped_refptr<libwebrtc::RTCRtpTransceiver>
                             transceiver) override;

    virtual void OnAddTrack(
        libwebrtc::vector<libwebrtc::scoped_refptr<libwebrtc::RTCMediaStream>>
            streams,
        libwebrtc::scoped_refptr<libwebrtc::RTCRtpReceiver> receiver) override;

    virtual void OnRemoveTrack(
        libwebrtc::scoped_refptr<libwebrtc::RTCRtpReceiver> receiver) override;
};

class CreateOfferAnswerObserver : public ObserverBase {
    friend class RTCPeerConnection;

public:
    CreateOfferAnswerObserver(RTCPeerConnection* peerConnection);

    void OnSuccess(const libwebrtc::string sdp, const libwebrtc::string type);

    void OnFailure(const std::string& error);

    virtual bool isCreateOffer()
    {
        return false;
    }

    virtual bool isCreateAnswer()
    {
        return false;
    }

    Promise* promise()
    {
        return m_promise;
    }

    void setPromise(Promise* promise)
    {
        m_promise = promise;
    }
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

class SetLocalRemoteDescriptionObserver : public ObserverBase {
    friend class RTCPeerConnection;

public:
    SetLocalRemoteDescriptionObserver(RTCPeerConnection* peerConnection);

    void OnSuccess();

    void OnFailure(const std::string& error);

    virtual bool isLocalDescription()
    {
        return false;
    }

    virtual bool isRemoteDescription()
    {
        return false;
    }

    Promise* promise()
    {
        return m_promise;
    }

    void setPromise(Promise* promise)
    {
        m_promise = promise;
    }
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

class GetStatsObserver : public ObserverBase {
    friend class RTCPeerConnection;

public:
    GetStatsObserver(RTCPeerConnection* peerConnection);

    void OnSuccess(const libwebrtc::vector<
                   libwebrtc::scoped_refptr<libwebrtc::MediaRTCStats>>
                       reports);

    void OnFailure(const std::string& error);

    virtual bool isLocalDescription()
    {
        return false;
    }

    virtual bool isRemoteDescription()
    {
        return false;
    }
};

class RTCPeerConnection : public EventTarget {
    friend class PeerConnectionObserver;
    friend class CreateOfferAnswerObserver;
    friend class SetLocalRemoteDescriptionObserver;

public:
    RTCPeerConnection(ExecutionContext* executionContext,
                      RTCConfiguration configuration = RTCConfiguration());
    virtual ~RTCPeerConnection();
    void dispose();

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

    RTCConfiguration getConfiguration();
    void setConfiguration(RTCConfiguration& configuration,
                          bool checkStatus = true);

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
    DECLARE_EVENT_LISTENER(track);
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

    Promise* getStats(MediaStreamTrack* selector = nullptr);

    libwebrtc::scoped_refptr<libwebrtc::RTCPeerConnection> backend();
    bool initializePeerConnection();
    bool initializePeerConnection(RTCConfiguration& configuration);

    ScriptObject createSessionDescriptionInitObject(RTCSdpType type,
                                                    String* sdp);
    RTCSdpType toRtcSdpType(std::string type);
    Nullable<libwebrtc::RTCSessionDescription::SdpType> toSdpType(
        Nullable<RTCSdpType> type);
    DOMException* toDomException(std::string type);

    bool isClosed();
    void addStream(MediaStream* stream);
    GCVector<MediaStream*> getRemoteStreams();

private:
    bool isDisposed()
    {
        return m_backend.get() == nullptr;
    }

    bool isValidRemoteState(RTCSdpType type);

    Promise* setRtcSessionDescription(RTCSessionDescriptionInit description,
                                      Promise* promise, bool isRemote);
    RTCRtpTransceiver* getOrCreateRTCRtpTransceiver(
        libwebrtc::scoped_refptr<libwebrtc::RTCRtpTransceiver>
            backendTransceiver);

    RTCRtpSender* getOrCreateRTCRtpSender(
        libwebrtc::scoped_refptr<libwebrtc::RTCRtpSender> sender);

    void syncTransceivers();

    ExecutionContext* m_executionContext = nullptr;
    WebRtcManager* m_webRtcManager = nullptr;
    RTCConfiguration m_configuration;
    PeerConnectionObserver* m_peerConnectionObserver = nullptr;
    libwebrtc::scoped_refptr<libwebrtc::RTCPeerConnection> m_backend;
    CreateOfferObserver* m_createOfferObserver = nullptr;
    CreateAnswerObserver* m_createAnswerObserver = nullptr;
    SetLocalDescriptionObserver* m_setLocalDescriptionObserver = nullptr;
    SetRemoteDescriptionObserver* m_setRemoteDescriptionObserver = nullptr;
    GetStatsObserver* m_getStatsObserver = nullptr;
    String* m_lastCreatedOffer;
    String* m_lastCreatedAnswer;
    GCVector<RTCRtpTransceiver*> m_transceivers;
    GCVector<RTCDataChannel*> m_dataChannels;
    Mutex* m_disposeLock = nullptr;
    bool m_closed = false;
    bool m_wait = false;

    String* m_signalingState = String::emptyString;
    String* m_iceGatheringState = String::emptyString;
    String* m_iceConnectionState = String::emptyString;
    String* m_connectionState = String::emptyString;
};
} // namespace Starfish

#endif
#endif
