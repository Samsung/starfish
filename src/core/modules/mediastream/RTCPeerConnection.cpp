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

#include "api/rtp_transceiver_interface.h"
#include "api/sctp_transport_interface.h"
#include "api/media_stream_interface.h"
#include "rtc_base/physical_socket_server.h"
#include "rtc_base/strings/json.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"

namespace Starfish {

ExecutionContext* PeerConnectionObserver::executionContext() const
{
    return m_peerConnection->executionContext();
}

PeerConnectionObserver::PeerConnectionObserver(
    RTCPeerConnection* peerConnection)
    : m_peerConnection(peerConnection)
{
    WEBRTC_LOGI("<PeerConnectionObserver::%s/>: %p : %p\n", __func__,
                (void*)this, (void*)m_peerConnection);

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
    WEBRTC_LOGI("<PeerConnectionObserver::%s self=%p pc=%p>\n", __func__,
                (void*)this, (void*)m_peerConnection);

    if (m_webRtcManager->peerConnectionFactory()) {
        if (m_peerConnection) {
            m_peerConnection->dispose();
        }
    }
    m_peerConnection = nullptr;
    WEBRTC_LOGI("</PeerConnectionObserver::%s self=%p pc=%p>\n", __func__,
                (void*)this, (void*)m_peerConnection);
}

void PeerConnectionObserver::OnSignalingChange(
    webrtc::PeerConnectionInterface::SignalingState newState)
{
    WEBRTC_LOGI("<PeerConnectionObserver::%s self=%p pc=%p>\n", __func__,
                (void*)this, (void*)m_peerConnection);

    if (newState == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
        return;
    }

    if (!m_peerConnection || m_peerConnection->isClosed()) {
        return;
    }

    struct Params {
        PeerConnectionObserver* self;
        webrtc::PeerConnectionInterface::SignalingState newState;
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
                    WEBRTC_LOGI(
                        "</PeerConnectionObserver::OnSignalingChange1 "
                        "self=%p>\n",
                        (void*)p->self);
                    return;
                }

                String* eventType = self->executionContext()
                                        ->starfish()
                                        ->staticStrings()
                                        ->m_signalingstatechange.localName();
                Event* e = new Event(self->executionContext(), eventType);
                self->m_peerConnection->dispatchEventByUA(e);
                WEBRTC_LOGI(
                    "</PeerConnectionObserver::OnSignalingChange2 self=%p>\n",
                    (void*)p->self);
                delete p;
            },
            p);
}

void PeerConnectionObserver::OnAddStream(
    rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
}

#if defined(STARFISH_WEBRTC_DEBUG)
class AudioTrackObserver : public webrtc::AudioTrackSinkInterface {
    virtual void OnData(const void* audioData, int bitsPerSample,
                        int sampleRate, size_t numberOfChannels,
                        size_t numberOfFrames)
    {
    }
};

class VideoFrameObserver : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
public:
    void OnFrame(const webrtc::VideoFrame& frame) override
    {
    }
};
#endif

void PeerConnectionObserver::OnTrack(
    rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    WEBRTC_LOGI("<PeerConnectionObserver::%s self=%p pc=%p>\n", __func__,
                (void*)this, (void*)m_peerConnection);

    if (!isMainThread()) {
        if (!m_peerConnection || m_peerConnection->isClosed()) {
            return;
        }

        struct Params {
            PeerConnectionObserver* self;
            rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver;
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
                    WEBRTC_LOGI("</PeerConnectionObserver::OnTrack self=%p>\n",
                                (void*)p->self);
                    delete p;
                },
                p);
        return;
    }

    m_peerConnection->syncTransceivers();
    RTCRtpTransceiver* rtpTransceiver =
        m_peerConnection->getTransceiver(transceiver);
    RTCRtpReceiver* rtpReceiver = rtpTransceiver->receiver();
    MediaStreamTrack* track = rtpReceiver->track();

    GCVector<MediaStream*> rtpStreams;
    for (auto stream : rtpReceiver->streams()) {
        stream->addTrack(track);
        rtpStreams.push_back(stream);
    }

#if defined(STARFISH_WEBRTC_DEBUG)
    if (track->isAudioStreamTrack()) {
        AudioTrackObserver* audioTrackObserver = new AudioTrackObserver();
        track->asAudioStreamTrack()->backend()->AddSink(audioTrackObserver);
    } else if (track->isVideoStreamTrack()) {
        VideoFrameObserver* videoFrameObserver = new VideoFrameObserver();
        track->asVideoStreamTrack()->backend()->AddOrUpdateSink(
            videoFrameObserver, rtc::VideoSinkWants());
    }
#endif

    String* eventType =
        executionContext()->starfish()->staticStrings()->m_track.localName();
    RTCTrackEventInit init(rtpReceiver, track, rtpStreams, rtpTransceiver);
    RTCTrackEvent* e = new RTCTrackEvent(executionContext(), eventType, init);
    m_peerConnection->dispatchEventByUA(e);
}

// https://w3c.github.io/webrtc-pc/#event-datachannel
void PeerConnectionObserver::OnDataChannel(
    rtc::scoped_refptr<webrtc::DataChannelInterface> channel)
{
    WEBRTC_LOGI("<PeerConnectionObserver::%s self=%p pc=%p>\n", __func__,
                (void*)this, (void*)m_peerConnection);

    if (!isMainThread()) {
        if (!m_peerConnection || m_peerConnection->isClosed()) {
            return;
        }

        struct Params {
            PeerConnectionObserver* self;
            rtc::scoped_refptr<webrtc::DataChannelInterface> channel;
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
                        return;
                    }

                    p->self->OnDataChannel(p->channel);
                    WEBRTC_LOGI(
                        "</PeerConnectionObserver::OnDataChannel self=%p>\n",
                        (void*)p->self);
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
    WEBRTC_LOGI("<PeerConnectionObserver::%s self=%p pc=%p>\n", __func__,
                (void*)this, (void*)m_peerConnection);

    if (!isMainThread()) {
        if (!m_peerConnection || m_peerConnection->isDisposed() ||
            m_peerConnection->isClosed()) {
            WEBRTC_LOGI("</PeerConnectionObserver::%s1 self=%p pc=%p>\n",
                        __func__, (void*)this, (void*)m_peerConnection);
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
                        WEBRTC_LOGI(
                            "<PeerConnectionObserver::OnRenegotiationNeeded2 "
                            "pc=%p>\n",
                            (void*)self->m_peerConnection);
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
    WEBRTC_LOGI("</PeerConnectionObserver::%s self=%p pc:%p>\n", __func__,
                (void*)this, (void*)m_peerConnection);
    return;
}

void PeerConnectionObserver::OnIceConnectionChange(
    webrtc::PeerConnectionInterface::IceConnectionState newState)
{
    WEBRTC_LOGI("<PeerConnectionObserver::%s self=%p pc=%p>\n", __func__,
                (void*)this, (void*)m_peerConnection);

    if (newState == webrtc::PeerConnectionInterface::IceConnectionState::
                        kIceConnectionClosed ||
        newState ==
            webrtc::PeerConnectionInterface::kIceConnectionDisconnected) {
        WEBRTC_LOGI(
            "</PeerConnectionObserver::OnIceConnectionChange1 self=%p>\n",
            (void*)this);
        return;
    }

    if (!isMainThread()) {
        if (!m_peerConnection || m_peerConnection->isClosed()) {
            WEBRTC_LOGI(
                "</PeerConnectionObserver::OnIceConnectionChange2 self=%p>\n",
                (void*)this);
            return;
        }

        struct Params {
            PeerConnectionObserver* self;
            webrtc::PeerConnectionInterface::IceConnectionState newState;
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
                        WEBRTC_LOGI(
                            "</PeerConnectionObserver::OnIceConnectionChange3 "
                            "self=%p>\n",
                            (void*)p->self);
                        return;
                    }

                    p->self->OnIceConnectionChange(p->newState);
                    WEBRTC_LOGI(
                        "</PeerConnectionObserver::OnIceConnectionChange "
                        "self=%p>\n",
                        (void*)p->self);
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

void PeerConnectionObserver::OnConnectionChange(
    webrtc::PeerConnectionInterface::PeerConnectionState newState)
{
    WEBRTC_LOGI("<PeerConnectionObserver::%s self=%p pc=%p>\n", __func__,
                (void*)this, (void*)m_peerConnection);

    if (newState ==
        webrtc::PeerConnectionInterface::PeerConnectionState::kClosed) {
        return;
    }

    if (!isMainThread()) {
        if (!m_peerConnection || m_peerConnection->isClosed()) {
            return;
        }

        struct Params {
            PeerConnectionObserver* self;
            webrtc::PeerConnectionInterface::PeerConnectionState newState;
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
                        return;
                    }

                    p->self->OnConnectionChange(p->newState);
                    WEBRTC_LOGI(
                        "</PeerConnectionObserver::OnConnectionChange "
                        "self=%p>\n",
                        (void*)p->self);
                    delete p;
                },
                p);
        return;
    }

    String* eventType = executionContext()
                            ->starfish()
                            ->staticStrings()
                            ->m_connectionstatechange.localName();
    Event* e = new Event(executionContext(), eventType);
    m_peerConnection->dispatchEventByUA(e);
}

void PeerConnectionObserver::OnIceGatheringChange(
    webrtc::PeerConnectionInterface::IceGatheringState newState)
{
    WEBRTC_LOGI("<PeerConnectionObserver::%s self=%p pc=%p>\n", __func__,
                (void*)this, (void*)m_peerConnection);

    if (newState == webrtc::PeerConnectionInterface::IceGatheringState::
                        kIceGatheringComplete) {
        return;
    }

    if (!isMainThread()) {
        if (!m_peerConnection || m_peerConnection->isClosed()) {
            return;
        }

        struct Params {
            PeerConnectionObserver* self;
            webrtc::PeerConnectionInterface::IceGatheringState newState;
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
                        return;
                    }

                    p->self->OnIceGatheringChange(p->newState);
                    WEBRTC_LOGI(
                        "</PeerConnectionObserver::OnIceGatheringChange "
                        "self=%p>\n",
                        (void*)p->self);
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
    const webrtc::IceCandidateInterface* candidate)
{
    WEBRTC_LOGI("<PeerConnectionObserver::%s self=%p pc=%p>\n", __func__,
                (void*)this, (void*)m_peerConnection);

    if (!m_peerConnection || m_peerConnection->isClosed()) {
        return;
    }

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
                    return;
                }

                webrtc::IceCandidateInterface* candidate =
                    webrtc::CreateIceCandidate(p->sdpMid, p->sdpMlineIndex,
                                               p->sdp, nullptr);

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

                WEBRTC_LOGI(
                    "</PeerConnectionObserver::OnIceCandidate self=%p>\n",
                    (void*)p->self);
                delete p;
            },
            p);
}

void CreateOfferAnswerObserver::OnSuccess(
    webrtc::SessionDescriptionInterface* desc)
{
    WEBRTC_LOGI("<CreateOfferAnswerObserver::%s>: %p\n", __func__, (void*)this);

    if (!m_peerConnection) {
        return;
    }

    // This callback is called from another thread
    // The following lines must be executed in the main thread.
    struct Params {
        CreateOfferAnswerObserver* self;
        std::unique_ptr<webrtc::SessionDescriptionInterface> desc;
    };

    Params* p = new Params{
        this, std::unique_ptr<webrtc::SessionDescriptionInterface>(desc)
    };

    m_peerConnection->executionContext()
        ->webBase()
        ->messageLoop()
        ->addIdlerWithNoGCRootingInOtherThread(
            m_peerConnection->executionContext()->globalScope(),
            [](size_t, void* data) {
                Params* p = (Params*)data;
                CreateOfferAnswerObserver* self = p->self;
                webrtc::SessionDescriptionInterface* desc = p->desc.get();

                RTCSdpType type =
                    self->m_peerConnection->toRtcSdpType(desc->GetType())
                        .value();
                std::string sdpString;
                desc->ToString(&sdpString);

                ObjectRef* sd =
                    self->m_peerConnection->createSessionDescriptionInitObject(
                        type, String::createASCIIString(sdpString.c_str(),
                                                        sdpString.size()));
                Promise* promise = nullptr;
                if (self->isCreateOffer()) {
                    promise = self->m_peerConnection->m_createOfferObserver
                                  ->promise();
                    self->m_peerConnection->m_createOfferObserver->setPromise(
                        nullptr);
                    self->m_peerConnection->m_lastCreatedOffer = sdpString;
                    self->m_peerConnection->syncTransceivers();
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
                    WEBRTC_LOGE("%s: unknown promise type\n", __func__);
                }
                delete p;
            },
            p);

    WEBRTC_LOGI("<CreateOfferAnswerObserver::/%s>: %p\n", __func__,
                (void*)this);
}

void CreateOfferAnswerObserver::OnFailure(webrtc::RTCError error)
{
    WEBRTC_LOGI("<CreateOfferAnswerObserver::%s>: %p\n", __func__, (void*)this);

    // This callback is called from another thread
    // The following lines must be executed in the main thread.
    struct Params {
        CreateOfferAnswerObserver* self;
        webrtc::RTCError error;
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
                    WEBRTC_LOGE("%s: unknown promise type\n", __func__);
                }

                delete p;
            },
            p);

    WEBRTC_LOGI("</CreateOfferAnswerObserver::%s>: %p\n", __func__,
                (void*)this);
}

void SetLocalRemoteDescriptionObserver::OnSuccess()
{
    WEBRTC_LOGI("<SetLocalRemoteDescriptionObserver::%s>: %p\n", __func__,
                (void*)this);

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
        WEBRTC_LOGE("%s: unknown promise type\n", __func__);
    }
    WEBRTC_LOGI("</SetLocalRemoteDescriptionObserver::%s>: %p\n", __func__,
                (void*)this);
}

void SetLocalRemoteDescriptionObserver::OnFailure(webrtc::RTCError error)
{
    WEBRTC_LOGI("<SetLocalRemoteDescriptionObserver::%s>: %p\n", __func__,
                (void*)this);

    // This callback is called from another thread
    // The following lines must be executed in the main thread.
    struct Params {
        SetLocalRemoteDescriptionObserver* self;
        webrtc::RTCError error;
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
                    return;
                }

                DOMException* exception =
                    self->m_peerConnection->toDomException(std::move(p->error));
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
                    WEBRTC_LOGE("%s: unknown promise type\n", __func__);
                }
                delete p;
            },
            p);

    WEBRTC_LOGI("</SetLocalRemoteDescriptionObserver::%s>: %p\n", __func__,
                (void*)this);
}

// https://w3c.github.io/webrtc-pc/#constructor
RTCPeerConnection::RTCPeerConnection(ExecutionContext* executionContext,
                                     RTCConfiguration configuration)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_disposeLock(new Mutex())
{
    WEBRTC_LOGI("<RTCPeerConnection::%s> %p\n", __func__, (void*)this);

    if (!configuration.certificates().empty()) {
        // TODO
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
        STARFISH_LOG_ERROR("%s: PeerConnection: failed\n", __func__);
        WEBRTC_LOGI("</RTCPeerConnection::%s> %p\n", __func__, (void*)this);
        throw new DOMException(executionContext, DOMException::DOM_EXCEPTION,
                               "Invalid Configuration");
    }

    m_webRtcManager->addPeerConnection(this);

    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj,
                 void* cd) { ((RTCPeerConnection*)obj)->~RTCPeerConnection(); },
        NULL, NULL, NULL);

    WEBRTC_LOGI("</RTCPeerConnection::%s> %p\n", __func__, (void*)this);
}

bool RTCPeerConnection::initializePeerConnection()
{
    return initializePeerConnection(m_configuration);
}

bool RTCPeerConnection::initializePeerConnection(
    RTCConfiguration& configuration)
{
    m_peerConnectionObserver = new PeerConnectionObserver(this);
    STARFISH_ASSERT(m_webRtcManager->createPeerConnectionFactory());

    webrtc::PeerConnectionInterface::RTCConfiguration config =
        configuration.genBackend();
    config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;

    webrtc::PeerConnectionDependencies dependencies{ m_peerConnectionObserver };
    m_backend =
        m_webRtcManager->createPeerConnection(config, std::move(dependencies));

    m_createOfferObserver = new PcObserver<CreateOfferObserver>(this);
    m_createAnswerObserver = new PcObserver<CreateAnswerObserver>(this);
    m_setLocalDescriptionObserver =
        new PcObserver<SetLocalDescriptionObserver>(this);
    m_setRemoteDescriptionObserver =
        new PcObserver<SetRemoteDescriptionObserver>(this);

    return m_backend != nullptr;
}

RTCPeerConnection::~RTCPeerConnection()
{
    WEBRTC_LOGI("<RTCPeerConnection::%s>: %p : %p\n", __func__, (void*)this,
                (void*)m_peerConnectionObserver);
    dispose();
    WEBRTC_LOGI("</RTCPeerConnection::%s>: %p\n", __func__, (void*)this);
}

void RTCPeerConnection::dispose()
{
    WEBRTC_LOGI("<RTCPeerConnection::%s>: %p\n", __func__, (void*)this);

    if (isDisposed()) {
        return;
    }

    if (!m_webRtcManager->peerConnectionFactory()) {
        WEBRTC_LOGI("    factory: %p\n",
                    (void*)m_webRtcManager->peerConnectionFactory().get());
        m_backend.release();
        m_peerConnectionObserver = nullptr;
        WEBRTC_LOGI("</RTCPeerConnection::%s_1>: %p\n", __func__, (void*)this);
        return;
    }

    m_peerConnectionObserver->m_peerConnection = nullptr;
    m_createOfferObserver->m_observer->m_peerConnection = nullptr;
    m_createAnswerObserver->m_observer->m_peerConnection = nullptr;
    m_setLocalDescriptionObserver->m_observer->m_peerConnection = nullptr;
    m_setRemoteDescriptionObserver->m_observer->m_peerConnection = nullptr;

    for (auto dataChannel : m_dataChannels) {
        dataChannel->dispose();
        dataChannel->m_peerConnection = nullptr;
        if (dataChannel->m_observer) {
            dataChannel->m_observer->m_dataChannel = nullptr;
        }
        dataChannel->m_backend = nullptr;
    }
    m_dataChannels.clear();

    for (auto transceiver : m_transceivers) {
        transceiver->dispose();
    }
    m_transceivers.clear();

    m_backend = nullptr;
    m_peerConnectionObserver = nullptr;
    m_webRtcManager->deletePeerConnection(this);
    m_webRtcManager = nullptr;
    WEBRTC_LOGI("</RTCPeerConnection::%s>: %p\n", __func__, (void*)this);
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
    WEBRTC_LOGI("<RTCPeerConnection::%s>: %p\n", __func__, (void*)this);

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

    Promise* promise = new Promise(scriptBindingInstance());
    m_createOfferObserver->setPromise(promise);

    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions opt(
        options.m_offerToReceiveVideo, options.m_offerToReceiveAudio,
        options.m_voiceActivityDetection, options.m_iceRestart, true);

    m_backend->CreateOffer(m_createOfferObserver->m_observer, opt);

    WEBRTC_LOGI("</RTCPeerConnection::%s>: %p\n", __func__, (void*)this);
    return promise;
}

// https://w3c.github.io/webrtc-pc/#dom-rtcpeerconnection-createanswer
Promise* RTCPeerConnection::createAnswer(RTCAnswerOptions options)
{
    WEBRTC_LOGI("<RTCPeerConnection::%s>: %p\n", __func__, (void*)this);

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

    Promise* promise = new Promise(scriptBindingInstance());
    m_createAnswerObserver->setPromise(promise);

    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions opt(
        webrtc::PeerConnectionInterface::RTCOfferAnswerOptions::
            kOfferToReceiveMediaTrue,
        webrtc::PeerConnectionInterface::RTCOfferAnswerOptions::
            kOfferToReceiveMediaTrue,
        options.m_voiceActivityDetection, false, true);

    m_backend->CreateAnswer(m_createAnswerObserver->m_observer, opt);

    WEBRTC_LOGI("</RTCPeerConnection::%s>: %p\n", __func__, (void*)this);
    return promise;
}

void RTCPeerConnection::syncTransceivers()
{
    GCUnorderedMap<webrtc::RtpTransceiverInterface*, RTCRtpTransceiver*>
        curTransceivers;
    for (auto transceiver : m_transceivers) {
        curTransceivers.insert(
            std::make_pair(transceiver->backend().get(), transceiver));
    }
    m_transceivers.clear();

    std::vector<rtc::scoped_refptr<webrtc::RtpTransceiverInterface>>
        backendTransceivers = m_backend->GetTransceivers();
    for (auto transceiver : backendTransceivers) {
        auto itr = curTransceivers.find(transceiver.get());
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
    rtc::scoped_refptr<webrtc::RtpTransceiverInterface> backendTransceiver)
{
    for (auto transceiver : m_transceivers) {
        if (transceiver->backend() == backendTransceiver.get()) {
            return transceiver;
        }
    }
    return nullptr;
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
    WEBRTC_LOGI("<RTCPeerConnection::%s>: %p\n", __func__, (void*)this);

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

    Promise* promise = new Promise(scriptBindingInstance());
    m_setLocalDescriptionObserver->setPromise(promise);

    Nullable<webrtc::SdpType> type = toSdpType(description.m_type);
    if (!type.hasValue()) {
        STARFISH_LOG_ERROR("%s: rollback is not supported\n", __func__);
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

    WEBRTC_LOGI("</RTCPeerConnection::%s>: %p\n", __func__, (void*)this);
    // 4.6
    return setRtcSessionDescription({ type, sdpString }, promise, false);
}

// https://w3c.github.io/webrtc-pc/#set-the-rtcsessiondescription
Promise* RTCPeerConnection::setRtcSessionDescription(
    RTCSessionDescriptionInit description, Promise* promise, bool isRemote)
{
    WEBRTC_LOGI("<RTCPeerConnection::%s>: %p\n", __func__, (void*)this);

    // 3.1.2
    if (isRemote) {
        if ((description.m_type == RTCSdpType::Answer)) {
            if (!((m_backend->signaling_state() ==
                   webrtc::PeerConnectionInterface::SignalingState::
                       kHaveLocalOffer) ||
                  (m_backend->signaling_state() ==
                   webrtc::PeerConnectionInterface::SignalingState::
                       kHaveRemotePrAnswer))) {
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
                   webrtc::PeerConnectionInterface::SignalingState::
                       kHaveRemoteOffer) ||
                  (m_backend->signaling_state() ==
                   webrtc::PeerConnectionInterface::SignalingState::
                       kHaveLocalPrAnswer))) {
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

    Nullable<webrtc::SdpType> type = toSdpType(description.m_type);
    std::string sdpString = std::string(description.m_sdp->toUTF8NonGCString());

    std::unique_ptr<webrtc::SessionDescriptionInterface> desc =
        webrtc::CreateSessionDescription(type.value(), sdpString);

    if (!desc) {
        STARFISH_LOG_WARN("%s: desc = nullptr\n", __func__);
        return promise;
    }

    if (isRemote) {
        // SetRemoteDescription takes the ownership of desc
        m_backend->SetRemoteDescription(
            m_setRemoteDescriptionObserver->m_observer, desc.release());
    } else {
        // SetLocalDescription takes the ownership of desc
        m_backend->SetLocalDescription(
            m_setLocalDescriptionObserver->m_observer, desc.release());
    }

    WEBRTC_LOGI("</RTCPeerConnection::%s>: %p\n", __func__, (void*)this);
    return promise;
}

RTCSessionDescription* RTCPeerConnection::localDescription()
{
    const webrtc::SessionDescriptionInterface* desc =
        m_backend->local_description();
    if (!desc) {
        return nullptr;
    }

    return new RTCSessionDescription(executionContext(), desc);
}

RTCSessionDescription* RTCPeerConnection::currentLocalDescription()
{
    const webrtc::SessionDescriptionInterface* desc =
        m_backend->current_local_description();
    if (!desc) {
        return nullptr;
    }

    return new RTCSessionDescription(executionContext(), desc);
}

RTCSessionDescription* RTCPeerConnection::pendingLocalDescription()
{
    const webrtc::SessionDescriptionInterface* desc =
        m_backend->pending_local_description();
    if (!desc) {
        return nullptr;
    }

    return new RTCSessionDescription(executionContext(), desc);
}

// https://w3c.github.io/webrtc-pc/#dom-peerconnection-setremotedescription
Promise* RTCPeerConnection::setRemoteDescription(
    RTCSessionDescriptionInit& description)
{
    WEBRTC_LOGI("<RTCPeerConnection::%s>: %p\n", __func__, (void*)this);

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

    Promise* promise = new Promise(scriptBindingInstance());
    m_setRemoteDescriptionObserver->setPromise(promise);

    Nullable<webrtc::SdpType> type = toSdpType(description.m_type);
    if (!type.hasValue()) {
        STARFISH_LOG_ERROR("%s: rollback is not supported\n", __func__);
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
    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::SessionDescriptionInterface> desc =
        webrtc::CreateSessionDescription(type.value(), sdpString, &error);

    if (error.line == "Invalid SDP") {
        RTCErrorInit init;
        init.m_errorDetail = RTCErrorDetailType::SdpSyntaxError;
        auto exception = new RTCError(executionContext(), init);
        promise->reject(exception->scriptValue());
        return promise;
    }

    WEBRTC_LOGI("</RTCPeerConnection::%s>: %p\n", __func__, (void*)this);
    return setRtcSessionDescription({ type, sdpString }, promise, true);
}

RTCSessionDescription* RTCPeerConnection::remoteDescription()
{
    const webrtc::SessionDescriptionInterface* desc =
        m_backend->remote_description();
    if (!desc) {
        return nullptr;
    }

    return new RTCSessionDescription(executionContext(), desc);
}

RTCSessionDescription* RTCPeerConnection::currentRemoteDescription()
{
    const webrtc::SessionDescriptionInterface* desc =
        m_backend->current_remote_description();
    if (!desc) {
        return nullptr;
    }

    return new RTCSessionDescription(executionContext(), desc);
}

RTCSessionDescription* RTCPeerConnection::pendingRemoteDescription()
{
    const webrtc::SessionDescriptionInterface* desc =
        m_backend->pending_remote_description();
    if (!desc) {
        return nullptr;
    }

    return new RTCSessionDescription(executionContext(), desc);
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
    const webrtc::SessionDescriptionInterface* remoteDescription =
        backend()->remote_description();
    if (!remoteDescription) {
        auto exception = new DOMException(executionContext(),
                                          DOMException::INVALID_STATE_ERR,
                                          "InvalidStateError");
        promise->reject(exception->scriptValue());
        return promise;
    }

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

    bool hasValidSdpMid = false;
    if (init.m_sdpMid.hasValue()) {
        size_t lineIndex = 0;
        size_t mediaSectionSize = remoteDescription->number_of_mediasections();
        for (size_t i = 0; i < mediaSectionSize; i++) {
            const webrtc::IceCandidateCollection* candidateCollection =
                remoteDescription->candidates(i);

            for (size_t j = 0; j < candidateCollection->count(); j++) {
                const webrtc::IceCandidateInterface* candidate =
                    candidateCollection->at(j);
                if (candidate->sdp_mid() == sdpMid) {
                    hasValidSdpMid = true;
                    lineIndex = i;
                    break;
                }
            }
        }

        if (hasValidSdpMid && (sdpMLineIndex >= mediaSectionSize)) {
            sdpMLineIndex = lineIndex;
        }
    } else if (init.m_sdpMLineIndex.hasValue()) {
        const webrtc::IceCandidateCollection* candidateCollection =
            remoteDescription->candidates(sdpMLineIndex);

        if (!candidateCollection) {
            auto exception = new DOMException(
                executionContext(), String::createASCIIString("OperationError"),
                String::createASCIIString("OperationError"));
            promise->reject(exception->scriptValue());
            return promise;
        }
    }

    if (init.m_candidate->equals(String::emptyString)) {
        // TODO: add a:end-of-candidates to sdp
        auto exception = new DOMException(executionContext(),
                                          DOMException::NOT_SUPPORTED_ERR,
                                          "NotSupportedError");
        promise->reject(exception->scriptValue());
        return promise;
    }

    std::string sdp(init.m_candidate->toUTF8NonGCString());

    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::IceCandidateInterface> candidate =
        std::unique_ptr<webrtc::IceCandidateInterface>(
            webrtc::CreateIceCandidate(sdpMid, sdpMLineIndex, sdp, &error));

    if (!candidate) {
        auto exception = new DOMException(
            executionContext(), String::createASCIIString("OperationError"),
            String::createASCIIString("OperationError"));

        promise->reject(exception->scriptValue());
        return promise;
    }

    bool r = backend()->AddIceCandidate(candidate.get());
    if (!r) {
        auto exception = new DOMException(
            executionContext(), String::createASCIIString("OperationError"),
            String::createASCIIString("OperationError"));
        promise->reject(exception->scriptValue());
        return promise;
    }

    promise->fulfill(scriptUndefined());
    return promise;
}

String* RTCPeerConnection::signalingState()
{
    if (isClosed()) {
        return String::emptyString;
    }

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
    WEBRTC_LOGI("<RTCPeerConnection::%s>: %p\n", __func__, (void*)this);
    if (isClosed()) {
        return;
    }

    m_closed = true;
    WEBRTC_LOGI("  <RTCPeerConnection::m_backend->close()>\n");
    if (m_backend) {
        m_backend->Close();
    }
    WEBRTC_LOGI("  </RTCPeerConnection::m_backend->close()>\n");
    WEBRTC_LOGI("</RTCPeerConnection::%s>: %p\n", __func__, (void*)this);
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

// https://w3c.github.io/webrtc-pc/#dom-peerconnection-createdatachannel
RTCDataChannel* RTCPeerConnection::createDataChannel(
    String* label, RTCDataChannelInit dataChannelDict)
{
    // 1-4
    if (isClosed()) {
        STARFISH_LOG_ERROR("%s: connection closed\n", __func__);
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "connection closed");
    }
    // 5-15
    if ((label->length() > 65535)) {
        STARFISH_LOG_ERROR("%s: label.length() > 65535\n", __func__);
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_TYPE_ERR,
                               "label.length() > 65535");
    }
    if (!dataChannelDict.negotiated() &&
        (dataChannelDict.protocol()->length() > 65535)) {
        STARFISH_LOG_ERROR("%s: protocol.length() > 65535\n", __func__);
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_TYPE_ERR,
                               "protocol.length() > 65535");
    }

    webrtc::DataChannelInit init;
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
        STARFISH_LOG_ERROR("%s: negotiated=true but id=null\n", __func__);
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_TYPE_ERR,
                               "negotiated=true but id=null");
    }
    if (dataChannelDict.hasMaxPacketLifeTime() &&
        dataChannelDict.hasMaxRetransmits()) {
        STARFISH_LOG_ERROR(
            "%s: both maxPacketLifeTime=true and maxRetransmits=true\n",
            __func__);
        throw new DOMException(
            executionContext(), DOMException::SCRIPT_TYPE_ERR,
            "both maxPacketLifeTime=true and maxRetransmits=true");
    }
    if (dataChannelDict.hasId() && (dataChannelDict.id() >= 65535)) {
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_TYPE_ERR, "id >= 65535");
    }

    rtc::scoped_refptr<webrtc::DataChannelInterface> dataChannel =
        m_backend->CreateDataChannel(
            std::string(label->toUTF8NonGCString().data()), &init);

    if (!dataChannel) {
        STARFISH_LOG_ERROR("%s: Invalid configuration\n", __func__);
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

    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> backendSenders =
        backend()->GetSenders();
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

    std::vector<rtc::scoped_refptr<webrtc::RtpReceiverInterface>>
        backendReceivers = m_backend->GetReceivers();
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
    WEBRTC_LOGI("<RTCPeerConnection::%s>: %p\n", __func__, (void*)this);

    // 5
    if (isClosed()) {
        STARFISH_LOG_ERROR("%s: InvalidStateError\n", __func__);
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }

    // 1-4
    std::vector<std::string> streamIds;
    for (auto stream : streams) {
        streamIds.push_back(stream->backend()->id());
    }

    // 6
    std::string id = "";
    if (track->isAudioStreamTrack()) {
        id = track->asAudioStreamTrack()->backend()->id();
    } else if (track->isVideoStreamTrack()) {
        id = track->asVideoStreamTrack()->backend()->id();
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
        webrtc::RtpTransceiverInit init;
        init.stream_ids = streamIds;

        webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface>> r;
        if (track->isAudioStreamTrack()) {
            r = backend()->AddTrack(track->asAudioStreamTrack()->backend(),
                                    streamIds);
        } else if (track->isVideoStreamTrack()) {
            r = backend()->AddTrack(track->asVideoStreamTrack()->backend(),
                                    streamIds);
        }

        if (!r.ok()) {
            STARFISH_LOG_ERROR("Failed to add an audio/video track: %s\n",
                               r.error().message());
            throw new DOMException(executionContext(),
                                   DOMException::INVALID_ACCESS_ERR,
                                   "InvalidAccessErr");
        }

        syncTransceivers();

        for (auto transceiver : m_transceivers) {
            if (transceiver->sender()->backend() == r.value()) {
                senderToReturn = transceiver->sender();
                senderToReturn->setTrack(track);
                break;
            }
        }

        STARFISH_ASSERT(senderToReturn);
    }

    WEBRTC_LOGI("</RTCPeerConnection::%s>: %p\n", __func__, (void*)this);
    return senderToReturn;
}

// https://w3c.github.io/webrtc-pc/#dom-rtcpeerconnection-removetrack
void RTCPeerConnection::removeTrack(RTCRtpSender* sender)
{
    // 1-3
    if (isClosed()) {
        STARFISH_LOG_ERROR("%s: InvalidStateError\n", __func__);
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
        STARFISH_LOG_ERROR("%s: InvalidAccessError\n", __func__);
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

    m_backend->RemoveTrackNew(sender->backend());
    aliveSender->setTrack(nullptr);

    if (!existingTransceiver) {
        STARFISH_LOG_ERROR("%s: Transceiver not exist\n", __func__);
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

    webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpTransceiverInterface>> r;
    if ((kind && kind->equals("audio")) ||
        (track && track->kind() == MediaStreamTrack::Kind::Audio)) {
        r = m_backend->AddTransceiver(cricket::MediaType::MEDIA_TYPE_AUDIO,
                                      init.toRtpTransceiverInit());
    } else if ((kind && kind->equals("video")) ||
               (track && track->kind() == MediaStreamTrack::Kind::Video)) {
        r = m_backend->AddTransceiver(cricket::MediaType::MEDIA_TYPE_VIDEO,
                                      init.toRtpTransceiverInit());
    }

    if (!r.ok()) {
        STARFISH_LOG_ERROR("%s: internal error\n", __func__);
        throw new DOMException(executionContext(), DOMException::DOM_EXCEPTION,
                               "addTransceiver: internal error");
    }

    syncTransceivers();
    RTCRtpTransceiver* transceiver = getTransceiver(r.value());

    if (track) {
        transceiver->sender()->setTrack(track);
    }

    return transceiver;
}

// https://w3c.github.io/webrtc-pc/#widl-RTCPeerConnection-getStats-Promise-RTCStatsReport--MediaStreamTrack-selector
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
            STARFISH_LOG_ERROR("%s: The track does not exist in this pc\n",
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

rtc::scoped_refptr<webrtc::PeerConnectionInterface> RTCPeerConnection::backend()
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
        webrtc::PeerConnectionInterface::PeerConnectionState::kClosed) {
        return true;
    }
    return false;
}

Nullable<RTCSdpType> RTCPeerConnection::toRtcSdpType(webrtc::SdpType type)
{
    RTCSessionDescriptionInit init(type, std::string(""));
    return init.m_type;
}

Nullable<webrtc::SdpType> RTCPeerConnection::toSdpType(
    Nullable<RTCSdpType> type)
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
    case webrtc::RTCErrorType::INTERNAL_ERROR:
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
