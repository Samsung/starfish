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

#include "api/create_peerconnection_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "modules/audio_device/include/fake_audio_device.h"

namespace Starfish {

WebRtcManager* WebRtcManager::m_instance = nullptr;

WebRtcManager::WebRtcManager()
{
    STARFISH_LOG_INFO("%s\n", __func__);
    m_peerConnectionFactory = webrtc::CreatePeerConnectionFactory(
        nullptr /* network_thread */, nullptr /* worker_thread */,
        nullptr /* signaling_thread */,
        // TODO: Setup AudioDeviceModule for docker and real machine
        rtc::scoped_refptr<webrtc::AudioDeviceModule>(
            new webrtc::FakeAudioDeviceModule()) /* default_adm: nullptr */,
        webrtc::CreateBuiltinAudioEncoderFactory(),
        webrtc::CreateBuiltinAudioDecoderFactory(),
        webrtc::CreateBuiltinVideoEncoderFactory(),
        webrtc::CreateBuiltinVideoDecoderFactory(), nullptr /* audio_mixer */,
        nullptr /* audio_processing */);

    STARFISH_ASSERT(m_peerConnectionFactory);

    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) { ((WebRtcManager*)obj)->~WebRtcManager(); },
        NULL, NULL, NULL);
}

WebRtcManager::~WebRtcManager()
{
    STARFISH_LOG_INFO("%s\n", __func__);
    m_peerConnection = nullptr;
    m_peerConnectionFactory = nullptr;
    m_instance = nullptr;
}

WebRtcManager* WebRtcManager::instance()
{
    if (m_instance == nullptr) {
        m_instance = new WebRtcManager();
    }
    return m_instance;
}
} // namespace Starfish

#endif
