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

#include "core/modules/mediastream/WebRtcManager.h"
#include "core/modules/mediastream/RTCPeerConnection.h"
#include "core/modules/mediastream/MediaStream.h"
#include "core/modules/mediastream/MediaStreamTrack.h"

#include "api/create_peerconnection_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "modules/audio_device/include/fake_audio_device.h"

namespace Starfish {

static rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
    m_peerConnectionFactory;
static std::unique_ptr<rtc::Thread> m_networkThread;
static std::unique_ptr<rtc::Thread> m_workerThread;
static std::unique_ptr<rtc::Thread> m_signalingThread;
static int peerConnectionCount = 0;

WebRtcManager::WebRtcManager()
{
    WEBRTC_LOGI("<%s/>: %p\n", __func__, (void*)this);

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) { ((WebRtcManager*)obj)->~WebRtcManager(); },
        NULL, NULL, NULL);
}

WebRtcManager::~WebRtcManager()
{
    WEBRTC_LOGI("<%s>: %p\n", __func__, (void*)this);
    dispose();
    WEBRTC_LOGI("</%s>: %p\n", __func__, (void*)this);
}

void WebRtcManager::initPeerConnectionFactory()
{
    WEBRTC_LOGI("<WebRtcManager::%s>\n", __func__);
    m_networkThread = rtc::Thread::CreateWithSocketServer();
    m_networkThread->Start();
    m_workerThread = rtc::Thread::Create();
    m_workerThread->Start();
    m_signalingThread = rtc::Thread::Create();
    m_signalingThread->Start();
    m_peerConnectionFactory = webrtc::CreatePeerConnectionFactory(
        m_networkThread.get() /* network_thread */,
        m_workerThread.get() /* worker_thread */,
        m_signalingThread.get() /* signaling_thread */,
// Setup AudioDeviceModule
#if defined(STARFISH_DOCKER)
        rtc::scoped_refptr<webrtc::AudioDeviceModule>(
            new webrtc::FakeAudioDeviceModule()),
#else
        nullptr, // default_adm: nullptr
#endif
        webrtc::CreateBuiltinAudioEncoderFactory(),
        webrtc::CreateBuiltinAudioDecoderFactory(),
        webrtc::CreateBuiltinVideoEncoderFactory(),
        webrtc::CreateBuiltinVideoDecoderFactory(), nullptr /* audio_mixer */,
        nullptr /* audio_processing */);
    WEBRTC_LOGI("</WebRtcManager::%s>\n", __func__);

    STARFISH_ASSERT(m_peerConnectionFactory);
}

void WebRtcManager::deletePeerConnectionFactory(bool force)
{
    if (force || peerConnectionCount == 0) {
        WEBRTC_LOGI("<WebRtcManager::%s>\n", __func__);

        WEBRTC_LOGI("  m_audioStreamTracks.size(): %zu\n",
                    m_audioStreamTracks.size());
        for (auto audioTrack : m_audioStreamTracks) {
            audioTrack->dispose();
        }
        m_audioStreamTracks.clear();

        for (auto videoTrack : m_videoStreamTracks) {
            videoTrack->dispose();
        }
        m_videoStreamTracks.clear();

        for (auto mediaStream : m_mediaStreams) {
            mediaStream->dispose();
        }
        m_mediaStreams.clear();

        GCVector<RTCPeerConnection*> pcs;
        pcs.insert(pcs.end(), m_peerConnections.begin(),
                   m_peerConnections.end());
        for (auto pc : pcs) {
            pc->dispose();
        }
        m_peerConnections.clear();
        peerConnectionCount = 0;
        m_peerConnectionFactory = nullptr;
        m_networkThread.reset();
        m_workerThread.reset();
        m_signalingThread.reset();
        WEBRTC_LOGI("</WebRtcManager::%s>\n", __func__);
    }
}

rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
WebRtcManager::peerConnectionFactory()
{
    return m_peerConnectionFactory;
}

rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
WebRtcManager::createPeerConnectionFactory()
{
    if (m_peerConnectionFactory == nullptr) {
        initPeerConnectionFactory();
    }
    return m_peerConnectionFactory;
}

rtc::scoped_refptr<webrtc::PeerConnectionInterface>
WebRtcManager::createPeerConnection(
    const webrtc::PeerConnectionInterface::RTCConfiguration& configuration,
    webrtc::PeerConnectionDependencies dependencies)
{
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> pc =
        createPeerConnectionFactory()->CreatePeerConnection(
            configuration, std::move(dependencies));
    peerConnectionCount++;
    return pc;
}

void WebRtcManager::deletePeerConnection(RTCPeerConnection* peerConnection)
{
    WEBRTC_LOGI("<WebRtcManager::%s>\n", __func__);
    peerConnectionCount--;
    WEBRTC_LOGI("  peerConnectionCount: %d\n", peerConnectionCount);
    if (peerConnection) {
        m_peerConnections.erase(m_peerConnections.find(peerConnection));
    }
    deletePeerConnectionFactory();
    WEBRTC_LOGI("</WebRtcManager::%s>\n", __func__);
}

rtc::scoped_refptr<webrtc::AudioTrackInterface> WebRtcManager::createAudioTrack(
    String* label)
{
    rtc::scoped_refptr<webrtc::AudioSourceInterface> audioDevice =
        m_peerConnectionFactory->CreateAudioSource(cricket::AudioOptions());

    rtc::scoped_refptr<webrtc::AudioTrackInterface> audioTrack;
    if (audioDevice) {
        std::string labelStr = label->toUTF8NonGCString().data();
        audioTrack =
            m_peerConnectionFactory->CreateAudioTrack(labelStr, audioDevice);
    } else {
        STARFISH_LOG_ERROR("AudioStreamTrack: failed\n");
    }

    return audioTrack;
}

void WebRtcManager::addPeerConnection(RTCPeerConnection* peerConnection)
{
    m_peerConnections.insert(peerConnection);
}

void WebRtcManager::addMediaStream(MediaStream* mediaStream)
{
    m_mediaStreams.push_back(mediaStream);
}

void WebRtcManager::addAudioStreamTrack(AudioStreamTrack* audioStreamTrack)
{
    WEBRTC_LOGI("<WebRtcManager::%s/>\n", __func__);
    m_audioStreamTracks.push_back(audioStreamTrack);
    WEBRTC_LOGI("    now: m_audioStreamTracks.size(): %zu\n",
                m_audioStreamTracks.size());
}

void WebRtcManager::addVideoStreamTrack(VideoStreamTrack* videoStreamTrack)
{
    WEBRTC_LOGI("<WebRtcManager::%s/>\n", __func__);
    m_videoStreamTracks.push_back(videoStreamTrack);
    WEBRTC_LOGI("    now: m_videoStreamTracks.size(): %zu\n",
                m_videoStreamTracks.size());
}

void WebRtcManager::dispose()
{
    WEBRTC_LOGI("<WebRtcManager::%s>: %p\n", __func__, (void*)this);
    WEBRTC_LOGI("  peerConnectionCount: %d\n", peerConnectionCount);
    deletePeerConnectionFactory(true);
    WEBRTC_LOGI("</WebRtcManager::%s>: %p\n", __func__, (void*)this);
}
} // namespace Starfish

#endif
