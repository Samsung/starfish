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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/mediastream/RTCPeerConnectionStats.h"

#include "EscargotPublic.h"

using namespace Escargot;

namespace Starfish {

RTCPeerConnectionStats::RTCPeerConnectionStats(RTCStats rtcStats)
{
    m_timestamp = rtcStats.timestamp();
    m_id = rtcStats.id();
    m_type = rtcStats.type();
}

void RTCPeerConnectionStats::fillStats(
    libwebrtc::scoped_refptr<libwebrtc::MediaRTCStats> mediaRTCStats)
{
    for (auto member : mediaRTCStats->Members().std_vector()) {
        std::string name = member->GetName().std_string();
        if (name == "dataChannelsOpened") {
            m_dataChannelsOpened = member->ValueUint32();
        } else if (name == "dataChannelsClosed") {
            m_dataChannelsClosed = member->ValueUint32();
        }
    }
}

} // namespace Starfish

#endif
