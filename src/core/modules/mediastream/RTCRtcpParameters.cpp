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

#include "core/modules/mediastream/RTCRtcpParameters.h"

namespace Starfish {

RTCRtcpParameters RTCRtcpParameters::toRTCRtcpParameters(
    libwebrtc::scoped_refptr<libwebrtc::RTCRtcpParameters>
        libwebrtcRTCRtcpParameters)
{
    RTCRtcpParameters rtcpParameters;
    libwebrtc::string libwebrtcCname = libwebrtcRTCRtcpParameters->cname();
    if (libwebrtcCname.size()) {
        rtcpParameters.setCname(String::createASCIIString(
            libwebrtcCname.c_string(), libwebrtcCname.size()));
    }
    rtcpParameters.setReducedSize(libwebrtcRTCRtcpParameters->reduced_size());

    return rtcpParameters;
}

RTCRtcpParameters::RTCRtcpParameters()
    : m_cname(String::emptyString)
    , m_reducedSize(false)
{
}

} // namespace Starfish

#endif
