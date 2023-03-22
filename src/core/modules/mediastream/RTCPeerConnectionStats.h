/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishRTCPeerConnectionStats__
#define __StarfishRTCPeerConnectionStats__

#include "core/modules/mediastream/RTCStats.h"

namespace Starfish {

struct RTCPeerConnectionStats : public RTCStats {
public:
    RTCPeerConnectionStats() = default;
    RTCPeerConnectionStats(RTCStats rtcStats);

    STARFISH_MAKE_STACK_ALLOCATED()

    DEFINE_GETTER_SETTER(uint32_t, dataChannelsOpened, DataChannelsOpened)
    DEFINE_GETTER_SETTER(uint32_t, dataChannelsClosed, DataChannelsClosed)

    void fillStats(
        libwebrtc::scoped_refptr<libwebrtc::MediaRTCStats> mediaRTCStats);

private:
    uint32_t m_dataChannelsOpened = 0;
    uint32_t m_dataChannelsClosed = 0;
};

} // namespace Starfish

#endif
#endif
