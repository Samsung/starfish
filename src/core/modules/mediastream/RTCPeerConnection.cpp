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

TestPeerConnectionObserver::TestPeerConnectionObserver(
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> pcFactory)
    : PeerConnectionObserver(pcFactory)
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

    if (!m_peerConnection.get()) {
        STARFISH_ASSERT(m_peerId == -1);
        m_peerId = peerId;

        if (!initializePeerConnection()) {
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
        m_peerConnection->SetRemoteDescription(
            DummySetSessionDescriptionObserver::Create(),
            sessionDescription.release());
        if (type == webrtc::SdpType::kOffer) {
            m_peerConnection->CreateAnswer(
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
        if (!m_peerConnection->AddIceCandidate(candidate.get())) {
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
    PeerConnectionObserver::deletePeerConnection();
    m_peerId = -1;
}

#endif

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

    // 9--11
    m_configuration = configuration;

#if defined(STARFISH_ENABLE_TEST)
    m_peerConnectionObserver = rtc::scoped_refptr<TestPeerConnectionObserver>(
        new rtc::RefCountedObject<TestPeerConnectionObserver>(
            this->executionContext()
                ->document()
                ->window()
                ->navigator()
                ->webRtcManager()
                ->peerConnectionFactory()));
#else
    m_peerConnectionObserver = rtc::scoped_refptr<PeerConnectionObserver>(
        new rtc::RefCountedObject<PeerConnectionObserver>(
            this->executionContext()
                ->document()
                ->window()
                ->navigator()
                ->webRtcManager()
                ->peerConnectionFactory()));
#endif

    STARFISH_ASSERT(m_peerConnectionObserver->peerConnection());
    this->executionContext()
        ->document()
        ->window()
        ->navigator()
        ->webRtcManager()
        ->setPeerConnection(m_peerConnectionObserver->peerConnection());

    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj,
                 void* cd) { ((RTCPeerConnection*)obj)->~RTCPeerConnection(); },
        NULL, NULL, NULL);
}

RTCPeerConnection::~RTCPeerConnection()
{
    m_peerConnectionObserver->deletePeerConnection();
}

ScriptBindingInstance* RTCPeerConnection::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

ExecutionContext* RTCPeerConnection::executionContext() const
{
    return m_executionContext;
}

PeerConnectionObserver::PeerConnectionObserver(
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> pcFactory)
    : m_peerConnectionFactory(pcFactory)
{
    initializePeerConnection();
}

bool PeerConnectionObserver::initializePeerConnection()
{
    STARFISH_LOG_INFO("%s\n", __func__);
    STARFISH_ASSERT(m_peerConnectionFactory);
    STARFISH_ASSERT(!m_peerConnection);

    if (!createPeerConnection(/*dtls=*/true)) {
        STARFISH_LOG_ERROR("CreatePeerConnection failed\n");
        deletePeerConnection();
    }

    return m_peerConnection != nullptr;
}

bool PeerConnectionObserver::reinitializePeerConnectionForLoopback()
{
    m_loopback = true;
    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders =
        m_peerConnection->GetSenders();
    m_peerConnection = nullptr;
    if (createPeerConnection(/*dtls=*/false)) {
        for (const auto& sender : senders) {
            m_peerConnection->AddTrack(sender->track(), sender->stream_ids());
        }
        m_peerConnection->CreateOffer(
            this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
    }
    return m_peerConnection != nullptr;
}

void PeerConnectionObserver::deletePeerConnection()
{
    m_peerConnection = nullptr;
    m_loopback = false;
}

rtc::scoped_refptr<webrtc::PeerConnectionInterface>
PeerConnectionObserver::peerConnection()
{
    return m_peerConnection;
}

void PeerConnectionObserver::connectToPeer()
{
    if (m_peerConnection.get()) {
        STARFISH_LOG_ERROR("We only support connecting to one peer at a time");
        return;
    }

    if (initializePeerConnection()) {
        m_peerConnection->CreateOffer(
            this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
    } else {
        STARFISH_LOG_ERROR("Failed to initialize PeerConnection");
    }
}

bool PeerConnectionObserver::createPeerConnection(bool dtls)
{
    STARFISH_ASSERT(m_peerConnectionFactory);
    STARFISH_ASSERT(!m_peerConnection);

    webrtc::PeerConnectionInterface::RTCConfiguration config;
    config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
    config.enable_dtls_srtp = dtls;
    webrtc::PeerConnectionInterface::IceServer server;

    server.uri = m_stun;
    config.servers.push_back(server);

    m_peerConnection = m_peerConnectionFactory->CreatePeerConnection(
        config, nullptr, nullptr, this);
    return m_peerConnection != nullptr;
}

// https://w3c.github.io/webrtc-pc/#dom-rtcpeerconnection-createoffer
Promise* RTCPeerConnection::createOffer(RTCOfferOptions options)
{
    Promise* promise = new Promise(scriptBindingInstance());
    // 1-2
    if (m_isClosed) {
        auto exception = new DOMException(executionContext(),
                                          DOMException::INVALID_STATE_ERR,
                                          "InvalidStateError");
        promise->reject(exception->scriptValue());
        return promise;
    }

    m_operationQueue->enqueue(
        [](Promise* promise, void* data) {
            RTCPeerConnection* connection = castTo<RTCPeerConnection*>(data);

            if (connection->m_isClosed) {
                ObjectRef* sd = connection->createSessionDescriptionInitObject(
                    RTCSdpType::Offer, String::createASCIIString(""));
                promise->reject(createScriptValue(sd));
                return;
            }

            if ((connection->m_signalingState ==
                 RTCSignalingState::HaveRemoteOffer) ||
                (connection->m_signalingState ==
                 RTCSignalingState::HaveLocalPranswer) ||
                (connection->m_signalingState ==
                 RTCSignalingState::HaveRemotePranswer) ||
                (connection->m_signalingState == RTCSignalingState::Closed)) {
                auto exception = new DOMException(
                    connection->executionContext(),
                    DOMException::INVALID_STATE_ERR, "InvalidStateError");
                promise->reject(exception->scriptValue());
                return;
            }

            connection->m_peerConnectionObserver->connectToPeer();

            // TODO: identity provider
            ObjectRef* sd = connection->createSessionDescriptionInitObject(
                RTCSdpType::Offer, String::createASCIIString("sdpString"));
            promise->fulfill(createScriptValue(sd));
            return;
        },
        promise, this);

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
    RTCSessionDescriptionInit d = description;
    if ((d.m_sdp->equals(String::emptyString)) &&
        ((d.m_type == RTCSdpType::Answer) ||
         (d.m_type == RTCSdpType::Pranswer))) {
        d.m_sdp = m_lastCreatedAnswer;
    }
    if ((d.m_sdp->equals(String::emptyString)) &&
        (d.m_type == RTCSdpType::Offer)) {
        d.m_sdp = m_lastCreatedOffer;
    }

    Promise* promise = new Promise(scriptBindingInstance());

    struct Params : public gc {
        RTCPeerConnection* self;
        RTCSessionDescriptionInit d;
    };
    Params* p = new Params();
    p->self = this;
    p->d = d;

    m_operationQueue->enqueue(
        [](Promise* promise, void* data) {
            Params* p = castTo<Params*>(data);
            RTCPeerConnection* con = castTo<RTCPeerConnection*>(p->self);
            RTCSessionDescriptionInit d = p->d;

            if ((d.m_type == RTCSdpType::Offer) &&
                !(d.m_sdp->equals(con->m_lastCreatedOffer))) {
                auto exception =
                    new DOMException(con->executionContext(),
                                     DOMException::INVALID_MODIFICATION_ERR,
                                     "InvalidModificationError");
                promise->reject(exception->scriptValue());
                return;
            }
            if (((d.m_type == RTCSdpType::Answer) ||
                 (d.m_type == RTCSdpType::Pranswer)) &&
                !(d.m_sdp->equals(con->m_lastCreatedAnswer))) {
                auto exception =
                    new DOMException(con->executionContext(),
                                     DOMException::INVALID_MODIFICATION_ERR,
                                     "InvalidModificationError");
                promise->reject(exception->scriptValue());
                return;
            }

            // 4.2
            if (con->m_isClosed) {
                promise->fulfill(scriptUndefined());
                return;
            }
            // 4.2.1
            if (d.m_type == RTCSdpType::Offer) {
                con->m_pendingLocalDescription =
                    new RTCSessionDescription(con->executionContext(), d);
                con->m_signalingState = RTCSignalingState::HaveLocalOffer;
            } else if (d.m_type == RTCSdpType::Answer) {
                con->m_currentLocalDescription =
                    new RTCSessionDescription(con->executionContext(), d);
                con->m_currentRemoteDescription =
                    con->m_pendingRemoteDescription;
                con->m_pendingRemoteDescription = nullptr;
                con->m_pendingLocalDescription = nullptr;
                con->m_lastCreatedOffer = String::emptyString;
                con->m_lastCreatedAnswer = String::emptyString;
                con->m_signalingState = RTCSignalingState::Stable;
            } else if (d.m_type == RTCSdpType::Rollback) {
                con->m_pendingLocalDescription = nullptr;
                con->m_signalingState = RTCSignalingState::Stable;
            } else if (d.m_type == RTCSdpType::Pranswer) {
                con->m_pendingLocalDescription =
                    new RTCSessionDescription(con->executionContext(), d);
                con->m_signalingState = RTCSignalingState::HaveLocalPranswer;
            }

            // TODO: remote descripition and so on
            promise->fulfill(scriptUndefined());
            return;
        },
        promise, p);

    return promise;
}

RTCSessionDescription* RTCPeerConnection::localDescription()
{
    if (m_pendingLocalDescription != nullptr) {
        return m_pendingLocalDescription;
    }
    return m_currentLocalDescription;
}

RTCSessionDescription* RTCPeerConnection::remoteDescription()
{
    if (m_pendingRemoteDescription != nullptr) {
        return m_pendingRemoteDescription;
    }
    return m_currentRemoteDescription;
}

String* RTCPeerConnection::signalingState()
{
    switch (m_signalingState) {
    case RTCSignalingState::Stable:
        return String::createASCIIString("stable");
    case RTCSignalingState::HaveLocalOffer:
        return String::createASCIIString("have-local-offer");
    case RTCSignalingState::HaveRemoteOffer:
        return String::createASCIIString("have-remote-offer");
    case RTCSignalingState::HaveLocalPranswer:
        return String::createASCIIString("have-local-pranswer");
    case RTCSignalingState::HaveRemotePranswer:
        return String::createASCIIString("have-remote-pranswer");
    case RTCSignalingState::Closed:
        return String::createASCIIString("closed");
    default:
        return String::emptyString;
    }
}

String* RTCPeerConnection::iceGatheringState()
{
    switch (m_iceGatheringState) {
    case RTCIceGatheringState::New:
        return String::createASCIIString("new");
    case RTCIceGatheringState::Gathering:
        return String::createASCIIString("gathering");
    case RTCIceGatheringState::Complete:
        return String::createASCIIString("complete");
    default:
        return String::emptyString;
    }
}

String* RTCPeerConnection::iceConnectionState()
{
    switch (m_iceConnectionState) {
    case RTCIceConnectionState::Closed:
        return String::createASCIIString("closed");
    case RTCIceConnectionState::Failed:
        return String::createASCIIString("failed");
    case RTCIceConnectionState::Disconnected:
        return String::createASCIIString("disconnected");
    case RTCIceConnectionState::New:
        return String::createASCIIString("new");
    case RTCIceConnectionState::Checking:
        return String::createASCIIString("checking");
    case RTCIceConnectionState::Completed:
        return String::createASCIIString("completed");
    case RTCIceConnectionState::Connected:
        return String::createASCIIString("connected");
    default:
        return String::emptyString;
    }
}

String* RTCPeerConnection::connectionState()
{
    switch (m_connectionState) {
    case RTCPeerConnectionState::Closed:
        return String::createASCIIString("closed");
    case RTCPeerConnectionState::Failed:
        return String::createASCIIString("failed");
    case RTCPeerConnectionState::Disconnected:
        return String::createASCIIString("disconnected");
    case RTCPeerConnectionState::New:
        return String::createASCIIString("new");
    case RTCPeerConnectionState::Connecting:
        return String::createASCIIString("connecting");
    case RTCPeerConnectionState::Connected:
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
    if (m_isClosed) {
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
    if (m_configuration.m_bundlePolicy != configuration.m_bundlePolicy) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_MODIFICATION_ERR,
                               "InvalidModificationError");
    }
    // 6
    if (m_configuration.m_rtcpMuxPolicy != configuration.m_rtcpMuxPolicy) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_MODIFICATION_ERR,
                               "InvalidModificationError");
    }
    if (configuration.m_rtcpMuxPolicy == RTCRtcpMuxPolicy::Negotiate) {
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
    if (m_isClosed) {
        return;
    }

    m_isClosed = true;
    m_signalingState = RTCSignalingState::Closed;
    m_iceConnectionState = RTCIceConnectionState::Closed;
    m_connectionState = RTCPeerConnectionState::Closed;
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
    return m_peerConnectionObserver->peerConnection();
}

bool RTCPeerConnection::isClosed()
{
    if (backend()->peer_connection_state() ==
        webrtc::PeerConnectionInterface::PeerConnectionState::kClosed) {
        return true;
    }
    return false;
}
}

#endif
