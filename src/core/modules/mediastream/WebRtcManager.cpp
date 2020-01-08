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

#include "core/modules/mediastream/WebRtcManager.h"
#include "core/modules/mediastream/RTCPeerConnection.h"

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
    STARFISH_LOG_INFO("%s\n", __func__);

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) { ((WebRtcManager*)obj)->~WebRtcManager(); },
        NULL, NULL, NULL);
}

WebRtcManager::~WebRtcManager()
{
    WEBRTC_LOGI("%s\n", __func__);
    dispose();
}

void WebRtcManager::initPeerConnectionFactory()
{
    if (m_peerConnectionFactory == nullptr) {
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

            // TODO: Setup AudioDeviceModule for docker and real machine
            rtc::scoped_refptr<webrtc::AudioDeviceModule>(
                new webrtc::FakeAudioDeviceModule()) /* default_adm: nullptr */,
            webrtc::CreateBuiltinAudioEncoderFactory(),
            webrtc::CreateBuiltinAudioDecoderFactory(),
            webrtc::CreateBuiltinVideoEncoderFactory(),
            webrtc::CreateBuiltinVideoDecoderFactory(),
            nullptr /* audio_mixer */, nullptr /* audio_processing */);
        WEBRTC_LOGI("</WebRtcManager::%s>\n", __func__);
    }

    STARFISH_ASSERT(m_peerConnectionFactory);
}

void WebRtcManager::deletePeerConnectionFactory()
{
    if (peerConnectionCount == 0) {
        WEBRTC_LOGI("<WebRtcManager::%s>\n", __func__);
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
    initPeerConnectionFactory();
    return m_peerConnectionFactory;
}

rtc::scoped_refptr<webrtc::PeerConnectionInterface>
WebRtcManager::createPeerConnection(
    const webrtc::PeerConnectionInterface::RTCConfiguration& configuration,
    webrtc::PeerConnectionDependencies dependencies)
{
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> pc =
        peerConnectionFactory()->CreatePeerConnection(configuration,
                                                      std::move(dependencies));
    peerConnectionCount++;
    return pc;
}

void WebRtcManager::deletePeerConnection()
{
    WEBRTC_LOGI("<WebRtcManager::%s>\n", __func__);
    peerConnectionCount--;
    WEBRTC_LOGI("  peerConnectionCount: %d\n", peerConnectionCount);
    deletePeerConnectionFactory();
    WEBRTC_LOGI("</WebRtcManager::%s>\n", __func__);
}

void WebRtcManager::dispose()
{
    WEBRTC_LOGI("WebRtcManager::%s\n", __func__);
    WEBRTC_LOGI("  peerConnectionCount: %d\n", peerConnectionCount);
    m_peerConnectionFactory = nullptr;
    WEBRTC_LOGI("/WebRtcManager::%s\n", __func__);
}
} // namespace Starfish

#endif
