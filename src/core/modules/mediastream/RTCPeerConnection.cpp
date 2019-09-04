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
#include "core/modules/mediastream/OperationQueue.h"
#include "core/modules/mediastream/MediaStream.h"
#include "core/modules/mediastream/RTCRtpSender.h"
#include "core/modules/mediastream/RTCTrackEvent.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/WebBase.h"
#include "core/page/GlobalScope.h"

#include "api/rtp_transceiver_interface.h"
#include "rtc_base/physical_socket_server.h"
#include "rtc_base/strings/json.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"

namespace Starfish {

#if defined(STARFISH_ENABLE_TEST)
class DummySetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
public:
    static DummySetSessionDescriptionObserver* Create()
    {
        return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
    }
    virtual void OnSuccess()
    {
        STARFISH_LOG_INFO("%s\n", __func__);
    }
    virtual void OnFailure(webrtc::RTCError error)
    {
        STARFISH_LOG_INFO("%s: %s: %s", __func__, ToString(error.type()).data(),
                          error.message());
    }
};

rtc::Thread* TestPeerConnectionObserver::m_socketThread = nullptr;

TestPeerConnectionObserver::TestPeerConnectionObserver()
    : TestPeerConnectionObserver(nullptr)
{
}

TestPeerConnectionObserver::TestPeerConnectionObserver(
    RTCPeerConnection* peerConnection)
    : PeerConnectionObserver(peerConnection)
{
    m_client->registerObserver(this);
}

void* TestPeerConnectionObserver::runSocketServer(void* arg)
{
    rtc::PhysicalSocketServer socketServer;
    rtc::AutoSocketServerThread thread(&socketServer);
    rtc::Thread** t = (rtc::Thread**)arg;
    *t = rtc::Thread::Current();
    STARFISH_LOG_INFO("%s\n", __func__);
    thread.Run();
    return nullptr;
}

// PeerConnectionClientObserver implementation.
void TestPeerConnectionObserver::OnSignedIn()
{
    STARFISH_LOG_INFO("%s\n", __func__);
}

void TestPeerConnectionObserver::OnDisconnected()
{
    STARFISH_LOG_INFO("%s\n", __func__);
    deletePeerConnection();
}

void TestPeerConnectionObserver::OnPeerConnected(int id,
                                                 const std::string& name)
{
    STARFISH_LOG_INFO("%s: peerId: %d\n", __func__, id);
}

void TestPeerConnectionObserver::OnPeerDisconnected(int id)
{
    STARFISH_LOG_INFO("%s: peerId: %d\n", __func__, id);
    if (id == m_peerId) {
        STARFISH_LOG_INFO("Our peer disconnected\n");
        deletePeerConnection();
    }
}

void TestPeerConnectionObserver::OnMessageFromPeer(int peerId,
                                                   const std::string& message)
{
    STARFISH_LOG_INFO("%s: peerId: %d\n", __func__, peerId);
    STARFISH_ASSERT(!message.empty());

    if (!m_peerConnection->backend().get()) {
        STARFISH_ASSERT(m_peerId == -1);
        m_peerId = peerId;

        if (!m_peerConnection->initializePeerConnection()) {
            STARFISH_LOG_ERROR(
                "Failed to initialize our PeerConnection instance\n");
            m_client->signOut();
            return;
        }
    }
    m_peerId = peerId;

    Json::Reader reader;
    Json::Value jmessage;
    if (!reader.parse(message, jmessage)) {
        STARFISH_LOG_WARN("Received unknown message. %s\n", message.data());
        return;
    }
    std::string typeStr;
    std::string jsonObject;

    rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionTypeName,
                                 &typeStr);
    if (!typeStr.empty()) {
        if (typeStr == "offer-loopback") {
            // This is a loopback call.
            // Recreate the peerconnection with DTLS disabled.
            if (!reinitializePeerConnectionForLoopback()) {
                STARFISH_LOG_ERROR(
                    "Failed to initialize our PeerConnection instance\n");
                deletePeerConnection();
                m_client->signOut();
            }
            return;
        }
        absl::optional<webrtc::SdpType> typeMaybe =
            webrtc::SdpTypeFromString(typeStr);
        if (!typeMaybe) {
            STARFISH_LOG_ERROR("Unknown SDP type: %s\n", typeStr.data());
            return;
        }
        webrtc::SdpType type = *typeMaybe;
        std::string sdp;
        if (!rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionSdpName,
                                          &sdp)) {
            STARFISH_LOG_WARN(
                "Can't parse received session description message.\n");
            return;
        }
        webrtc::SdpParseError error;
        std::unique_ptr<webrtc::SessionDescriptionInterface>
            sessionDescription =
                webrtc::CreateSessionDescription(type, sdp, &error);
        if (!sessionDescription) {
            STARFISH_LOG_WARN(
                "Can't parse received session description message. "
                "SdpParseError was: %s\n",
                error.description.data());
            return;
        }
        STARFISH_LOG_INFO("Received session description : %s\n",
                          message.data());
        m_peerConnection->backend()->SetRemoteDescription(
            DummySetSessionDescriptionObserver::Create(),
            sessionDescription.release());
        if (type == webrtc::SdpType::kOffer) {
            m_peerConnection->backend()->CreateAnswer(
                this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
        }
    } else {
        std::string sdpMid;
        int sdpMlineindex = 0;
        std::string sdp;
        if (!rtc::GetStringFromJsonObject(jmessage, kCandidateSdpMidName,
                                          &sdpMid) ||
            !rtc::GetIntFromJsonObject(jmessage, kCandidateSdpMlineIndexName,
                                       &sdpMlineindex) ||
            !rtc::GetStringFromJsonObject(jmessage, kCandidateSdpName, &sdp)) {
            STARFISH_LOG_WARN("Can't parse received message.\n");
            return;
        }
        webrtc::SdpParseError error;
        std::unique_ptr<webrtc::IceCandidateInterface> candidate(
            webrtc::CreateIceCandidate(sdpMid, sdpMlineindex, sdp, &error));
        if (!candidate.get()) {
            STARFISH_LOG_WARN(
                "Can't parse received candidate message. "
                "SdpParseError was: %s",
                error.description.data());
            return;
        }
        if (!m_peerConnection->backend()->AddIceCandidate(candidate.get())) {
            STARFISH_LOG_WARN("Failed to apply the received candidate\n");
            return;
        }
        STARFISH_LOG_INFO("Received candidate : %s\n", message.data());
    }
}

void TestPeerConnectionObserver::OnMessageSent(int err)
{
    STARFISH_LOG_INFO("%s: peerId: %d\n", __func__, m_peerId);
    m_client->sendToPeer(m_peerId, "");
}

void TestPeerConnectionObserver::OnServerConnectionFailure()
{
    STARFISH_LOG_ERROR("Failed to connect to %s\n", m_server.data());
}

void TestPeerConnectionObserver::startLogin(const std::string& server, int port)
{
    if (m_client->isConnected()) {
        return;
    }
    STARFISH_LOG_INFO("%s\n", __func__);
    m_server = server;
    m_client->connect(server, port, m_client->peerName());
}

void TestPeerConnectionObserver::deletePeerConnection()
{
    m_peerId = -1;
    m_loopback = false;
}

bool TestPeerConnectionObserver::reinitializePeerConnectionForLoopback()
{
    m_loopback = true;
    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders =
        m_peerConnection->backend()->GetSenders();

    bool r = m_peerConnection->initializePeerConnection();
    if (r) {
        for (const auto& sender : senders) {
            m_peerConnection->backend()->AddTrack(sender->track(),
                                                  sender->stream_ids());
        }
        m_peerConnection->backend()->CreateOffer(
            this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
    }
    return r;
}

#endif

PeerConnectionObserver::PeerConnectionObserver()
    : PeerConnectionObserver(nullptr)
{
}

PeerConnectionObserver::PeerConnectionObserver(
    RTCPeerConnection* peerConnection)
    : m_peerConnection(peerConnection)
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
        rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track;
        std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> streams;
    };

    Params* p = new Params();
    p->self = this;
    p->track = track;
    p->streams = streams;

    m_peerConnection->executionContext()
        ->webBase()
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            m_peerConnection->executionContext()->globalScope(),
            [](size_t, void* data) {
                Params* p = (Params*)data;
                PeerConnectionObserver* self = p->self;
                rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrack(
                    (webrtc::VideoTrackInterface*)p->track.get());

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

CreateSessionDescriptionObserver* CreateSessionDescriptionObserver::create(
    RTCPeerConnection* peerConnection, Promise* promise)
{
    CreateSessionDescriptionObserver* observer =
        new rtc::RefCountedObject<CreateSessionDescriptionObserver>();
    observer->m_peerConnection = peerConnection;
    observer->m_promise = promise;
    return observer;
}

void CreateSessionDescriptionObserver::OnSuccess(
    webrtc::SessionDescriptionInterface* desc)
{
    STARFISH_ASSERT(m_promise);
    STARFISH_LOG_INFO("CreateSessionDescriptionObserver::%s\n", __func__);

    // This callback is called from another thread
    // The following lines must be executed in the main thread.
    struct Params {
        CreateSessionDescriptionObserver* self;
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
                CreateSessionDescriptionObserver* self = p->self;
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
                promise->fulfill(createScriptValue(sd));
                self->setPromise(nullptr);
                delete promise;
                delete p;
            },
            p);
}

void CreateSessionDescriptionObserver::OnFailure(webrtc::RTCError error)
{
    STARFISH_ASSERT(m_promise);
    STARFISH_LOG_WARN("CreateSessionDescriptionObserver::%s\n", __func__);

    // This callback is called from another thread
    // The following lines must be executed in the main thread.
    struct Params {
        CreateSessionDescriptionObserver* self;
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
                CreateSessionDescriptionObserver* self = p->self;
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

SetSessionDescriptionObserver* SetSessionDescriptionObserver::create(
    RTCPeerConnection* peerConnection, Promise* promise)
{
    SetSessionDescriptionObserver* observer =
        new rtc::RefCountedObject<SetSessionDescriptionObserver>();
    observer->m_peerConnection = peerConnection;
    observer->m_promise = promise;
    return observer;
}

void SetSessionDescriptionObserver::OnSuccess()
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

void SetSessionDescriptionObserver::OnFailure(webrtc::RTCError error)
{
    STARFISH_ASSERT(m_promise);
    STARFISH_LOG_WARN("SetSessionDescriptionObserver::%s\n", __func__);

    // This callback is called from another thread
    // The following lines must be executed in the main thread.
    struct Params {
        SetSessionDescriptionObserver* self;
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
                SetSessionDescriptionObserver* self = p->self;
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
    , m_operationQueue(new OperationQueue(executionContext))
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
#if defined(STARFISH_ENABLE_TEST)
    // TODO: Remove TestPeerConnectionObserver after finishing JS interface
    // for network connection
    m_peerConnectionObserver = rtc::scoped_refptr<TestPeerConnectionObserver>(
        new rtc::RefCountedObject<TestPeerConnectionObserver>(this));
#else
    m_peerConnectionObserver = rtc::scoped_refptr<PeerConnectionObserver>(
        new rtc::RefCountedObject<PeerConnectionObserver>(this));
#endif

    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
        peerConnectionFactory = this->executionContext()
                                    ->document()
                                    ->window()
                                    ->navigator()
                                    ->webRtcManager()
                                    ->peerConnectionFactory();
    STARFISH_ASSERT(peerConnectionFactory);

    bool dtls = true;
    webrtc::PeerConnectionInterface::RTCConfiguration config =
        configuration.backend();
    config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
    config.enable_rtp_data_channel = true;
    config.enable_dtls_srtp = dtls;

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

    m_backend = peerConnectionFactory->CreatePeerConnection(
        config, nullptr, nullptr, m_peerConnectionObserver);
    m_createOfferSessionObserver =
        CreateSessionDescriptionObserver::create(this, nullptr);
    m_createAnswerSessionObserver =
        CreateSessionDescriptionObserver::create(this, nullptr);

    m_setLocalSessionObserver =
        SetSessionDescriptionObserver::create(this, nullptr);
    m_setRemoteSessionObserver =
        SetSessionDescriptionObserver::create(this, nullptr);

    return m_backend != nullptr;
}

RTCPeerConnection::~RTCPeerConnection()
{
    deletePeerConnection();
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

    Promise* promise = new (NoGC) Promise(scriptBindingInstance());
    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions* opt =
        new webrtc::PeerConnectionInterface::RTCOfferAnswerOptions(
            options.m_offerToReceiveVideo, options.m_offerToReceiveAudio,
            options.m_voiceActivityDetection, options.m_iceRestart, true);
    m_operationQueue->enqueue(
        [](Promise* promise, void* data1, void* data2) {
            RTCPeerConnection* self = (RTCPeerConnection*)data1;
            webrtc::PeerConnectionInterface::RTCOfferAnswerOptions* opt =
                (webrtc::PeerConnectionInterface::RTCOfferAnswerOptions*)data2;

            if (self->backend()) {
                // the observer creates an exception if needed
                self->m_createOfferSessionObserver->setPromise(promise);
                self->backend()->CreateOffer(self->m_createOfferSessionObserver,
                                             *opt);
                delete opt;
            } else {
                STARFISH_LOG_WARN("%s: connection failed\n", __func__);
                auto exception = new DOMException(
                    self->executionContext(), DOMException::INVALID_STATE_ERR,
                    "InvalidStateError");
                promise->reject(exception->scriptValue());
            }
        },
        promise, this, opt);

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

    Promise* promise = new (NoGC) Promise(scriptBindingInstance());
    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions* opt =
        new webrtc::PeerConnectionInterface::RTCOfferAnswerOptions(
            webrtc::PeerConnectionInterface::RTCOfferAnswerOptions::
                kOfferToReceiveMediaTrue,
            webrtc::PeerConnectionInterface::RTCOfferAnswerOptions::
                kOfferToReceiveMediaTrue,
            options.m_voiceActivityDetection, false, true);
    m_operationQueue->enqueue(
        [](Promise* promise, void* data1, void* data2) {
            RTCPeerConnection* self = (RTCPeerConnection*)data1;
            webrtc::PeerConnectionInterface::RTCOfferAnswerOptions* opt =
                (webrtc::PeerConnectionInterface::RTCOfferAnswerOptions*)data2;

            if (self->backend()) {
                // the observer creates an exception if needed
                self->m_createAnswerSessionObserver->setPromise(promise);
                self->backend()->CreateAnswer(
                    self->m_createAnswerSessionObserver, *opt);
            } else {
                STARFISH_LOG_WARN("%s: connection failed\n", __func__);
                auto exception = new DOMException(
                    self->executionContext(), DOMException::INVALID_STATE_ERR,
                    "InvalidStateError");
                promise->reject(exception->scriptValue());
            }
            delete opt;
        },
        promise, this, opt);

    return promise;
}

ScriptObject RTCPeerConnection::createSessionDescriptionInitObject(
    RTCSdpType type, String* sdp)
{
    RTCSessionDescriptionInit sd(type, sdp);
    ContextRef* ctx = scriptBindingInstance()->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);

    ScriptObject obj = ObjectRef::create(state);
    String* sdValue = sd.type();
    obj->set(state, ValueRef::create(StringRef::fromASCII("type")),
             ValueRef::create(toJSString(sdValue)));
    String* sdpValue = sd.sdp();
    obj->set(state, ValueRef::create(StringRef::fromASCII("sdp")),
             ValueRef::create(toJSString(sdpValue)));

    return obj;
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

    Promise* promise = new (NoGC) Promise(scriptBindingInstance());
    struct Params : public gc {
        RTCPeerConnection* self;
        RTCSdpType type;
        String* sdp;
    };
    Params* p = new (NoGC) Params();
    p->self = this;
    p->type = description.m_type;
    p->sdp = description.m_sdp;

    m_operationQueue->enqueue(
        [](Promise* promise, void* data1) {
            Params* p = (Params*)data1;
            RTCPeerConnection* self = (RTCPeerConnection*)(p->self);

            if (self->isClosed()) {
                promise->fulfill(scriptUndefined());
                delete p;
                return;
            }

            if (self->backend()) {
                // the observer creates an exception if needed
                self->m_setLocalSessionObserver->setPromise(promise);

                webrtc::SdpType type = self->toSdpType(p->type);
                std::string sdpString =
                    std::string(p->sdp->toUTF8NonGCString());
                std::unique_ptr<webrtc::SessionDescriptionInterface> desc =
                    webrtc::CreateSessionDescription(type, sdpString);

                // SetLocalDescription takes the ownership of desc
                self->backend()->SetLocalDescription(
                    self->m_setLocalSessionObserver, desc.release());
            } else {
                STARFISH_LOG_WARN("%s: connection failed\n", __func__);
                auto exception = new DOMException(
                    self->executionContext(), DOMException::INVALID_STATE_ERR,
                    "InvalidStateError");
                promise->reject(exception->scriptValue());
            }
            delete p;
            return;
        },
        promise, p);

    return promise;
}

RTCSessionDescription* RTCPeerConnection::localDescription()
{
    return new RTCSessionDescription(executionContext(),
                                     m_backend->local_description());
}

RTCSessionDescription* RTCPeerConnection::currentLocalDescription()
{
    return new RTCSessionDescription(executionContext(),
                                     m_backend->current_local_description());
}

RTCSessionDescription* RTCPeerConnection::pendingLocalDescription()
{
    return new RTCSessionDescription(executionContext(),
                                     m_backend->pending_local_description());
}

Promise* RTCPeerConnection::setRemoteDescription(
    RTCSessionDescriptionInit& description)
{
    if (isClosed()) {
        Promise* promise = new Promise(scriptBindingInstance());
        promise->fulfill(scriptUndefined());
        return promise;
    }

    Promise* promise = new (NoGC) Promise(scriptBindingInstance());
    struct Params : public gc {
        RTCPeerConnection* self;
        RTCSdpType type;
        String* sdp;
    };
    Params* p = new (NoGC) Params();
    p->self = this;
    p->type = description.m_type;
    p->sdp = description.m_sdp;

    m_operationQueue->enqueue(
        [](Promise* promise, void* data1) {
            Params* p = (Params*)data1;
            RTCPeerConnection* self = (RTCPeerConnection*)(p->self);

            if (self->isClosed()) {
                promise->fulfill(scriptUndefined());
                delete p;
                return;
            }

            if (self->backend()) {
                // the observer creates an exception if needed
                self->m_setRemoteSessionObserver->setPromise(promise);

                webrtc::SdpType type = self->toSdpType(p->type);
                std::string sdpString =
                    std::string(p->sdp->toUTF8NonGCString());

                std::unique_ptr<webrtc::SessionDescriptionInterface> desc =
                    webrtc::CreateSessionDescription(type, sdpString);

                // SetRemoteDescription takes the ownership of desc
                self->backend()->SetRemoteDescription(
                    self->m_setRemoteSessionObserver, desc.release());
            } else {
                STARFISH_LOG_WARN("%s: connection failed\n", __func__);
                auto exception = new DOMException(
                    self->executionContext(), DOMException::INVALID_STATE_ERR,
                    "InvalidStateError");
                promise->reject(exception->scriptValue());
            }
            delete p;
            return;
        },
        promise, p);

    return promise;
}

RTCSessionDescription* RTCPeerConnection::remoteDescription()
{
    return new RTCSessionDescription(executionContext(),
                                     m_backend->remote_description());
}

RTCSessionDescription* RTCPeerConnection::currentRemoteDescription()
{
    return new RTCSessionDescription(executionContext(),
                                     m_backend->current_remote_description());
}

RTCSessionDescription* RTCPeerConnection::pendingRemoteDescription()
{
    return new RTCSessionDescription(executionContext(),
                                     m_backend->pending_remote_description());
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

GCVector<RTCRtpSender*> RTCPeerConnection::getSenders()
{
    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders =
        backend()->GetSenders();
    GCVector<RTCRtpSender*> results;
    for (auto& sender : senders) {
        results.push_back(new RTCRtpSender(executionContext(), sender));
    }
    return results;
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
    switch (type) {
    case webrtc::SdpType::kOffer:
        return RTCSdpType::Offer;
    case webrtc::SdpType::kPrAnswer:
        return RTCSdpType::Pranswer;
    case webrtc::SdpType::kAnswer:
        return RTCSdpType::Answer;
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
}

webrtc::SdpType RTCPeerConnection::toSdpType(RTCSdpType type)
{
    switch (type) {
    case RTCSdpType::Offer:
        return webrtc::SdpType::kOffer;
    case RTCSdpType::Pranswer:
        return webrtc::SdpType::kPrAnswer;
    case RTCSdpType::Answer:
        return webrtc::SdpType::kAnswer;
    // case RTCSdpType::Rollback:
    //     STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
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
}

#endif
