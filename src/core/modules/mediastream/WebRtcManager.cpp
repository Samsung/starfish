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

#include "libwebrtc.h"

namespace Starfish {

static libwebrtc::scoped_refptr<libwebrtc::RTCPeerConnectionFactory>
    m_peerConnectionFactory;
static int peerConnectionCount = 0;

WebRtcManager::WebRtcManager()
{
    libwebrtc::LibWebRTC::Initialize();
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) { ((WebRtcManager*)obj)->~WebRtcManager(); },
        NULL, NULL, NULL);
}

WebRtcManager::~WebRtcManager()
{
    if (!isDisposed()) {
        dispose();
    }
}

void WebRtcManager::deletePeerConnectionFactory(bool force)
{
    if (force || peerConnectionCount == 0) {
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
    }
}

libwebrtc::scoped_refptr<libwebrtc::RTCPeerConnectionFactory>
WebRtcManager::peerConnectionFactory()
{
    if (m_peerConnectionFactory.get() == nullptr) {
        m_peerConnectionFactory =
            libwebrtc::LibWebRTC::CreateRTCPeerConnectionFactory();
        m_audioDevice = m_peerConnectionFactory->GetAudioDevice();
        m_videoDevice = m_peerConnectionFactory->GetVideoDevice();
    }
    return m_peerConnectionFactory;
}

libwebrtc::scoped_refptr<libwebrtc::RTCPeerConnection>
WebRtcManager::createPeerConnection(
    const libwebrtc::RTCConfiguration& configuration)
{
    libwebrtc::scoped_refptr<libwebrtc::RTCMediaConstraints> constraints =
        libwebrtc::RTCMediaConstraints::Create();
    // TODO : FIXME
    libwebrtc::scoped_refptr<libwebrtc::RTCPeerConnection> pc =
        peerConnectionFactory()->Create(configuration, constraints);
    peerConnectionCount++;
    return pc;
}

void WebRtcManager::deletePeerConnection(RTCPeerConnection* peerConnection)
{
    if (peerConnection) {
        peerConnectionCount--;
        m_peerConnections.erase(m_peerConnections.find(peerConnection));
    }
    deletePeerConnectionFactory();
}

libwebrtc::scoped_refptr<libwebrtc::RTCAudioTrack>
WebRtcManager::createAudioTrack(String* label)
{
    libwebrtc::scoped_refptr<libwebrtc::RTCMediaConstraints> audioConstraints =
        libwebrtc::RTCMediaConstraints::Create();
    std::string sourceId = "";
    std::string deviceId = "";

    char strRecordingName[256] = {
        0,
    };
    char strRecordingGuid[256] = {
        0,
    };

    int playoutDevices = m_audioDevice->PlayoutDevices();
    int recordingDevices = m_audioDevice->RecordingDevices();

    for (uint16_t i = 0; i < recordingDevices; i++) {
        // TODO : FIXME
        m_audioDevice->RecordingDeviceName(i, strRecordingName,
                                           strRecordingGuid);
        m_audioDevice->SetRecordingDevice(i);
        break;
    }

    char strPlayoutName[256] = {
        0,
    };
    char strPlayoutGuid[256] = {
        0,
    };

    for (uint16_t i = 0; i < playoutDevices; i++) {
        // TODO : FIXME
        m_audioDevice->PlayoutDeviceName(i, strPlayoutName, strPlayoutGuid);
        m_audioDevice->SetPlayoutDevice(i);
        break;
    }
    libwebrtc::scoped_refptr<libwebrtc::RTCAudioTrack> audioTrack;
    libwebrtc::scoped_refptr<libwebrtc::RTCAudioSource> audioSource =
        m_peerConnectionFactory->CreateAudioSource("audio_input");
    if (audioSource) {
        std::string labelStr = label->toUTF8NonGCString().data();
        audioTrack = m_peerConnectionFactory->CreateAudioTrack(
            audioSource, labelStr.c_str());
    } else {
        STARFISH_LOG_ERROR("AudioStreamTrack: failed");
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
    m_audioStreamTracks.push_back(audioStreamTrack);
}

void WebRtcManager::addVideoStreamTrack(VideoStreamTrack* videoStreamTrack)
{
    m_videoStreamTracks.push_back(videoStreamTrack);
}

void WebRtcManager::dispose()
{
    if (isDisposed()) {
        STARFISH_LOG_WARN("WebRtcManager[%p] is already disposed.", this);
        return;
    }
    deletePeerConnectionFactory(true);
}
} // namespace Starfish

#endif
