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
#include "core/modules/mediastream/RTCError.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/WebBase.h"
#include "core/page/GlobalScope.h"

#include "api/rtp_transceiver_interface.h"
#include "api/sctp_transport_interface.h"
#include "rtc_base/physical_socket_server.h"
#include "rtc_base/strings/json.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"

namespace Starfish {

PeerConnectionObserver::PeerConnectionObserver()
    : PeerConnectionObserver(nullptr)
{
}

PeerConnectionObserver::PeerConnectionObserver(
    RTCPeerConnection* peerConnection)
    : m_peerConnection(peerConnection)
{
}

void PeerConnectionObserver::OnSignalingChange(
    webrtc::PeerConnectionInterface::SignalingState new_state)
{
    struct Params {
        PeerConnectionObserver* self;
    };

    Params* p = new Params();
    p->self = this;

    m_peerConnection->executionContext()
        ->webBase()
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            m_peerConnection->executionContext()->globalScope(),
            [](size_t, void* data) {
                Params* p = (Params*)data;
                PeerConnectionObserver* self = p->self;

                String* eventType =
                    self->m_peerConnection->m_executionContext->starfish()
                        ->staticStrings()
                        ->m_signalingstatechange.localName();
                Event* e = new Event(self->m_peerConnection->m_executionContext,
                                     eventType);
                self->m_peerConnection->dispatchEventByUA(e);
                delete p;
            },
            p);
}

void PeerConnectionObserver::OnAddStream(
    rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
}

void PeerConnectionObserver::OnTrack(
    rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track =
        transceiver->receiver()->track();
    std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> streams =
        transceiver->receiver()->streams();

    struct Params {
        PeerConnectionObserver* self;
        rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrack;
        std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> streams;
    };

    Params* p = new Params();
    p->self = this;
    p->streams = streams;

    std::string videoKind(track->kVideoKind);
    std::string audioKind(track->kAudioKind);
    if (track->kind() == videoKind) {
        webrtc::VideoTrackInterface* videoTrack =
            (webrtc::VideoTrackInterface*)track.get();
        p->videoTrack = m_peerConnection->executionContext()
                            ->document()
                            ->window()
                            ->navigator()
                            ->webRtcManager()
                            ->peerConnectionFactory()
                            ->CreateVideoTrack("asdf", videoTrack->GetSource());
    } else if (track->kind() == audioKind) {
        STARFISH_LOG_WARN("%s: AudioTrack not yet supported.\n", __func__);
        return;
    }

    m_peerConnection->executionContext()
        ->webBase()
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            m_peerConnection->executionContext()->globalScope(),
            [](size_t, void* data) {
                Params* p = (Params*)data;
                PeerConnectionObserver* self = p->self;
                rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrack =
                    p->videoTrack;

                VideoStreamTrack* videoStreamTrack = new VideoStreamTrack(
                    self->m_peerConnection->executionContext(), videoTrack);

                GCVector<MediaStream*> streams;
                for (size_t i = 0; i < p->streams.size(); i++) {
                    MediaStream* stream = new MediaStream(
                        self->m_peerConnection->executionContext(),
                        p->streams[i]);
                    stream->addTrack(videoStreamTrack);
                    streams.push_back(stream);
                }

                String* eventType =
                    self->m_peerConnection->m_executionContext->starfish()
                        ->staticStrings()
                        ->m_track.localName();
                RTCTrackEventInit init(videoStreamTrack, streams);
                RTCTrackEvent* e = new RTCTrackEvent(
                    self->m_peerConnection->m_executionContext, eventType,
                    init);
                self->m_peerConnection->dispatchEventByUA(e);
                delete p;
            },
            p);
}

void PeerConnectionObserver::OnIceCandidate(
    const webrtc::IceCandidateInterface* candidate)
{
    struct Params {
        PeerConnectionObserver* self;
        std::string sdpMid;
        int sdpMlineIndex;
        std::string sdp;
        webrtc::IceCandidateInterface* candidate;
    };

    std::string sdp;
    candidate->ToString(&sdp);

    Params* p = new Params();
    p->self = this;
    p->sdpMid = candidate->sdp_mid();
    p->sdpMlineIndex = candidate->sdp_mline_index();
    p->sdp = std::move(sdp);

    m_peerConnection->executionContext()
        ->webBase()
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            m_peerConnection->executionContext()->globalScope(),
            [](size_t, void* data) {
                Params* p = (Params*)data;
                PeerConnectionObserver* self = p->self;
                webrtc::IceCandidateInterface* candidate =
                    webrtc::CreateIceCandidate(p->sdpMid, p->sdpMlineIndex,
                                               p->sdp, nullptr);

                RTCIceCandidate* cand = new RTCIceCandidate(
                    self->m_peerConnection->executionContext(), candidate);

                RTCPeerConnectionIceEventInit init;
                init.m_candidate = cand;

                String* eventType =
                    self->m_peerConnection->m_executionContext->starfish()
                        ->staticStrings()
                        ->m_icecandidate.localName();
                RTCPeerConnectionIceEvent* e = new RTCPeerConnectionIceEvent(
                    self->m_peerConnection->m_executionContext, eventType,
                    init);
                self->m_peerConnection->dispatchEventByUA(e);
                delete p;
            },
            p);
}

CreateOfferObserver* CreateOfferObserver::create(
    RTCPeerConnection* peerConnection, Promise* promise)
{
    CreateOfferObserver* observer =
        new rtc::RefCountedObject<CreateOfferObserver>();
    observer->init(peerConnection, promise);
    return observer;
}

CreateAnswerObserver* CreateAnswerObserver::create(
    RTCPeerConnection* peerConnection, Promise* promise)
{
    CreateAnswerObserver* observer =
        new rtc::RefCountedObject<CreateAnswerObserver>();
    observer->init(peerConnection, promise);
    return observer;
}

void CreateOfferAnswerObserver::init(RTCPeerConnection* peerConnection,
                                     Promise* promise)
{
    m_peerConnection = peerConnection;
    m_promise = promise;
}

void CreateOfferAnswerObserver::OnSuccess(
    webrtc::SessionDescriptionInterface* desc)
{
    STARFISH_ASSERT(m_promise);
    STARFISH_LOG_INFO("CreateSessionDescriptionObserver::%s\n", __func__);

    // This callback is called from another thread
    // The following lines must be executed in the main thread.
    struct Params {
        CreateOfferAnswerObserver* self;
        std::unique_ptr<webrtc::SessionDescriptionInterface> desc;
        Promise* promise;
    };

    Params* p = new Params();
    p->self = this;
    // The ownership is also passed on
    p->desc = std::unique_ptr<webrtc::SessionDescriptionInterface>(desc);
    p->promise = m_promise;

    m_peerConnection->executionContext()
        ->webBase()
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            m_peerConnection->executionContext()->globalScope(),
            [](size_t, void* data) {
                Params* p = (Params*)data;
                CreateOfferAnswerObserver* self = p->self;
                webrtc::SessionDescriptionInterface* desc = p->desc.get();
                Promise* promise = p->promise;

                RTCSdpType type =
                    self->m_peerConnection->toRtcSdpType(desc->GetType());
                std::string sdpString;
                desc->ToString(&sdpString);

                ObjectRef* sd =
                    self->m_peerConnection->createSessionDescriptionInitObject(
                        type, String::createASCIIString(sdpString.c_str(),
                                                        sdpString.size()));
                if (self->isCreateOffer()) {
                    self->m_peerConnection->m_lastCreatedOffer = sdpString;
                } else if (self->isCreateAnswer()) {
                    self->m_peerConnection->m_lastCreatedAnswer = sdpString;
                }

                promise->fulfill(createScriptValue(sd));
                self->setPromise(nullptr);
                delete promise;
                delete p;
            },
            p);
}

void CreateOfferAnswerObserver::OnFailure(webrtc::RTCError error)
{
    STARFISH_ASSERT(m_promise);
    STARFISH_LOG_WARN("CreateSessionDescriptionObserver::%s\n", __func__);

    // This callback is called from another thread
    // The following lines must be executed in the main thread.
    struct Params {
        CreateOfferAnswerObserver* self;
        webrtc::RTCError error;
        Promise* promise;
    };

    Params* p = new Params();
    p->self = this;
    p->error = std::move(error);
    p->promise = m_promise;

    m_peerConnection->executionContext()
        ->webBase()
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            m_peerConnection->executionContext()->globalScope(),
            [](size_t, void* data) {
                Params* p = (Params*)data;
                CreateOfferAnswerObserver* self = p->self;
                Promise* promise = p->promise;

                DOMException* exception =
                    self->m_peerConnection->toDomException(std::move(p->error));
                STARFISH_ASSERT(exception);
                promise->reject(exception->scriptValue());
                self->setPromise(nullptr);
                delete promise;
                delete p;
            },
            p);
}

SetLocalRemoteDescriptionObserver* SetLocalRemoteDescriptionObserver::create(
    RTCPeerConnection* peerConnection, Promise* promise)
{
    SetLocalRemoteDescriptionObserver* observer =
        new rtc::RefCountedObject<SetLocalRemoteDescriptionObserver>();
    observer->m_peerConnection = peerConnection;
    observer->m_promise = promise;
    return observer;
}

void SetLocalRemoteDescriptionObserver::OnSuccess()
{
    STARFISH_ASSERT(m_promise);
    STARFISH_LOG_INFO("SetSessionDescriptionObserver::%s\n", __func__);

    m_peerConnection->executionContext()
        ->webBase()
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            m_peerConnection->executionContext()->globalScope(),
            [](size_t, void* data) {
                Promise* promise = (Promise*)data;
                promise->fulfill(scriptUndefined());
                delete promise;
            },
            m_promise);
}

void SetLocalRemoteDescriptionObserver::OnFailure(webrtc::RTCError error)
{
    STARFISH_ASSERT(m_promise);
    STARFISH_LOG_WARN("SetSessionDescriptionObserver::%s\n", __func__);

    // This callback is called from another thread
    // The following lines must be executed in the main thread.
    struct Params {
        SetLocalRemoteDescriptionObserver* self;
        webrtc::RTCError error;
        Promise* promise;
    };

    Params* p = new Params();
    p->self = this;
    p->error = std::move(error);
    p->promise = m_promise;

    m_peerConnection->executionContext()
        ->webBase()
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            m_peerConnection->executionContext()->globalScope(),
            [](size_t, void* data) {
                Params* p = (Params*)data;
                SetLocalRemoteDescriptionObserver* self = p->self;
                Promise* promise = p->promise;

                DOMException* exception =
                    self->m_peerConnection->toDomException(std::move(p->error));
                STARFISH_ASSERT(exception);
                promise->reject(exception->scriptValue());
                self->setPromise(nullptr);
                delete p;
            },
            p);
}

// https://w3c.github.io/webrtc-pc/#constructor
RTCPeerConnection::RTCPeerConnection(ExecutionContext* executionContext,
                                     RTCConfiguration configuration)
    : EventTarget()
    , m_executionContext(executionContext)
{
    if (!configuration.certificates().empty()) {
        // TODO
    } else {
    }

    if (!configuration.isValid()) {
        throw new DOMException(executionContext, DOMException::SCRIPT_TYPE_ERR,
                               "TypeError");
    }

    if (!initializePeerConnection(configuration)) {
        deletePeerConnection();
        STARFISH_LOG_ERROR("%s: PeerConnection: failed\n", __func__);
        throw new DOMException(executionContext, DOMException::DOM_EXCEPTION,
                               "UnknownError");
    }

    // 9--11
    m_configuration = configuration;

    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj,
                 void* cd) { ((RTCPeerConnection*)obj)->~RTCPeerConnection(); },
        NULL, NULL, NULL);
}

bool RTCPeerConnection::initializePeerConnection()
{
    return initializePeerConnection(m_configuration);
}

bool RTCPeerConnection::initializePeerConnection(
    RTCConfiguration& configuration)
{
    m_peerConnectionObserver = std::unique_ptr<PeerConnectionObserver>(
        new PeerConnectionObserver(this));

    WebRtcManager* webRtcManager = this->executionContext()
                                       ->document()
                                       ->window()
                                       ->navigator()
                                       ->webRtcManager();
    STARFISH_ASSERT(webRtcManager->peerConnectionFactory());

    bool dtls = true;
    webrtc::PeerConnectionInterface::RTCConfiguration config =
        configuration.backend();
    config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
    config.enable_rtp_data_channel = true;
    config.enable_dtls_srtp = dtls;
    config.set_dscp(false);

    // NOTE: libwebrtc still uses uri internally, and when it is empty
    // random crash occurs.
    // for testing server.uri = "stun:stun.l.google.com:19302" can be used
    for (auto& server : config.servers) {
        if (server.uri.empty() && !server.urls.empty()) {
            server.uri = server.urls[0];
            server.urls.clear();
        }
    }

    // FIXME: Due to a bug in the binding generator, iceservers cannot be
    // obtained. Use the default server
    webrtc::PeerConnectionInterface::IceServer server;
    server.uri = m_stun;
    config.servers.push_back(server);

    webrtc::PeerConnectionDependencies dependencies{
        m_peerConnectionObserver.get()
    };
    m_backend = webRtcManager->peerConnectionFactory()->CreatePeerConnection(
        config, std::move(dependencies));

    m_createOfferObserver = CreateOfferObserver::create(this, nullptr);
    m_createAnswerObserver = CreateAnswerObserver::create(this, nullptr);
    m_setLocalDescriptionObserver =
        SetLocalRemoteDescriptionObserver::create(this, nullptr);
    m_setRemoteDescriptionObserver =
        SetLocalRemoteDescriptionObserver::create(this, nullptr);

    return m_backend != nullptr;
}

RTCPeerConnection::~RTCPeerConnection()
{
    if (backend()) {
        m_backend->Close();
        deletePeerConnection();
    }
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
        STARFISH_LOG_WARN("%s: backend() == nullptr\n", __func__);
        auto exception = new DOMException(
            executionContext(), DOMException::INVALID_STATE_ERR,
            "Internal Error: backend() == nullptr");
        promise->reject(exception->scriptValue());
        return promise;
    }

    Promise* promise = new (NoGC) Promise(scriptBindingInstance());
    m_createOfferObserver->setPromise(promise);

    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions opt(
        options.m_offerToReceiveVideo, options.m_offerToReceiveAudio,
        options.m_voiceActivityDetection, options.m_iceRestart, true);

    m_backend->CreateOffer(m_createOfferObserver, opt);

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
        STARFISH_LOG_WARN("%s: backend() == nullptr\n", __func__);
        auto exception = new DOMException(
            executionContext(), DOMException::INVALID_STATE_ERR,
            "Internal Error: backend() == nullptr");
        promise->reject(exception->scriptValue());
        return promise;
    }

    Promise* promise = new (NoGC) Promise(scriptBindingInstance());
    m_createAnswerObserver->setPromise(promise);

    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions opt(
        webrtc::PeerConnectionInterface::RTCOfferAnswerOptions::
            kOfferToReceiveMediaTrue,
        webrtc::PeerConnectionInterface::RTCOfferAnswerOptions::
            kOfferToReceiveMediaTrue,
        options.m_voiceActivityDetection, false, true);

    m_backend->CreateAnswer(m_createAnswerObserver, opt);

    return promise;
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
                   obj->set(state, ValueRef::create(
                                       StringRef::createFromASCII("type")),
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
        STARFISH_LOG_WARN("%s: backend() == nullptr\n", __func__);
        auto exception = new DOMException(
            executionContext(), DOMException::INVALID_STATE_ERR,
            "Internal Error: backend() == nullptr");
        promise->reject(exception->scriptValue());
        return promise;
    }

    Promise* promise = new (NoGC) Promise(scriptBindingInstance());
    m_setLocalDescriptionObserver->setPromise(promise);

    webrtc::SdpType type = toSdpType(description.m_type);
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
        !description.m_sdp->equals("") && (m_lastCreatedAnswer != sdpString)) {
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
    std::unique_ptr<webrtc::SessionDescriptionInterface> desc =
        webrtc::CreateSessionDescription(type, sdpString);

    if (desc) {
        // SetLocalDescription takes the ownership of desc
        m_backend->SetLocalDescription(m_setLocalDescriptionObserver,
                                       desc.release());
    }

    return promise;
}

RTCSessionDescription* RTCPeerConnection::localDescription()
{
    const webrtc::SessionDescriptionInterface* desc =
        m_backend->local_description();
    if (!desc) {
        return nullptr;
    }

    return new RTCSessionDescription(executionContext(),
                                     m_backend->local_description());
}

RTCSessionDescription* RTCPeerConnection::currentLocalDescription()
{
    const webrtc::SessionDescriptionInterface* desc =
        m_backend->current_local_description();
    if (!desc) {
        return nullptr;
    }

    return new RTCSessionDescription(executionContext(),
                                     m_backend->current_local_description());
}

RTCSessionDescription* RTCPeerConnection::pendingLocalDescription()
{
    const webrtc::SessionDescriptionInterface* desc =
        m_backend->pending_local_description();
    if (!desc) {
        return nullptr;
    }

    return new RTCSessionDescription(executionContext(),
                                     m_backend->pending_local_description());
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
        STARFISH_LOG_WARN("%s: backend() == nullptr\n", __func__);
        auto exception = new DOMException(
            executionContext(), DOMException::INVALID_STATE_ERR,
            "Internal Error: backend() == nullptr");
        promise->reject(exception->scriptValue());
        return promise;
    }

    Promise* promise = new (NoGC) Promise(scriptBindingInstance());
    m_setRemoteDescriptionObserver->setPromise(promise);

    webrtc::SdpType type = toSdpType(description.m_type);
    std::string sdpString = std::string(description.m_sdp->toUTF8NonGCString());

    // https://w3c.github.io/webrtc-pc/#dom-peerconnection-setremotedescription
    // 3
    if (description.m_type == RTCSdpType::Unknown) {
        auto exception =
            new DOMException(executionContext(), DOMException::SCRIPT_TYPE_ERR,
                             "setRemoteDescription");
        promise->reject(exception->scriptValue());
        return promise;
    }

    // 4
    if ((description.m_type == RTCSdpType::Offer) &&
        !isValidRemoteState(description.m_type)) {
        auto exception = new DOMException(executionContext(),
                                          DOMException::INVALID_STATE_ERR,
                                          "setRemoteDescription");
        promise->reject(exception->scriptValue());
        return promise;
    }

    // https://w3c.github.io/webrtc-pc/#set-remote-description
    // 3.3
    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::SessionDescriptionInterface> desc =
        webrtc::CreateSessionDescription(type, sdpString, &error);

    if (error.line == "Invalid SDP") {
        RTCErrorInit init;
        init.m_errorDetail = RTCErrorDetailType::SdpSyntaxError;
        auto exception = new RTCError(executionContext(), init);
        promise->reject(exception->scriptValue());
        return promise;
    }

    if (desc) {
        // SetRemoteDescription takes the ownership of desc
        m_backend->SetRemoteDescription(m_setRemoteDescriptionObserver,
                                        desc.release());
    }
    return promise;
}

RTCSessionDescription* RTCPeerConnection::remoteDescription()
{
    const webrtc::SessionDescriptionInterface* desc =
        m_backend->remote_description();
    if (!desc) {
        return nullptr;
    }

    return new RTCSessionDescription(executionContext(),
                                     m_backend->remote_description());
}

RTCSessionDescription* RTCPeerConnection::currentRemoteDescription()
{
    const webrtc::SessionDescriptionInterface* desc =
        m_backend->current_remote_description();
    if (!desc) {
        return nullptr;
    }

    return new RTCSessionDescription(executionContext(),
                                     m_backend->current_remote_description());
}

RTCSessionDescription* RTCPeerConnection::pendingRemoteDescription()
{
    const webrtc::SessionDescriptionInterface* desc =
        m_backend->pending_remote_description();
    if (!desc) {
        return nullptr;
    }

    return new RTCSessionDescription(executionContext(),
                                     m_backend->pending_remote_description());
}

Promise* RTCPeerConnection::addIceCandidate(RTCIceCandidateInit init)
{
    std::string sdpMid;
    if (init.m_sdpMid.hasValue()) {
        sdpMid = init.m_sdpMid.getValue()->toUTF8NonGCString();
    }

    std::string sdp(init.m_candidate->toUTF8NonGCString());

    uint32_t sdpMLineIndex = 0;
    if (init.m_sdpMid.hasValue()) {
        sdpMLineIndex = init.m_sdpMLineIndex.getValue();
    }

    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::IceCandidateInterface> candidate =
        std::unique_ptr<webrtc::IceCandidateInterface>(
            webrtc::CreateIceCandidate(sdpMid, sdpMLineIndex, sdp, &error));

    bool r = backend()->AddIceCandidate(candidate.get());

    Promise* promise = new Promise(scriptBindingInstance());
    if (!r) {
        promise->reject(scriptUndefined());
        return promise;
    }

    promise->fulfill(scriptUndefined());
    return promise;
}

String* RTCPeerConnection::signalingState()
{
    switch (m_backend->signaling_state()) {
    case webrtc::PeerConnectionInterface::SignalingState::kStable:
        return String::createASCIIString("stable");
    case webrtc::PeerConnectionInterface::SignalingState::kHaveLocalOffer:
        return String::createASCIIString("have-local-offer");
    case webrtc::PeerConnectionInterface::SignalingState::kHaveRemoteOffer:
        return String::createASCIIString("have-remote-offer");
    case webrtc::PeerConnectionInterface::SignalingState::kHaveLocalPrAnswer:
        return String::createASCIIString("have-local-pranswer");
    case webrtc::PeerConnectionInterface::SignalingState::kHaveRemotePrAnswer:
        return String::createASCIIString("have-remote-pranswer");
    case webrtc::PeerConnectionInterface::SignalingState::kClosed:
        return String::createASCIIString("closed");
    default:
        return String::emptyString;
    }
}

String* RTCPeerConnection::iceGatheringState()
{
    switch (m_backend->ice_gathering_state()) {
    case webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringNew:
        return String::createASCIIString("new");
    case webrtc::PeerConnectionInterface::IceGatheringState::
        kIceGatheringGathering:
        return String::createASCIIString("gathering");
    case webrtc::PeerConnectionInterface::IceGatheringState::
        kIceGatheringComplete:
        return String::createASCIIString("complete");
    default:
        return String::emptyString;
    }
}

String* RTCPeerConnection::iceConnectionState()
{
    switch (m_backend->ice_connection_state()) {
    case webrtc::PeerConnectionInterface::IceConnectionState::
        kIceConnectionClosed:
        return String::createASCIIString("closed");
    case webrtc::PeerConnectionInterface::IceConnectionState::
        kIceConnectionFailed:
        return String::createASCIIString("failed");
    case webrtc::PeerConnectionInterface::IceConnectionState::
        kIceConnectionDisconnected:
        return String::createASCIIString("disconnected");
    case webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionNew:
        return String::createASCIIString("new");
    case webrtc::PeerConnectionInterface::IceConnectionState::
        kIceConnectionChecking:
        return String::createASCIIString("checking");
    case webrtc::PeerConnectionInterface::IceConnectionState::
        kIceConnectionCompleted:
        return String::createASCIIString("completed");
    case webrtc::PeerConnectionInterface::IceConnectionState::
        kIceConnectionConnected:
        return String::createASCIIString("connected");
    default:
        return String::emptyString;
    }
}

String* RTCPeerConnection::connectionState()
{
    switch (m_backend->peer_connection_state()) {
    case webrtc::PeerConnectionInterface::PeerConnectionState::kClosed:
        return String::createASCIIString("closed");
    case webrtc::PeerConnectionInterface::PeerConnectionState::kFailed:
        return String::createASCIIString("failed");
    case webrtc::PeerConnectionInterface::PeerConnectionState::kDisconnected:
        return String::createASCIIString("disconnected");
    case webrtc::PeerConnectionInterface::PeerConnectionState::kNew:
        return String::createASCIIString("new");
    case webrtc::PeerConnectionInterface::PeerConnectionState::kConnecting:
        return String::createASCIIString("connecting");
    case webrtc::PeerConnectionInterface::PeerConnectionState::kConnected:
        return String::createASCIIString("connected");
    default:
        return String::emptyString;
    }
}

GCVector<RTCIceServer> RTCPeerConnection::getDefaultIceServers()
{
    // FIXME: IceServer is browser specific. Update as
    // IceServers become available
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    GCVector<RTCIceServer> iceServers;
    return std::move(iceServers);
}

RTCConfiguration& RTCPeerConnection::getConfiguration()
{
    return m_configuration;
}

void RTCPeerConnection::setConfiguration(RTCConfiguration& configuration)
{
    if (isClosed()) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
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
    if (m_configuration.backend().bundle_policy !=
        configuration.backend().bundle_policy) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_MODIFICATION_ERR,
                               "InvalidModificationError");
    }
    // 6
    if (m_configuration.backend().rtcp_mux_policy !=
        configuration.backend().rtcp_mux_policy) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_MODIFICATION_ERR,
                               "InvalidModificationError");
    }
    if (configuration.backend().rtcp_mux_policy ==
        webrtc::PeerConnectionInterface::RtcpMuxPolicy::
            kRtcpMuxPolicyNegotiate) {
        // TODO: Support non-muxed RTCP
        throw new DOMException(executionContext(),
                               DOMException::NOT_SUPPORTED_ERR,
                               "NotSupportedError");
    }

    // 7 - 11: TODO

    // 12
    m_configuration = configuration;
}

void RTCPeerConnection::close()
{
    if (isClosed()) {
        return;
    }

    m_backend->Close();
}

DEFINE_EVENT_LISTENER(RTCPeerConnection, negotiationneeded);
DEFINE_EVENT_LISTENER(RTCPeerConnection, icecandidate);
DEFINE_EVENT_LISTENER(RTCPeerConnection, icecandidateerror);
DEFINE_EVENT_LISTENER(RTCPeerConnection, signalingstatechange);
DEFINE_EVENT_LISTENER(RTCPeerConnection, iceconnectionstatechange);
DEFINE_EVENT_LISTENER(RTCPeerConnection, icegatheringstatechange);
DEFINE_EVENT_LISTENER(RTCPeerConnection, connectionstatechange);
DEFINE_EVENT_LISTENER(RTCPeerConnection, datachannel);

RTCSctpTransport* RTCPeerConnection::sctp()
{
    if (!m_backend) {
        return nullptr;
    }

    rtc::scoped_refptr<webrtc::SctpTransportInterface> sctp =
        m_backend->GetSctpTransport();

    if (sctp.get() == nullptr) {
        return nullptr;
    }

    return new RTCSctpTransport(executionContext(), sctp);
}

RTCDataChannel* RTCPeerConnection::createDataChannel(
    String* label, RTCDataChannelInit dataChannelDict)
{
    if (!m_backend) {
        return nullptr;
    }

    webrtc::DataChannelInit init;
    init.ordered = dataChannelDict.m_ordered;
    init.protocol =
        std::string(dataChannelDict.m_protocol->toUTF8NonGCString().data());
    init.negotiated = dataChannelDict.m_negotiated;
    if (dataChannelDict.m_hasMaxPacketLifeTime) {
        init.maxRetransmitTime = dataChannelDict.m_maxPacketLifeTime;
    }
    if (dataChannelDict.m_hasMaxRetransmits) {
        init.maxRetransmits = dataChannelDict.m_maxRetransmits;
    }
    if (dataChannelDict.m_hasId) {
        init.id = dataChannelDict.m_id;
    }

    rtc::scoped_refptr<webrtc::DataChannelInterface> dataChannel =
        m_backend->CreateDataChannel(
            std::string(label->toUTF8NonGCString().data()), &init);

    return new RTCDataChannel(executionContext(), dataChannel);
}

GCVector<RTCRtpSender*> RTCPeerConnection::getSenders()
{
    GCVector<RTCRtpSender*> results;
    if (!m_backend) {
        return std::move(results);
    }

    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders =
        backend()->GetSenders();

    for (auto& sender : senders) {
        results.push_back(new RTCRtpSender(executionContext(), sender));
    }
    return std::move(results);
}

GCVector<RTCRtpReceiver*> RTCPeerConnection::getReceivers()
{
    GCVector<RTCRtpReceiver*> results;
    if (!m_backend) {
        return std::move(results);
    }

    std::vector<rtc::scoped_refptr<webrtc::RtpReceiverInterface>> receivers =
        m_backend->GetReceivers();

    for (auto& receiver : receivers) {
        results.push_back(new RTCRtpReceiver(executionContext(), receiver));
    }
    return std::move(results);
}

GCVector<RTCRtpTransceiver*> RTCPeerConnection::getTransceivers()
{
    GCVector<RTCRtpTransceiver*> results;
    if (!m_backend) {
        return std::move(results);
    }

    std::vector<rtc::scoped_refptr<webrtc::RtpTransceiverInterface>>
        transceivers = m_backend->GetTransceivers();

    for (auto& transceiver : transceivers) {
        results.push_back(
            new RTCRtpTransceiver(executionContext(), transceiver));
    }
    return std::move(results);
}

// https://w3c.github.io/webrtc-pc/#dom-rtcpeerconnection-addtrack
RTCRtpSender* RTCPeerConnection::addTrack(MediaStreamTrack* track,
                                          GCVector<MediaStream*>& streams)
{
    if (isClosed()) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }

    for (auto& transceiver : backend()->GetTransceivers()) {
        std::string id = "";
        if (track->isAudioStreamTrack()) {
            id = track->asAudioStreamTrack()->backend()->id();
        } else if (track->isVideoStreamTrack()) {
            id = track->asVideoStreamTrack()->backend()->id();
        }

        if (!transceiver->stopped() &&
            transceiver->sender()->track()->id() == id) {
            throw new DOMException(executionContext(),
                                   DOMException::INVALID_ACCESS_ERR,
                                   "InvalidAccessErr");
        }
    }

    std::vector<std::string> streamIds;
    for (auto stream : streams) {
        streamIds.push_back(stream->backend()->id());
    }

    webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface>> r;
    if (track->isAudioStreamTrack()) {
        r = backend()->AddTrack(track->asAudioStreamTrack()->backend(),
                                std::move(streamIds));
    } else if (track->isVideoStreamTrack()) {
        r = backend()->AddTrack(track->asVideoStreamTrack()->backend(),
                                std::move(streamIds));
    }

    if (r.ok()) {
        RTCRtpSender* rtpSender =
            new RTCRtpSender(executionContext(), r.value());
        return rtpSender;
    } else {
        STARFISH_LOG_ERROR("Failed to add audio/video track: %s\n",
                           r.error().message());
        throw new DOMException(executionContext(),
                               DOMException::INVALID_ACCESS_ERR,
                               "InvalidAccessErr");
    }
}

void RTCPeerConnection::removeTrack(RTCRtpSender* sender)
{
    if (isClosed()) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }

    if (!m_backend) {
        return;
    }
    if (!sender->backend()) {
        return;
    }

    m_backend->RemoveTrackNew(sender->backend());
}

RTCRtpTransceiver* RTCPeerConnection::addTransceiver(
    DOMStringOrMediaStreamTrack trackOrKind, RTCRtpTransceiverInit init)
{
    if (!m_backend) {
        return new RTCRtpTransceiver(executionContext());
    }

    webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpTransceiverInterface>> r;
    if (trackOrKind.isDOMStringValue()) {
        String* kind = trackOrKind.getDOMStringValue();
        if (kind->equals("audio")) {
            r = m_backend->AddTransceiver(cricket::MediaType::MEDIA_TYPE_AUDIO);
        } else if (kind->equals("audio")) {
            r = m_backend->AddTransceiver(cricket::MediaType::MEDIA_TYPE_VIDEO);
        }
    } else if (trackOrKind.isMediaStreamTrackValue()) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    if (!r.ok()) {
        return new RTCRtpTransceiver(executionContext());
    }

    return new RTCRtpTransceiver(executionContext(), r.value());
}

rtc::scoped_refptr<webrtc::PeerConnectionInterface> RTCPeerConnection::backend()
{
    return m_backend;
}

bool RTCPeerConnection::isClosed()
{
    if (m_backend->peer_connection_state() ==
        webrtc::PeerConnectionInterface::PeerConnectionState::kClosed) {
        return true;
    }
    return false;
}

void RTCPeerConnection::deletePeerConnection()
{
    m_backend = nullptr;
}

RTCSdpType RTCPeerConnection::toRtcSdpType(webrtc::SdpType type)
{
    RTCSessionDescriptionInit init(type, String::emptyString);
    return init.m_type;
}

webrtc::SdpType RTCPeerConnection::toSdpType(RTCSdpType type)
{
    RTCSessionDescriptionInit init(type, String::emptyString);
    return init.toSdpType();
}

DOMException* RTCPeerConnection::toDomException(webrtc::RTCError error)
{
    DOMException* exception = nullptr;
    switch (error.type()) {
    case webrtc::RTCErrorType::UNSUPPORTED_OPERATION:
    case webrtc::RTCErrorType::UNSUPPORTED_PARAMETER:
    case webrtc::RTCErrorType::RESOURCE_EXHAUSTED:
    case webrtc::RTCErrorType::INTERNAL_ERROR:
        exception = new DOMException(
            m_executionContext, DOMException::DOM_EXCEPTION, "OperationError");
        break;
    case webrtc::RTCErrorType::INVALID_PARAMETER:
        exception = new DOMException(m_executionContext,
                                     DOMException::INVALID_ACCESS_ERR,
                                     "InvalidAccessErr");
        break;
    case webrtc::RTCErrorType::INVALID_RANGE:
        exception = new DOMException(
            m_executionContext, DOMException::SCRIPT_RANGE_ERR, "RangeError");
        break;
    case webrtc::RTCErrorType::SYNTAX_ERROR:
        exception = new DOMException(m_executionContext,
                                     DOMException::SYNTAX_ERR, "SyntaxError");
        break;
    case webrtc::RTCErrorType::INVALID_STATE:
        exception = new DOMException(m_executionContext,
                                     DOMException::INVALID_STATE_ERR,
                                     "InvalidStateError");
        break;
    case webrtc::RTCErrorType::INVALID_MODIFICATION:
        exception = new DOMException(m_executionContext,
                                     DOMException::INVALID_MODIFICATION_ERR,
                                     "InvalidModificationError");
        break;
    case webrtc::RTCErrorType::NETWORK_ERROR:
        exception = new DOMException(m_executionContext,
                                     DOMException::NETWORK_ERR, "NetworkError");
        break;
    default:
        break;
    }

    return exception;
}

bool RTCPeerConnection::isValidRemoteState(RTCSdpType type)
{
    webrtc::PeerConnectionInterface::SignalingState state =
        m_backend->signaling_state();
    if (type == RTCSdpType::Offer) {
        if ((state ==
             webrtc::PeerConnectionInterface::SignalingState::kStable) ||
            (state == webrtc::PeerConnectionInterface::SignalingState::
                          kHaveRemoteOffer)) {
            return true;
        }
    } else if ((type == RTCSdpType::Pranswer) || (type == RTCSdpType::Answer)) {
        if ((state == webrtc::PeerConnectionInterface::SignalingState::
                          kHaveLocalOffer) ||
            (state == webrtc::PeerConnectionInterface::SignalingState::
                          kHaveRemotePrAnswer)) {
            return true;
        }
    }

    return false;
}
}

#endif
