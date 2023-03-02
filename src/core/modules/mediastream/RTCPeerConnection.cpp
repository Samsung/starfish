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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/mediastream/RTCPeerConnection.h"

#include "EscargotPublic.h"
#include "binding/ScriptBindingInstance.h"
#include "core/dom/DOMException.h"
#include "core/dom/Event.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/Document.h"
#include "core/page/Window.h"
#include "core/page/Navigator.h"
#include "core/modules/mediastream/WebRtcManager.h"
#include "core/modules/mediastream/RTCCertificate.h"
#include "core/modules/mediastream/RTCConfiguration.h"
#include "core/modules/mediastream/RTCSessionDescription.h"
#include "core/modules/mediastream/MediaStream.h"
#include "core/modules/mediastream/RTCRtpSender.h"
#include "core/modules/mediastream/RTCRtpReceiver.h"
#include "core/modules/mediastream/RTCRtpTransceiver.h"
#include "core/modules/mediastream/RTCTrackEvent.h"
#include "core/modules/mediastream/RTCPeerConnectionIceEvent.h"
#include "core/modules/mediastream/RTCDataChannelEvent.h"
#include "core/modules/mediastream/RTCError.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/threading/Mutex.h"
#include "core/page/WebBase.h"
#include "core/page/GlobalScope.h"

#include "rtc_rtp_sender.h"

namespace Starfish {

ExecutionContext* PeerConnectionObserver::executionContext() const
{
    return m_peerConnection->executionContext();
}

PeerConnectionObserver::PeerConnectionObserver(
    RTCPeerConnection* peerConnection)
    : m_peerConnection(peerConnection)
{
    m_webRtcManager =
        executionContext()->document()->window()->navigator()->webRtcManager();

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            ((PeerConnectionObserver*)obj)->~PeerConnectionObserver();
        },
        NULL, NULL, NULL);
}

PeerConnectionObserver::~PeerConnectionObserver()
{
    if (m_peerConnection) {
        m_peerConnection->dispose();
    }
    m_peerConnection = nullptr;
}

void PeerConnectionObserver::OnSignalingState(
    libwebrtc::RTCSignalingState newState)
{
    if (newState == libwebrtc::RTCSignalingState::RTCSignalingStateClosed) {
        return;
    }

    if (!m_peerConnection || m_peerConnection->isClosed()) {
        return;
    }

    struct Params {
        PeerConnectionObserver* self;
        libwebrtc::RTCSignalingState newState;
    };

    Params* p = new Params{ this, newState };

    executionContext()
        ->webBase()
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            executionContext()->globalScope(),
            [](size_t, void* data) {
                Params* p = (Params*)data;
                PeerConnectionObserver* self = p->self;

                if (!self->m_peerConnection ||
                    self->m_peerConnection->isClosed()) {
                    delete p;
                    return;
                }

                String* eventType = self->executionContext()
                                        ->starfish()
                                        ->staticStrings()
                                        ->m_signalingstatechange.localName();
                Event* e = new Event(self->executionContext(), eventType);
                self->m_peerConnection->dispatchEventByUA(e);
                delete p;
            },
            p);
}

#if defined(STARFISH_WEBRTC_DEBUG)
class AudioTrackObserver : public b2bua::AudioFrame {
    virtual void UpdateFrame(int id, uint32_t timestamp, const int16_t* data,
                             size_t samplesPerChannel, int sampleRateHz,
                             size_t numChannels)
    {
    }
};

class VideoFrameObserver
    : public libwebrtc::RTCVideoRenderer<
          libwebrtc::scoped_refptr<libwebrtc::RTCVideoFrame>> {
public:
    void OnFrame(
        libwebrtc::scoped_refptr<libwebrtc::RTCVideoFrame> frame) override{};
};
#endif

void PeerConnectionObserver::OnTrack(
    libwebrtc::scoped_refptr<libwebrtc::RTCRtpTransceiver> transceiver)
{
    if (!isMainThread()) {
        if (!m_peerConnection || m_peerConnection->isClosed()) {
            return;
        }

        struct Params {
            PeerConnectionObserver* self;
            libwebrtc::scoped_refptr<libwebrtc::RTCRtpTransceiver> transceiver;
        };

        Params* p = new Params{ this, transceiver };

        executionContext()
            ->webBase()
            ->messageLoop()
            ->addIdlerWithNoGCRootingInOtherThread(
                executionContext()->globalScope(),
                [](size_t, void* data) {
                    Params* p = (Params*)data;

                    if (!p->self->m_peerConnection ||
                        p->self->m_peerConnection->isClosed()) {
                        return;
                    }

                    p->self->OnTrack(p->transceiver);
                    delete p;
                },
                p);
        return;
    }

    m_peerConnection->syncTransceivers();
    RTCRtpTransceiver* rtpTransceiver =
        m_peerConnection->getTransceiver(transceiver);
    if (!rtpTransceiver) {
        STARFISH_LOG_DEBUG("Transceiver is null");
        return;
    }
    RTCRtpReceiver* rtpReceiver = rtpTransceiver->receiver();
    if (!rtpReceiver) {
        STARFISH_LOG_DEBUG("Receiver is null");
        return;
    }
    MediaStreamTrack* track = rtpReceiver->track();
    if (!track) {
        STARFISH_LOG_DEBUG("MediaTrack is null");
        return;
    }
    GCVector<MediaStream*> rtpStreams;
    for (auto stream : rtpReceiver->streams()) {
        stream->addTrack(track);
        rtpStreams.push_back(stream);
    }

    String* eventType =
        executionContext()->starfish()->staticStrings()->m_track.localName();
    RTCTrackEventInit init(rtpReceiver, track, rtpStreams, rtpTransceiver);
    RTCTrackEvent* e = new RTCTrackEvent(executionContext(), eventType, init);
    m_peerConnection->dispatchEventByUA(e);
}

// https://w3c.github.io/webrtc-pc/#event-datachannel
void PeerConnectionObserver::OnDataChannel(
    libwebrtc::scoped_refptr<libwebrtc::RTCDataChannel> channel)
{
    if (!isMainThread()) {
        if (!m_peerConnection || m_peerConnection->isClosed()) {
            return;
        }

        struct Params {
            PeerConnectionObserver* self;
            libwebrtc::scoped_refptr<libwebrtc::RTCDataChannel> channel;
        };

        Params* p = new Params{ this, channel };

        executionContext()
            ->webBase()
            ->messageLoop()
            ->addIdlerWithNoGCRootingInOtherThread(
                executionContext()->globalScope(),
                [](size_t, void* data) {
                    Params* p = (Params*)data;

                    if (!p->self->m_peerConnection ||
                        p->self->m_peerConnection->isClosed()) {
                        delete p;
                        return;
                    }

                    p->self->OnDataChannel(p->channel);
                    delete p;
                },
                p);
        return;
    }

    RTCDataChannelInit channelInit;
    RTCDataChannel* rtcChannel = new RTCDataChannel(
        executionContext(), m_peerConnection, channelInit, channel);

    String* eventType = executionContext()
                            ->starfish()
                            ->staticStrings()
                            ->m_datachannel.localName();
    RTCDataChannelEventInit eventInit;
    RTCDataChannelEvent* e = new RTCDataChannelEvent(
        executionContext(), eventType, eventInit, rtcChannel);
    m_peerConnection->dispatchEventByUA(e);
}

// https://w3c.github.io/webrtc-pc/#dfn-update-the-negotiation-needed-flag
void PeerConnectionObserver::OnRenegotiationNeeded()
{
    if (!isMainThread()) {
        if (!m_peerConnection || m_peerConnection->isDisposed() ||
            m_peerConnection->isClosed()) {
            return;
        }

        executionContext()
            ->webBase()
            ->messageLoop()
            ->addIdlerWithNoGCRootingInOtherThread(
                executionContext()->globalScope(),
                [](size_t, void* data) {
                    PeerConnectionObserver* self =
                        (PeerConnectionObserver*)data;

                    if (!self->m_peerConnection ||
                        self->m_peerConnection->isDisposed() ||
                        self->m_peerConnection->isClosed()) {
                        return;
                    }
                    self->OnRenegotiationNeeded();
                },
                this);
        return;
    }

    String* eventType = executionContext()
                            ->starfish()
                            ->staticStrings()
                            ->m_negotiationneeded.localName();
    Event* e = new Event(executionContext(), eventType);
    m_peerConnection->dispatchEventByUA(e);
    return;
}

void PeerConnectionObserver::OnIceConnectionState(
    libwebrtc::RTCIceConnectionState newState)
{
    if (newState ==
            libwebrtc::RTCIceConnectionState::RTCIceConnectionStateClosed ||
        newState == libwebrtc::RTCIceConnectionState::
                        RTCIceConnectionStateDisconnected) {
        return;
    }

    if (!isMainThread()) {
        if (!m_peerConnection || m_peerConnection->isClosed()) {
            return;
        }

        struct Params {
            PeerConnectionObserver* self;
            libwebrtc::RTCIceConnectionState newState;
        };

        Params* p = new Params{ this, newState };

        executionContext()
            ->webBase()
            ->messageLoop()
            ->addIdlerWithNoGCRootingInOtherThread(
                executionContext()->globalScope(),
                [](size_t, void* data) {
                    Params* p = (Params*)data;

                    if (!p->self->m_peerConnection ||
                        p->self->m_peerConnection->isClosed()) {
                        delete p;
                        return;
                    }

                    p->self->OnIceConnectionState(p->newState);
                    delete p;
                },
                p);
        return;
    }

    String* eventType = executionContext()
                            ->starfish()
                            ->staticStrings()
                            ->m_iceconnectionstatechange.localName();
    Event* e = new Event(executionContext(), eventType);
    m_peerConnection->dispatchEventByUA(e);
}

void PeerConnectionObserver::OnIceGatheringState(
    libwebrtc::RTCIceGatheringState newState)
{
    if (newState ==
        libwebrtc::RTCIceGatheringState::RTCIceGatheringStateComplete) {
        return;
    }

    if (!isMainThread()) {
        if (!m_peerConnection || m_peerConnection->isClosed()) {
            return;
        }

        struct Params {
            PeerConnectionObserver* self;
            libwebrtc::RTCIceGatheringState newState;
        };

        Params* p = new Params{ this, newState };

        executionContext()
            ->webBase()
            ->messageLoop()
            ->addIdlerWithNoGCRootingInOtherThread(
                executionContext()->globalScope(),
                [](size_t, void* data) {
                    Params* p = (Params*)data;

                    if (!p->self->m_peerConnection ||
                        p->self->m_peerConnection->isClosed()) {
                        delete p;
                        return;
                    }

                    p->self->OnIceGatheringState(p->newState);
                    delete p;
                },
                p);
        return;
    }

    String* eventType = executionContext()
                            ->starfish()
                            ->staticStrings()
                            ->m_icegatheringstatechange.localName();
    Event* e = new Event(executionContext(), eventType);
    m_peerConnection->dispatchEventByUA(e);
}

void PeerConnectionObserver::OnIceCandidate(
    libwebrtc::scoped_refptr<libwebrtc::RTCIceCandidate> candidate)
{
    if (!m_peerConnection || m_peerConnection->isClosed()) {
        return;
    }

    struct Params {
        PeerConnectionObserver* self;
        std::string sdpMid;
        int sdpMlineIndex;
        std::string sdp;
    };

    libwebrtc::string sdp;
    candidate->ToString(sdp);

    Params* p = new Params();
    p->self = this;
    p->sdpMid = candidate->sdp_mid().std_string();
    p->sdpMlineIndex = candidate->sdp_mline_index();
    p->sdp = sdp.std_string();

    executionContext()
        ->webBase()
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            executionContext()->globalScope(),
            [](size_t, void* data) {
                Params* p = (Params*)data;
                PeerConnectionObserver* self = p->self;

                if (!self->m_peerConnection ||
                    self->m_peerConnection->isClosed()) {
                    delete p;
                    return;
                }

                RTCIceCandidateInit cinit(p->sdp, p->sdpMid, p->sdpMlineIndex);
                RTCIceCandidate* cand =
                    new RTCIceCandidate(self->executionContext(), cinit);

                RTCPeerConnectionIceEventInit init;
                init.m_candidate = cand;

                String* eventType = self->executionContext()
                                        ->starfish()
                                        ->staticStrings()
                                        ->m_icecandidate.localName();
                RTCPeerConnectionIceEvent* e = new RTCPeerConnectionIceEvent(
                    self->executionContext(), eventType, init);
                self->m_peerConnection->dispatchEventByUA(e);
                delete p;
            },
            p);
}

void CreateOfferAnswerObserver::OnSuccess(const libwebrtc::string sdp,
                                          const libwebrtc::string type)
{
    if (!m_peerConnection) {
        return;
    }

    // This callback is called from another thread
    // The following lines must be executed in the main thread.
    struct Params {
        CreateOfferAnswerObserver* self;
        std::string sdp;
        std::string type;
    };

    Params* p = new Params{ this, sdp.std_string(), type.std_string() };

    m_peerConnection->executionContext()
        ->webBase()
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            m_peerConnection->executionContext()->globalScope(),
            [](size_t, void* data) {
                Params* p = (Params*)data;
                CreateOfferAnswerObserver* self = p->self;
                std::string sdpString = p->sdp;
                std::string typeString = p->type;

                ObjectRef* sd =
                    self->m_peerConnection->createSessionDescriptionInitObject(
                        self->m_peerConnection->toRtcSdpType(typeString),
                        String::createASCIIString(sdpString.c_str(),
                                                  sdpString.size()));
                Promise* promise = nullptr;
                if (self->isCreateOffer()) {
                    promise = self->m_peerConnection->m_createOfferObserver
                                  ->promise();
                    self->m_peerConnection->m_createOfferObserver->setPromise(
                        nullptr);
                    self->m_peerConnection->m_lastCreatedOffer = sdpString;
                } else if (self->isCreateAnswer()) {
                    promise = self->m_peerConnection->m_createAnswerObserver
                                  ->promise();
                    self->m_peerConnection->m_createAnswerObserver->setPromise(
                        nullptr);
                    self->m_peerConnection->m_lastCreatedAnswer = sdpString;
                }

                if (promise) {
                    promise->fulfill(createScriptValue(sd));
                } else {
                    STARFISH_LOG_ERROR("Unknown promise type");
                }
                delete p;
            },
            p);
}

void CreateOfferAnswerObserver::OnFailure(const char* error)
{
    // This callback is called from another thread
    // The following lines must be executed in the main thread.
    struct Params {
        CreateOfferAnswerObserver* self;
        std::string error;
    };

    Params* p = new Params{ this, std::move(error) };

    m_peerConnection->executionContext()
        ->webBase()
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            m_peerConnection->executionContext()->globalScope(),
            [](size_t, void* data) {
                Params* p = (Params*)data;
                CreateOfferAnswerObserver* self = p->self;

                if (!self->m_peerConnection) {
                    return;
                }

                DOMException* exception =
                    self->m_peerConnection->toDomException(std::move(p->error));
                STARFISH_ASSERT(exception);

                Promise* promise = nullptr;
                if (self->isCreateOffer()) {
                    promise = self->m_peerConnection->m_createOfferObserver
                                  ->promise();
                    self->m_peerConnection->m_createOfferObserver->setPromise(
                        nullptr);
                } else if (self->isCreateAnswer()) {
                    promise = self->m_peerConnection->m_createAnswerObserver
                                  ->promise();
                    self->m_peerConnection->m_createAnswerObserver->setPromise(
                        nullptr);
                }

                if (promise) {
                    promise->reject(exception->scriptValue());
                } else {
                    STARFISH_LOG_ERROR("Unknown promise type");
                }

                delete p;
            },
            p);
}

void SetLocalRemoteDescriptionObserver::OnSuccess()
{
    if (!isMainThread()) {
        m_peerConnection->executionContext()
            ->webBase()
            ->messageLoop()
            ->addIdlerWithNoGCRootingInOtherThread(
                m_peerConnection->executionContext()->globalScope(),
                [](size_t, void* data) {
                    SetLocalRemoteDescriptionObserver* self =
                        (SetLocalRemoteDescriptionObserver*)data;
                    if (!self->m_peerConnection) {
                        return;
                    }

                    self->OnSuccess();
                },
                this);
        return;
    }

    Promise* promise = nullptr;
    if (isLocalDescription()) {
        promise = m_peerConnection->m_setLocalDescriptionObserver->promise();
        m_peerConnection->m_setLocalDescriptionObserver->setPromise(nullptr);
    } else if (isRemoteDescription()) {
        promise = m_peerConnection->m_setRemoteDescriptionObserver->promise();
        m_peerConnection->m_setRemoteDescriptionObserver->setPromise(nullptr);
    }

    if (promise) {
        promise->fulfill(scriptUndefined());
    } else {
        STARFISH_LOG_ERROR("Unknown promise type");
    }
}

void SetLocalRemoteDescriptionObserver::OnFailure(const char* error)
{
    // This callback is called from another thread
    // The following lines must be executed in the main thread.
    struct Params {
        SetLocalRemoteDescriptionObserver* self;
        std::string error;
    };

    Params* p = new Params();
    p->self = this;
    p->error = std::move(error);

    m_peerConnection->executionContext()
        ->webBase()
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            m_peerConnection->executionContext()->globalScope(),
            [](size_t, void* data) {
                Params* p = (Params*)data;
                SetLocalRemoteDescriptionObserver* self = p->self;

                if (!self->m_peerConnection) {
                    delete p;
                    return;
                }

                DOMException* exception =
                    self->m_peerConnection->toDomException(p->error.c_str());
                STARFISH_ASSERT(exception);

                Promise* promise = nullptr;
                if (self->isLocalDescription()) {
                    promise = self->m_peerConnection
                                  ->m_setLocalDescriptionObserver->promise();
                    self->m_peerConnection->m_setLocalDescriptionObserver
                        ->setPromise(nullptr);
                } else if (self->isRemoteDescription()) {
                    promise = self->m_peerConnection
                                  ->m_setRemoteDescriptionObserver->promise();
                    self->m_peerConnection->m_setRemoteDescriptionObserver
                        ->setPromise(nullptr);
                }

                if (promise) {
                    promise->reject(exception->scriptValue());
                } else {
                    STARFISH_LOG_DEBUG("Unknown promise type");
                }
                delete p;
            },
            p);
}

// https://w3c.github.io/webrtc-pc/#constructor
RTCPeerConnection::RTCPeerConnection(ExecutionContext* executionContext,
                                     RTCConfiguration configuration)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_disposeLock(new Mutex())
{
    if (!configuration.certificates().empty()) {
        // TODO
        STARFISH_UNIMPLEMENTED();
    } else {
    }

    setConfiguration(configuration, false);

    m_webRtcManager = this->executionContext()
                          ->document()
                          ->window()
                          ->navigator()
                          ->webRtcManager();

    if (!initializePeerConnection(configuration)) {
        m_peerConnectionObserver->m_peerConnection = nullptr;
        m_peerConnectionObserver = nullptr;
        m_webRtcManager->deletePeerConnection(nullptr);
        m_backend = nullptr;
        STARFISH_LOG_ERROR("PeerConnection: failed");
        throw new DOMException(executionContext, DOMException::DOM_EXCEPTION,
                               "Invalid Configuration");
    }

    m_webRtcManager->addPeerConnection(this);

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            ((RTCPeerConnection*)obj)->~RTCPeerConnection();
        },
        NULL, NULL, NULL);
}

bool RTCPeerConnection::initializePeerConnection()
{
    return initializePeerConnection(m_configuration);
}

bool RTCPeerConnection::initializePeerConnection(
    RTCConfiguration& configuration)
{
    m_peerConnectionObserver = new PeerConnectionObserver(this);
    STARFISH_ASSERT(m_webRtcManager->peerConnectionFactory());

    libwebrtc::RTCConfiguration config = configuration.genBackend();
    config.sdp_semantics = libwebrtc::SdpSemantics::kUnifiedPlan;

    libwebrtc::scoped_refptr<libwebrtc::RTCMediaConstraints> constraints =
        libwebrtc::RTCMediaConstraints::Create();

    m_backend =
        m_webRtcManager->peerConnectionFactory()->Create(config, constraints);
    m_backend->RegisterRTCPeerConnectionObserver(m_peerConnectionObserver);

    m_createOfferObserver = new CreateOfferObserver(this);
    m_createAnswerObserver = new CreateAnswerObserver(this);
    m_setLocalDescriptionObserver = new SetLocalDescriptionObserver(this);
    m_setRemoteDescriptionObserver = new SetRemoteDescriptionObserver(this);

    return m_backend != nullptr;
}

RTCPeerConnection::~RTCPeerConnection()
{
    dispose();
}

void RTCPeerConnection::dispose()
{
    if (isDisposed()) {
        return;
    }

    if (!m_backend.get()) {
        return;
    }

    for (auto dataChannel : m_dataChannels) {
        dataChannel->dispose();
    }
    m_dataChannels.clear();

    for (auto transceiver : m_transceivers) {
        transceiver->dispose();
    }
    m_transceivers.clear();

    m_backend = nullptr;
    m_webRtcManager->deletePeerConnection(this);
}

ScriptBindingInstance* RTCPeerConnection::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

ExecutionContext* RTCPeerConnection::executionContext() const
{
    return m_executionContext;
}

// https://w3c.github.io/webrtc-pc/#dom-rtcpeerconnection-createoffer
Promise* RTCPeerConnection::createOffer(RTCOfferOptions options)
{
    // 1-2
    if (isClosed()) {
        Promise* promise = new Promise(scriptBindingInstance());
        auto exception = new DOMException(executionContext(),
                                          DOMException::INVALID_STATE_ERR,
                                          "InvalidStateError");
        promise->reject(exception->scriptValue());
        return promise;
    }

    if (backend() == nullptr) {
        Promise* promise = new Promise(scriptBindingInstance());
        STARFISH_LOG_WARN("backend() == nullptr");
        auto exception = new DOMException(
            executionContext(), DOMException::INVALID_STATE_ERR,
            "Internal Error: backend() == nullptr");
        promise->reject(exception->scriptValue());
        return promise;
    }

    Promise* promise = new Promise(scriptBindingInstance());
    m_createOfferObserver->setPromise(promise);

    libwebrtc::scoped_refptr<libwebrtc::RTCMediaConstraints> constraints =
        libwebrtc::RTCMediaConstraints::Create();

    m_backend->CreateOffer(
        [this](const libwebrtc::string sdp, const libwebrtc::string type) {
            m_createOfferObserver->OnSuccess(sdp, type);
        },
        [this](const char* error) { m_createOfferObserver->OnFailure(error); },
        constraints);

    return promise;
}

// https://w3c.github.io/webrtc-pc/#dom-rtcpeerconnection-createanswer
Promise* RTCPeerConnection::createAnswer(RTCAnswerOptions options)
{
    if (isClosed()) {
        Promise* promise = new Promise(scriptBindingInstance());
        auto exception = new DOMException(executionContext(),
                                          DOMException::INVALID_STATE_ERR,
                                          "InvalidStateError");
        promise->reject(exception->scriptValue());
        return promise;
    }

    if (backend() == nullptr) {
        Promise* promise = new Promise(scriptBindingInstance());
        STARFISH_LOG_WARN("backend() == nullptr");
        auto exception = new DOMException(
            executionContext(), DOMException::INVALID_STATE_ERR,
            "Internal Error: backend() == nullptr");
        promise->reject(exception->scriptValue());
        return promise;
    }

    Promise* promise = new Promise(scriptBindingInstance());
    m_createAnswerObserver->setPromise(promise);

    libwebrtc::scoped_refptr<libwebrtc::RTCMediaConstraints> constraints =
        libwebrtc::RTCMediaConstraints::Create();

    m_backend->CreateAnswer(
        [this](const libwebrtc::string sdp, const libwebrtc::string type) {
            m_createAnswerObserver->OnSuccess(sdp, type);
        },
        [this](const std::string& error) {
            m_createAnswerObserver->OnFailure(error.c_str());
        },
        constraints);
    return promise;
}

void RTCPeerConnection::syncTransceivers()
{
    GCUnorderedMap<std::string, RTCRtpTransceiver*> curTransceivers;
    for (auto transceiver : m_transceivers) {
        curTransceivers.insert(std::make_pair(
            transceiver->backend()->receiver()->id().c_string(), transceiver));
    }
    m_transceivers.clear();

    std::vector<libwebrtc::scoped_refptr<libwebrtc::RTCRtpTransceiver>>
        backendTransceivers = m_backend->transceivers().std_vector();
    for (auto transceiver : backendTransceivers) {
        auto itr =
            curTransceivers.find(transceiver->receiver()->id().std_string());
        if (itr != curTransceivers.end()) {
            m_transceivers.push_back(itr->second);
        } else {
            RTCRtpTransceiver* newTransceiver =
                new RTCRtpTransceiver(executionContext(), this, transceiver);
            m_transceivers.push_back(newTransceiver);
        }
    }
}

RTCRtpTransceiver* RTCPeerConnection::getTransceiver(
    libwebrtc::scoped_refptr<libwebrtc::RTCRtpTransceiver> backendTransceiver)
{
    syncTransceivers();
    for (auto transceiver : m_transceivers) {
        if (backendTransceiver->sender()->id().std_string() ==
                transceiver->backend()->sender()->id().std_string() &&
            backendTransceiver->receiver()->id().std_string() ==
                transceiver->backend()->receiver()->id().std_string()) {
            return transceiver;
        }
    }
    return nullptr;
}

RTCRtpSender* RTCPeerConnection::getSender(
    libwebrtc::scoped_refptr<libwebrtc::RTCRtpSender> backendSender)
{
    RTCRtpSender* result = nullptr;
    syncTransceivers();
    for (auto transceiver : m_transceivers) {
        if (transceiver->sender()->backend()->id().std_string() ==
            backendSender->id().std_string()) {
            result = transceiver->sender();
            break;
        }
    }
    return result;
}

ScriptObject RTCPeerConnection::createSessionDescriptionInitObject(
    RTCSdpType type, String* sdp)
{
    ContextRef* ctx = scriptBindingInstance()->scriptContext();
    return Evaluator::execute(
               ctx,
               [](ExecutionStateRef* state, RTCSdpType type,
                  String* sdp) -> ValueRef* {
                   RTCSessionDescriptionInit sd(type, sdp);
                   ScriptObject obj = ObjectRef::create(state);
                   String* sdValue = sd.type();
                   obj->set(
                       state,
                       ValueRef::create(StringRef::createFromASCII("type")),
                       ValueRef::create(toJSString(sdValue)));
                   String* sdpValue = sd.sdp();
                   obj->set(state,
                            ValueRef::create(StringRef::createFromASCII("sdp")),
                            ValueRef::create(toJSString(sdpValue)));

                   return obj;
               },
               type, sdp)
        .result->asObject();
}

// https://w3c.github.io/webrtc-pc/#dom-peerconnection-setlocaldescription
Promise* RTCPeerConnection::setLocalDescription(
    RTCSessionDescriptionInit& description)
{
    if (isClosed()) {
        Promise* promise = new Promise(scriptBindingInstance());
        promise->fulfill(scriptUndefined());
        return promise;
    }

    if (backend() == nullptr) {
        Promise* promise = new Promise(scriptBindingInstance());
        STARFISH_LOG_WARN("backend() == nullptr");
        auto exception = new DOMException(
            executionContext(), DOMException::INVALID_STATE_ERR,
            "Internal Error: backend() == nullptr");
        promise->reject(exception->scriptValue());
        return promise;
    }

    Promise* promise = new Promise(scriptBindingInstance());
    m_setLocalDescriptionObserver->setPromise(promise);

    Nullable<libwebrtc::RTCSessionDescription::SdpType> type =
        toSdpType(description.m_type);
    if (!type.hasValue()) {
        STARFISH_LOG_ERROR("%s: rollback is not supported", __func__);
        auto exception =
            new DOMException(executionContext(), DOMException::DOM_EXCEPTION,
                             "rollback is not supported");
        promise->reject(exception->scriptValue());
        return promise;
    }

    std::string sdpString = std::string(description.m_sdp->toUTF8NonGCString());

    // 4.2
    if ((description.m_type == RTCSdpType::Offer) &&
        !description.m_sdp->equals("") && (m_lastCreatedOffer != sdpString)) {
        auto exception = new DOMException(
            executionContext(), DOMException::INVALID_MODIFICATION_ERR,
            "setLocalDescription");
        promise->reject(exception->scriptValue());
        return promise;
    }

    // 4.3
    if ((description.m_type == RTCSdpType::Answer ||
         description.m_type == RTCSdpType::Pranswer) &&
        !description.m_sdp->equals("") && (m_lastCreatedAnswer != "") &&
        (m_lastCreatedAnswer != sdpString)) {
        auto exception = new DOMException(
            executionContext(), DOMException::INVALID_MODIFICATION_ERR,
            "setLocalDescription");
        promise->reject(exception->scriptValue());
        return promise;
    }

    // 4.4
    if ((description.m_type == RTCSdpType::Offer) &&
        description.m_sdp->equals("")) {
        sdpString = m_lastCreatedOffer;
    }

    // 4.5
    if ((description.m_type == RTCSdpType::Answer ||
         description.m_type == RTCSdpType::Pranswer) &&
        description.m_sdp->equals("")) {
        sdpString = m_lastCreatedAnswer;
    }

    // 4.6
    return setRtcSessionDescription({ type, sdpString }, promise, false);
}

// https://w3c.github.io/webrtc-pc/#set-the-rtcsessiondescription
Promise* RTCPeerConnection::setRtcSessionDescription(
    RTCSessionDescriptionInit description, Promise* promise, bool isRemote)
{
    // 3.1.2
    if (isRemote) {
        if ((description.m_type == RTCSdpType::Answer)) {
            if (!((m_backend->signaling_state() ==
                   libwebrtc::RTCSignalingState::
                       RTCSignalingStateHaveLocalOffer) ||
                  (m_backend->signaling_state() ==
                   libwebrtc::RTCSignalingState::
                       RTCSignalingStateHaveRemoteOffer))) {
                auto exception = new DOMException(
                    executionContext(), DOMException::INVALID_STATE_ERR,
                    "InvalidStateError");
                promise->reject(exception->scriptValue());
                return promise;
            }
        }
    } else {
        if ((description.m_type == RTCSdpType::Answer) ||
            (description.m_type == RTCSdpType::Pranswer)) {
            if (!((m_backend->signaling_state() ==
                   libwebrtc::RTCSignalingState::
                       RTCSignalingStateHaveRemoteOffer) ||
                  (m_backend->signaling_state() ==
                   libwebrtc::RTCSignalingState::
                       RTCSignalingStateHaveRemotePrAnswer))) {
                auto exception = new DOMException(
                    executionContext(), DOMException::INVALID_STATE_ERR,
                    "InvalidStateError");
                promise->reject(exception->scriptValue());
                return promise;
            }
        }
    }

    // 3.2.2 and 3.2.3
    if (description.m_type == RTCSdpType::Answer) {
        m_lastCreatedOffer = "";
        m_lastCreatedAnswer = "";
    }

    Nullable<libwebrtc::RTCSessionDescription::SdpType> type =
        toSdpType(description.m_type);
    std::string typeString = description.type()->toUTF8NonGCString();
    std::string sdpString = std::string(description.m_sdp->toUTF8NonGCString());
    libwebrtc::SdpParseError error;

    libwebrtc::scoped_refptr<libwebrtc::RTCSessionDescription> des =
        libwebrtc::RTCSessionDescription::Create(typeString.c_str(),
                                                 sdpString.c_str(), &error);

    if (!des.get()) {
        STARFISH_LOG_WARN("%s: desc = nullptr", __func__);
        return promise;
    }

    if (isRemote) {
        // SetRemoteDescription takes the ownership of desc
        m_backend->SetRemoteDescription(
            des->sdp(), des->type(),
            [this]() { m_setRemoteDescriptionObserver->OnSuccess(); },
            [this](const char* error) {
                m_setRemoteDescriptionObserver->OnFailure(error);
            });
    } else {
        // SetLocalDescription takes the ownership of desc
        m_backend->SetLocalDescription(
            des->sdp(), des->type(),
            [this]() { m_setLocalDescriptionObserver->OnSuccess(); },
            [this](const char* error) {
                m_setLocalDescriptionObserver->OnFailure(error);
            });
    }
    return promise;
}

RTCSessionDescription* RTCPeerConnection::localDescription()
{
    libwebrtc::scoped_refptr<libwebrtc::RTCSessionDescription> description;
    libwebrtc::SdpParseError error;
    char* sdp_ptr = nullptr;
    char* type_ptr = nullptr;

    m_backend->GetLocalDescription(
        [&sdp_ptr, &type_ptr](const char* sdp, const char* type) {
            sdp_ptr = const_cast<char*>(sdp);
            type_ptr = const_cast<char*>(type);
        },
        [](const std::string& error) {});

    if (sdp_ptr && type_ptr) {
        description =
            libwebrtc::RTCSessionDescription::Create(type_ptr, sdp_ptr, &error);
        return new RTCSessionDescription(executionContext(), description);
    }
    return nullptr;
}

RTCSessionDescription* RTCPeerConnection::currentLocalDescription()
{
    // TODO:FIX ME!!
    return localDescription();
}

RTCSessionDescription* RTCPeerConnection::pendingLocalDescription()
{
    // TODO:FIX ME!!
    return localDescription();
}

// https://w3c.github.io/webrtc-pc/#dom-peerconnection-setremotedescription
Promise* RTCPeerConnection::setRemoteDescription(
    RTCSessionDescriptionInit& description)
{
    if (isClosed()) {
        Promise* promise = new Promise(scriptBindingInstance());
        promise->fulfill(scriptUndefined());
        return promise;
    }

    if (backend() == nullptr) {
        Promise* promise = new Promise(scriptBindingInstance());
        STARFISH_LOG_WARN("backend() == nullptr");
        auto exception = new DOMException(
            executionContext(), DOMException::INVALID_STATE_ERR,
            "Internal Error: backend() == nullptr");
        promise->reject(exception->scriptValue());
        return promise;
    }

    Promise* promise = new Promise(scriptBindingInstance());
    m_setRemoteDescriptionObserver->setPromise(promise);

    Nullable<libwebrtc::RTCSessionDescription::SdpType> type =
        toSdpType(description.m_type);
    if (!type.hasValue()) {
        STARFISH_LOG_ERROR("%s: rollback is not supported", __func__);
        auto exception =
            new DOMException(executionContext(), DOMException::DOM_EXCEPTION,
                             "rollback is not supported");
        promise->reject(exception->scriptValue());
        return promise;
    }

    std::string sdpString = std::string(description.m_sdp->toUTF8NonGCString());

    // https://w3c.github.io/webrtc-pc/#dom-peerconnection-setremotedescription
    // 3
    if (!description.m_type.hasValue()) {
        auto exception =
            new DOMException(executionContext(), DOMException::SCRIPT_TYPE_ERR,
                             "Invalid description type");
        promise->reject(exception->scriptValue());
        return promise;
    }

    // 4
    if ((description.m_type == RTCSdpType::Offer) &&
        !isValidRemoteState(description.m_type.value())) {
        auto exception = new DOMException(executionContext(),
                                          DOMException::INVALID_STATE_ERR,
                                          "setRemoteDescription");
        promise->reject(exception->scriptValue());
        return promise;
    }

    // https://w3c.github.io/webrtc-pc/#set-remote-description
    // 3.3

    // if (error.line == "Invalid SDP") {
    //     RTCErrorInit init;
    //     init.m_errorDetail = RTCErrorDetailType::SdpSyntaxError;
    //     auto exception = new RTCError(executionContext(), init);
    //     promise->reject(exception->scriptValue());
    //     return promise;
    // }

    return setRtcSessionDescription({ type, sdpString }, promise, true);
}

RTCSessionDescription* RTCPeerConnection::remoteDescription()
{
    libwebrtc::scoped_refptr<libwebrtc::RTCSessionDescription> description;
    libwebrtc::SdpParseError error;
    char* sdp_ptr = nullptr;
    char* type_ptr = nullptr;

    m_backend->GetRemoteDescription(
        [&sdp_ptr, &type_ptr](const char* sdp, const char* type) {
            sdp_ptr = const_cast<char*>(sdp);
            type_ptr = const_cast<char*>(type);
        },
        [](const std::string& error) {});

    if (sdp_ptr && type_ptr) {
        description =
            libwebrtc::RTCSessionDescription::Create(type_ptr, sdp_ptr, &error);
        return new RTCSessionDescription(executionContext(), description);
    }
    return nullptr;
}

RTCSessionDescription* RTCPeerConnection::currentRemoteDescription()
{
    // TODO:FIX ME!!
    STARFISH_UNIMPLEMENTED();
    return remoteDescription();
}

RTCSessionDescription* RTCPeerConnection::pendingRemoteDescription()
{
    // TODO:FIX ME!!
    STARFISH_UNIMPLEMENTED();
    return remoteDescription();
}

// https://w3c.github.io/webrtc-pc/#dom-peerconnection-addicecandidate
Promise* RTCPeerConnection::addIceCandidate(RTCIceCandidateInit init)
{
    Promise* promise = new Promise(scriptBindingInstance());

    // 1-3
    if (!init.m_candidate->equals(String::emptyString) &&
        !init.m_sdpMid.hasValue() && !init.m_sdpMLineIndex.hasValue()) {
        auto exception = new DOMException(
            executionContext(), DOMException::SCRIPT_TYPE_ERR, "TypeError");
        promise->reject(exception->scriptValue());
        return promise;
    }

    // 4.1
    backend()->GetRemoteDescription([](const char* sdp, const char* type) {},
                                    [](const std::string& error) {
                                        // auto exception = new
                                        // DOMException(executionContext(),
                                        //                                   DOMException::INVALID_STATE_ERR,
                                        //                                   "InvalidStateError");
                                        // promise->reject(exception->scriptValue());
                                        // return promise;
                                    });

    if (init.m_candidate->equals(String::emptyString) &&
        !init.m_sdpMid.hasValue() && !init.m_sdpMLineIndex.hasValue()) {
        auto exception = new DOMException(
            executionContext(), DOMException::SCRIPT_TYPE_ERR, "TypeError");
        promise->reject(exception->scriptValue());
        return promise;
    }

    std::string sdpMid;
    if (init.m_sdpMid.hasValue()) {
        sdpMid = init.m_sdpMid.getValue()->toUTF8NonGCString();
    }

    uint32_t sdpMLineIndex = 0;
    if (init.m_sdpMLineIndex.hasValue()) {
        sdpMLineIndex = init.m_sdpMLineIndex.getValue();
    }

    libwebrtc::SdpParseError error;
    std::string candidate;
    if (init.m_candidate) {
        candidate = init.m_candidate->toUTF8NonGCString();
    }

    if (init.m_candidate->equals(String::emptyString)) {
        // TODO: add a:end-of-candidates to sdp
        auto exception = new DOMException(executionContext(),
                                          DOMException::NOT_SUPPORTED_ERR,
                                          "NotSupportedError");
        promise->reject(exception->scriptValue());
        return promise;
    }

    libwebrtc::scoped_refptr<libwebrtc::RTCIceCandidate> rtc_candidate =
        libwebrtc::RTCIceCandidate::Create(candidate.c_str(), sdpMid.c_str(),
                                           sdpMLineIndex, &error);

    if (rtc_candidate.get() == nullptr) {
        auto exception = new DOMException(
            executionContext(), String::createASCIIString("OperationError"),
            String::createASCIIString("OperationError"));

        promise->reject(exception->scriptValue());
        return promise;
    }

    backend()->AddCandidate(rtc_candidate->sdp_mid(),
                            rtc_candidate->sdp_mline_index(),
                            rtc_candidate->candidate());
    promise->fulfill(scriptUndefined());
    return promise;
}

String* RTCPeerConnection::signalingState()
{
    if (isClosed()) {
        return String::emptyString;
    }

    switch (m_backend->signaling_state()) {
    case libwebrtc::RTCSignalingState::RTCSignalingStateStable:
        return String::createASCIIString("stable");
    case libwebrtc::RTCSignalingState::RTCSignalingStateHaveLocalOffer:
        return String::createASCIIString("have-local-offer");
    case libwebrtc::RTCSignalingState::RTCSignalingStateHaveRemoteOffer:
        return String::createASCIIString("have-remote-offer");
    case libwebrtc::RTCSignalingState::RTCSignalingStateHaveLocalPrAnswer:
        return String::createASCIIString("have-local-pranswer");
    case libwebrtc::RTCSignalingState::RTCSignalingStateHaveRemotePrAnswer:
        return String::createASCIIString("have-remote-pranswer");
    case libwebrtc::RTCSignalingState::RTCSignalingStateClosed:
        return String::createASCIIString("closed");
    default:
        return String::emptyString;
    }
}

String* RTCPeerConnection::iceGatheringState()
{
    switch (m_backend->ice_gathering_state()) {
    case libwebrtc::RTCIceGatheringState::RTCIceGatheringStateNew:
        return String::createASCIIString("new");
    case libwebrtc::RTCIceGatheringState::RTCIceGatheringStateGathering:
        return String::createASCIIString("gathering");
    case libwebrtc::RTCIceGatheringState::RTCIceGatheringStateComplete:
        return String::createASCIIString("complete");
    default:
        return String::emptyString;
    }
}

String* RTCPeerConnection::iceConnectionState()
{
    switch (m_backend->ice_connection_state()) {
    case libwebrtc::RTCIceConnectionState::RTCIceConnectionStateClosed:
        return String::createASCIIString("closed");
    case libwebrtc::RTCIceConnectionState::RTCIceConnectionStateFailed:
        return String::createASCIIString("failed");
    case libwebrtc::RTCIceConnectionState::RTCIceConnectionStateDisconnected:
        return String::createASCIIString("disconnected");
    case libwebrtc::RTCIceConnectionState::RTCIceConnectionStateNew:
        return String::createASCIIString("new");
    case libwebrtc::RTCIceConnectionState::RTCIceConnectionStateChecking:
        return String::createASCIIString("checking");
    case libwebrtc::RTCIceConnectionState::RTCIceConnectionStateCompleted:
        return String::createASCIIString("completed");
    case libwebrtc::RTCIceConnectionState::RTCIceConnectionStateConnected:
        return String::createASCIIString("connected");
    default:
        return String::emptyString;
    }
}

String* RTCPeerConnection::connectionState()
{
    switch (m_backend->peer_connection_state()) {
    case libwebrtc::RTCPeerConnectionState::RTCPeerConnectionStateClosed:
        return String::createASCIIString("closed");
    case libwebrtc::RTCPeerConnectionState::RTCPeerConnectionStateFailed:
        return String::createASCIIString("failed");
    case libwebrtc::RTCPeerConnectionState::RTCPeerConnectionStateDisconnected:
        return String::createASCIIString("disconnected");
    case libwebrtc::RTCPeerConnectionState::RTCPeerConnectionStateNew:
        return String::createASCIIString("new");
    case libwebrtc::RTCPeerConnectionState::RTCPeerConnectionStateConnecting:
        return String::createASCIIString("connecting");
    case libwebrtc::RTCPeerConnectionState::RTCPeerConnectionStateConnected:
        return String::createASCIIString("connected");
    default:
        return String::emptyString;
    }
}

GCVector<RTCIceServer> RTCPeerConnection::getDefaultIceServers()
{
    // FIXME: IceServer is browser specific. Update as
    // IceServers become available
    STARFISH_UNIMPLEMENTED();
    GCVector<RTCIceServer> iceServers;
    return std::move(iceServers);
}

RTCConfiguration RTCPeerConnection::getConfiguration()
{
    return m_configuration;
}

void RTCPeerConnection::setConfiguration(RTCConfiguration& configuration,
                                         bool checkStatus)
{
    if (checkStatus && isClosed()) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }

    if (!configuration.isValid()) {
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_TYPE_ERR, "TypeError");
    }

    if (!configuration.hasIceServers()) {
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_TYPE_ERR, "TypeError");
    }

    // 1-3
    if ((configuration.peerIdentity() != nullptr) &&
        !(m_configuration.peerIdentity()->equals(
            configuration.peerIdentity()))) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_MODIFICATION_ERR,
                               "InvalidModificationError");
    }

    // 4
    if (!configuration.certificates().empty()) {
        if (m_configuration.certificates().size() !=
            configuration.certificates().size()) {
            throw new DOMException(executionContext(),
                                   DOMException::INVALID_MODIFICATION_ERR,
                                   "InvalidModificationError");
        }

        for (size_t i = 0; i < configuration.certificates().size(); i++) {
            if (!configuration.certificates()[i]->equals(
                    m_configuration.certificates()[i])) {
                throw new DOMException(executionContext(),
                                       DOMException::INVALID_MODIFICATION_ERR,
                                       "InvalidModificationError");
            }
        }
    }

    // 5-6
    if (!configuration.isValid()) {
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_TYPE_ERR, "TypeError");
    }

    // 5
    if (m_configuration.hasBundlePolicy() &&
        (m_configuration.m_bundlePolicy != configuration.m_bundlePolicy)) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_MODIFICATION_ERR,
                               "InvalidModificationError");
    }
    // 6
    if (m_configuration.hasRtcpMuxPolicy() &&
        (m_configuration.m_rtcpMuxPolicy != configuration.m_rtcpMuxPolicy)) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_MODIFICATION_ERR,
                               "InvalidModificationError");
    }
    if (configuration.hasRtcpMuxPolicy() &&
        (configuration.m_rtcpMuxPolicy == RTCRtcpMuxPolicy::Negotiate)) {
        // TODO: Support non-muxed RTCP
        throw new DOMException(executionContext(),
                               DOMException::NOT_SUPPORTED_ERR,
                               "NotSupportedError");
    }

    // 7 - 10: TODO

    // 11
    if (configuration.hasIceServers()) {
        for (RTCIceServer& server : configuration.iceServers()) {
            GCVector<String*> urls = server.m_urls;
            for (String* u : urls) {
                String* url = u->trim()->toLower();
                if (url->startsWith("turn") || url->startsWith("turns")) {
                    if (!server.hasUsername()) {
                        if (!server.m_credential.isNoneValue()) {
                            throw new DOMException(
                                executionContext(),
                                DOMException::INVALID_ACCESS_ERR,
                                "InvalidAccessError");
                        }
                    } else if (!server.m_username->equals(
                                   String::emptyString)) {
                        if (server.m_credential.isNoneValue()) {
                            throw new DOMException(
                                executionContext(),
                                DOMException::INVALID_ACCESS_ERR,
                                "InvalidAccessError");
                        }
                    }

                    if ((server.m_credentialType ==
                         RTCIceCredentialType::Password) &&
                        !server.m_credential.isDOMStringValue()) {
                        throw new DOMException(executionContext(),
                                               DOMException::INVALID_ACCESS_ERR,
                                               "InvalidAccessError");
                    }
                    if ((server.m_credentialType ==
                         RTCIceCredentialType::OAuth) &&
                        !server.m_credential.isRTCOAuthCredentialValue()) {
                        throw new DOMException(executionContext(),
                                               DOMException::INVALID_ACCESS_ERR,
                                               "InvalidAccessError");
                    }
                } else if (url->startsWith("stun") ||
                           url->startsWith("stuns")) {
                } else if (url->startsWith("relative-url") ||
                           url->startsWith("http:") ||
                           url->startsWith("https:")) {
                    throw new DOMException(executionContext(),
                                           DOMException::SYNTAX_ERR,
                                           "SyntaxError");
                }
            }
            if (!server.hasValidCredentialType()) {
                throw new DOMException(executionContext(),
                                       DOMException::SCRIPT_TYPE_ERR,
                                       "TypeError");
            }
        }
    }

    // 12
    m_configuration = configuration;
}

void RTCPeerConnection::close()
{
    if (isClosed()) {
        return;
    }

    m_closed = true;
    if (m_backend) {
        m_backend->Close();
    }
}

DEFINE_EVENT_LISTENER(RTCPeerConnection, negotiationneeded);
DEFINE_EVENT_LISTENER(RTCPeerConnection, icecandidate);
DEFINE_EVENT_LISTENER(RTCPeerConnection, icecandidateerror);
DEFINE_EVENT_LISTENER(RTCPeerConnection, signalingstatechange);
DEFINE_EVENT_LISTENER(RTCPeerConnection, iceconnectionstatechange);
DEFINE_EVENT_LISTENER(RTCPeerConnection, icegatheringstatechange);
DEFINE_EVENT_LISTENER(RTCPeerConnection, connectionstatechange);
DEFINE_EVENT_LISTENER(RTCPeerConnection, datachannel);
DEFINE_EVENT_LISTENER(RTCPeerConnection, track);

RTCSctpTransport* RTCPeerConnection::sctp()
{
    return nullptr;
    // TODO:FIX ME!
    // return new RTCSctpTransport(executionContext(), sctp);
}

// https://w3c.github.io/webrtc-pc/#dom-peerconnection-createdatachannel
RTCDataChannel* RTCPeerConnection::createDataChannel(
    String* label, RTCDataChannelInit dataChannelDict)
{
    // 1-4
    if (isClosed()) {
        STARFISH_LOG_ERROR("connection closed");
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "connection closed");
    }
    // 5-15
    if ((label->length() > 65535)) {
        STARFISH_LOG_ERROR("label.length() > 65535");
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_TYPE_ERR,
                               "label.length() > 65535");
    }
    if (!dataChannelDict.negotiated() &&
        (dataChannelDict.protocol()->length() > 65535)) {
        STARFISH_LOG_ERROR("protocol.length() > 65535");
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_TYPE_ERR,
                               "protocol.length() > 65535");
    }

    libwebrtc::RTCDataChannelInit init;
    if (dataChannelDict.hasMaxPacketLifeTime()) {
        init.maxRetransmitTime = dataChannelDict.maxPacketLifeTime();
    }
    if (dataChannelDict.hasMaxRetransmits()) {
        init.maxRetransmits = dataChannelDict.maxRetransmits();
    }
    init.ordered = dataChannelDict.ordered();
    init.protocol =
        std::string(dataChannelDict.protocol()->toUTF8NonGCString().data());
    init.negotiated = dataChannelDict.negotiated();
    if (dataChannelDict.hasId() && dataChannelDict.negotiated()) {
        init.id = dataChannelDict.id();
    }
    if (dataChannelDict.negotiated() && !dataChannelDict.hasId()) {
        STARFISH_LOG_ERROR("negotiated=true but id=null");
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_TYPE_ERR,
                               "negotiated=true but id=null");
    }
    if (dataChannelDict.hasMaxPacketLifeTime() &&
        dataChannelDict.hasMaxRetransmits()) {
        STARFISH_LOG_ERROR(
            "both maxPacketLifeTime=true and maxRetransmits=true");
        throw new DOMException(
            executionContext(), DOMException::SCRIPT_TYPE_ERR,
            "both maxPacketLifeTime=true and maxRetransmits=true");
    }
    if (dataChannelDict.hasId() && (dataChannelDict.id() >= 65535)) {
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_TYPE_ERR, "id >= 65535");
    }

    libwebrtc::scoped_refptr<libwebrtc::RTCDataChannel> dataChannel =
        m_backend->CreateDataChannel(
            std::string(label->toUTF8NonGCString().data()), &init);

    if (!dataChannel) {
        STARFISH_LOG_ERROR("Invalid configuration");
        throw new DOMException(executionContext(), DOMException::DOM_EXCEPTION,
                               "Invalid configuration");
    }

    RTCDataChannel* rtcDataChannel = new RTCDataChannel(
        executionContext(), this, dataChannelDict, dataChannel);
    m_dataChannels.push_back(rtcDataChannel);

    return rtcDataChannel;
}

GCVector<RTCRtpSender*> RTCPeerConnection::getSenders()
{
    GCVector<RTCRtpSender*> senders;

    if (!m_backend) {
        return std::move(senders);
    }

    for (auto transceiver : m_transceivers) {
        senders.push_back(transceiver->sender());
    }

    libwebrtc::vector<libwebrtc::scoped_refptr<libwebrtc::RTCRtpSender>>
        backendSenders = backend()->senders();
    STARFISH_ASSERT(senders.size() == backendSenders.size());

    return std::move(senders);
}

GCVector<RTCRtpReceiver*> RTCPeerConnection::getReceivers()
{
    GCVector<RTCRtpReceiver*> receivers;

    if (!m_backend) {
        return std::move(receivers);
    }

    for (auto transceiver : m_transceivers) {
        receivers.push_back(transceiver->receiver());
    }

    libwebrtc::vector<libwebrtc::scoped_refptr<libwebrtc::RTCRtpReceiver>>
        backendReceivers = m_backend->receivers();
    STARFISH_ASSERT(receivers.size() == backendReceivers.size());

    return std::move(receivers);
}

GCVector<RTCRtpTransceiver*> RTCPeerConnection::getTransceivers()
{
    return m_transceivers;
}

// https://w3c.github.io/webrtc-pc/#dom-rtcpeerconnection-addtrack
RTCRtpSender* RTCPeerConnection::addTrack(MediaStreamTrack* track,
                                          GCVector<MediaStream*>& streams)
{
    // 5
    if (isClosed()) {
        STARFISH_LOG_ERROR("InvalidStateError");
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }

    // 1-4
    std::vector<std::string> streamIds;
    for (auto stream : streams) {
        streamIds.push_back(stream->backend()->id().std_string());
    }

    // 6
    std::string id = "";
    if (track->isAudioStreamTrack()) {
        id = track->asAudioStreamTrack()->backend()->id().std_string();
    } else if (track->isVideoStreamTrack()) {
        id = track->asVideoStreamTrack()->backend()->id().std_string();
    }

    // 7
    GCVector<RTCRtpSender*> senders;
    RTCRtpSender* senderToReturn = nullptr;
    RTCRtpTransceiver* transceiverWithSender = nullptr;
    for (auto transceiver : m_transceivers) {
        if (!transceiver->stopped()) {
            senders.push_back(transceiver->sender());

            if (transceiver->sender()->track() &&
                transceiver->sender()->track()->id() == id) {
                throw new DOMException(executionContext(),
                                       DOMException::INVALID_ACCESS_ERR,
                                       "InvalidAccessErr");
            }

            RTCRtpSender* sender = transceiver->sender();
            if (!sender->track() &&
                (transceiver->receiver()->track()->kind() == track->kind()) &&
                !transceiver->sentBefore()) {
                senderToReturn = sender;
            }
        }
    }

    // 8
    if (senderToReturn) {
        senderToReturn->setTrack(track);
        senderToReturn->setStreams(streams);

        RTCRtpTransceiver* transceiverWithSender = nullptr;
        for (auto transceiver : m_transceivers) {
            if (transceiver->sender() == senderToReturn) {
                transceiverWithSender = transceiver;
                break;
            }
        }

        if (transceiverWithSender) {
            if (transceiverWithSender->direction() ==
                RTCRtpTransceiverDirection::Recvonly) {
                transceiverWithSender->setDirection(
                    RTCRtpTransceiverDirection::Sendrecv);
            } else if (transceiverWithSender->direction() ==
                       RTCRtpTransceiverDirection::Inactive) {
                transceiverWithSender->setDirection(
                    RTCRtpTransceiverDirection::Sendonly);
            }
        }
    } else { // 9
        std::vector<libwebrtc::string> stream_ids;
        for (auto id : streamIds) {
            stream_ids.push_back(id.c_str());
        }

        libwebrtc::scoped_refptr<libwebrtc::RTCRtpSender> r;
        if (track->isAudioStreamTrack()) {
            r = backend()->AddTrack(track->asAudioStreamTrack()->backend(),
                                    stream_ids);
        } else if (track->isVideoStreamTrack()) {
            r = backend()->AddTrack(track->asVideoStreamTrack()->backend(),
                                    stream_ids);
        }

        if (!r.get()) {
            STARFISH_LOG_ERROR("Failed to add an audio/video track: %s", "");
            throw new DOMException(executionContext(),
                                   DOMException::INVALID_ACCESS_ERR,
                                   "InvalidAccessErr");
        }

        senderToReturn = getSender(r);
        senderToReturn->setTrack(track);
        STARFISH_ASSERT(senderToReturn);
    }
    return senderToReturn;
}

// https://w3c.github.io/webrtc-pc/#dom-rtcpeerconnection-removetrack
void RTCPeerConnection::removeTrack(RTCRtpSender* sender)
{
    // 1-3
    if (isClosed()) {
        STARFISH_LOG_ERROR("InvalidStateError");
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }

    // 4-6
    RTCRtpSender* existingSender = nullptr;
    RTCRtpSender* aliveSender = nullptr;
    RTCRtpTransceiver* existingTransceiver = nullptr;

    for (auto transceiver : getTransceivers()) {
        if (transceiver->sender() == sender) {
            existingSender = transceiver->sender();
            if (!transceiver->stopped()) {
                aliveSender = transceiver->sender();
                existingTransceiver = transceiver;
            }
        }
    }

    if (!existingSender) {
        STARFISH_LOG_ERROR("InvalidAccessError");
        throw new DOMException(executionContext(),
                               DOMException::INVALID_ACCESS_ERR,
                               "InvalidAccessError");
    }

    if (!aliveSender) {
        return;
    }

    // 7
    if (!aliveSender->track()) {
        return;
    }

    m_backend->RemoveTrack(sender->backend());
    aliveSender->setTrack(nullptr);

    if (!existingTransceiver) {
        STARFISH_LOG_ERROR("Transceiver not exist");
        return;
    }

    if (existingTransceiver->direction() ==
        RTCRtpTransceiverDirection::Sendrecv) {
        existingTransceiver->setDirection(RTCRtpTransceiverDirection::Recvonly);
    } else if (existingTransceiver->direction() ==
               RTCRtpTransceiverDirection::Sendonly) {
        existingTransceiver->setDirection(RTCRtpTransceiverDirection::Inactive);
    }
}

RTCRtpTransceiver* RTCPeerConnection::addTransceiver(
    DOMStringOrMediaStreamTrack trackOrKind, RTCRtpTransceiverInit init)
{
    if (isClosed()) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }

    String* kind = nullptr;
    MediaStreamTrack* track = nullptr;
    if (trackOrKind.isDOMStringValue()) {
        kind = trackOrKind.getDOMStringValue();
        if (!kind->equals("audio") && !kind->equals("video")) {
            throw new DOMException(executionContext(),
                                   DOMException::SCRIPT_TYPE_ERR,
                                   "ScriptTypeError");
        }
    } else if (trackOrKind.isMediaStreamTrackValue()) {
        track = trackOrKind.getMediaStreamTrackValue();
    }

    libwebrtc::scoped_refptr<libwebrtc::RTCRtpTransceiver> r;
    if ((kind && kind->equals("audio")) ||
        (track && track->kind() == MediaStreamTrack::Kind::Audio)) {
        r = m_backend->AddTransceiver(track->asAudioStreamTrack()->backend(),
                                      init.toRtpTransceiverInit());
    } else if ((kind && kind->equals("video")) ||
               (track && track->kind() == MediaStreamTrack::Kind::Video)) {
        r = m_backend->AddTransceiver(track->asVideoStreamTrack()->backend(),
                                      init.toRtpTransceiverInit());
    }

    if (!r.get()) {
        STARFISH_LOG_ERROR("%s: internal error", __func__);
        throw new DOMException(executionContext(), DOMException::DOM_EXCEPTION,
                               "addTransceiver: internal error");
    }

    RTCRtpTransceiver* transceiver = getTransceiver(r);

    if (track) {
        transceiver->sender()->setTrack(track);
    }

    return transceiver;
}

// https://w3c.github.io/webrtc-pc/#widl-RTCPeerConnection-getStats-Promise-RTCStatsReport--MediaStreamTrack-selector
#if 0 // Chrome specific?
Promise* RTCPeerConnection::getStats(MediaStreamTrack* selector)
{
    Promise* promise = new Promise(scriptBindingInstance());

    if (selector) {
        int count = 0;
        for (auto transceiver : m_transceivers) {
            if (transceiver->sender()->track() == selector) {
                count++;
            } else if (transceiver->receiver()->track() == selector) {
                count++;
            }
        }

        if (count != 1) {
            STARFISH_LOG_ERROR("%s: The track does not exist in this pc",
                               __func__);
            auto exception = new DOMException(
                executionContext(), DOMException::INVALID_ACCESS_ERR,
                "The track does not exist in the pc");
            promise->reject(exception->scriptValue());
            return promise;
        }
    }
    promise->fulfill(scriptUndefined());
    return promise;
}
#endif

libwebrtc::scoped_refptr<libwebrtc::RTCPeerConnection>
RTCPeerConnection::backend()
{
    return m_backend;
}

bool RTCPeerConnection::isClosed()
{
    if (m_closed) {
        return true;
    }

    if (!m_backend) {
        return true;
    }

    if (m_backend->peer_connection_state() ==
        libwebrtc::RTCPeerConnectionState::RTCPeerConnectionStateClosed) {
        return true;
    }
    return false;
}

RTCSdpType RTCPeerConnection::toRtcSdpType(std::string type)
{
    RTCSdpType result;
    if (type.compare("offer") == 0) {
        result = RTCSdpType::Offer;
    } else if (type.compare("pranswer") == 0) {
        result = RTCSdpType::Pranswer;
    } else if (type.compare("answer") == 0) {
        result = RTCSdpType::Answer;
    } else if (type.compare("rollback") == 0) {
        result = RTCSdpType::Rollback;
    }
    return result;
}

Nullable<libwebrtc::RTCSessionDescription::SdpType>
RTCPeerConnection::toSdpType(Nullable<RTCSdpType> type)
{
    RTCSessionDescriptionInit init(type, String::emptyString);
    return init.toSdpType();
}

DOMException* RTCPeerConnection::toDomException(std::string error)
{
    return new DOMException(m_executionContext, DOMException::DOM_EXCEPTION,
                            error.c_str());
}

bool RTCPeerConnection::isValidRemoteState(RTCSdpType type)
{
    libwebrtc::RTCSignalingState state = m_backend->signaling_state();
    if (type == RTCSdpType::Offer) {
        if ((state == libwebrtc::RTCSignalingState::RTCSignalingStateStable) ||
            (state ==
             libwebrtc::RTCSignalingState::RTCSignalingStateHaveRemoteOffer)) {
            return true;
        }
    } else if ((type == RTCSdpType::Pranswer) || (type == RTCSdpType::Answer)) {
        if ((state ==
             libwebrtc::RTCSignalingState::RTCSignalingStateHaveLocalOffer) ||
            (state == libwebrtc::RTCSignalingState::
                          RTCSignalingStateHaveRemotePrAnswer)) {
            return true;
        }
    }

    return false;
}
} // namespace Starfish

#endif
