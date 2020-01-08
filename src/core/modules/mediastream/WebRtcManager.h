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

#include "api/peer_connection_interface.h"

namespace Starfish {
class RTCPeerConnection;

class WebRtcManager : public gc {
public:
    WebRtcManager();
    virtual ~WebRtcManager();

    void dispose();

    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
    peerConnectionFactory();

    rtc::scoped_refptr<webrtc::PeerConnectionInterface> createPeerConnection(
        const webrtc::PeerConnectionInterface::RTCConfiguration& configuration,
        webrtc::PeerConnectionDependencies dependencies);
    void deletePeerConnection();

private:
    void initPeerConnectionFactory();
    void deletePeerConnectionFactory();
};
} // namespace Starfish

#endif
