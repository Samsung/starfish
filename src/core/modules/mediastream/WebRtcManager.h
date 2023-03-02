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

#include "rtc_peerconnection.h"
#include "rtc_peerconnection_factory.h"
#include "rtc_video_device.h"
#include "rtc_audio_device.h"

namespace Starfish {
class RTCPeerConnection;
class MediaStream;
class AudioStreamTrack;
class VideoStreamTrack;
class PeerConnectionObserver;

class WebRtcManager : public gc {
public:
    WebRtcManager();
    virtual ~WebRtcManager();

    void dispose();

    libwebrtc::scoped_refptr<libwebrtc::RTCPeerConnectionFactory>
    peerConnectionFactory();

    libwebrtc::scoped_refptr<libwebrtc::RTCPeerConnection> createPeerConnection(
        const libwebrtc::RTCConfiguration& configuration);
    void deletePeerConnection(RTCPeerConnection* peerConnection);

    libwebrtc::scoped_refptr<libwebrtc::RTCAudioTrack> createAudioTrack(
        String* label);

    void addPeerConnection(RTCPeerConnection* peerConnection);
    void addMediaStream(MediaStream* mediaStream);
    void addAudioStreamTrack(AudioStreamTrack* audioStreamTrack);
    void addVideoStreamTrack(VideoStreamTrack* videoStreamTrack);

    libwebrtc::scoped_refptr<libwebrtc::RTCAudioDevice> audioDevice()
    {
        return m_audioDevice;
    }

    libwebrtc::scoped_refptr<libwebrtc::RTCVideoDevice> videoDevice()
    {
        return m_videoDevice;
    }

private:
    void deletePeerConnectionFactory(bool force = false);

    GCUnorderedSet<RTCPeerConnection*> m_peerConnections;
    GCVector<MediaStream*> m_mediaStreams;
    GCVector<AudioStreamTrack*> m_audioStreamTracks;
    GCVector<VideoStreamTrack*> m_videoStreamTracks;
    libwebrtc::scoped_refptr<libwebrtc::RTCPeerConnectionFactory>
        m_peerConnectionFactory;
    libwebrtc::scoped_refptr<libwebrtc::RTCAudioDevice> m_audioDevice;
    libwebrtc::scoped_refptr<libwebrtc::RTCVideoDevice> m_videoDevice;
};
} // namespace Starfish

#endif
