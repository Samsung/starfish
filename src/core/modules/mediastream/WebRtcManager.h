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

#include "api/peer_connection_interface.h"

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

    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
    peerConnectionFactory();

    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
    createPeerConnectionFactory();

    rtc::scoped_refptr<webrtc::PeerConnectionInterface> createPeerConnection(
        const webrtc::PeerConnectionInterface::RTCConfiguration& configuration,
        webrtc::PeerConnectionDependencies dependencies);
    void deletePeerConnection(RTCPeerConnection* peerConnection);

    rtc::scoped_refptr<webrtc::AudioTrackInterface> createAudioTrack(
        String* label);

    void addPeerConnection(RTCPeerConnection* peerConnection);
    void addMediaStream(MediaStream* mediaStream);
    void addAudioStreamTrack(AudioStreamTrack* audioStreamTrack);
    void addVideoStreamTrack(VideoStreamTrack* videoStreamTrack);

private:
    void initPeerConnectionFactory();
    void deletePeerConnectionFactory(bool force = false);

    GCUnorderedSet<RTCPeerConnection*> m_peerConnections;
    GCVector<MediaStream*> m_mediaStreams;
    GCVector<AudioStreamTrack*> m_audioStreamTracks;
    GCVector<VideoStreamTrack*> m_videoStreamTracks;
};
} // namespace Starfish

#endif
