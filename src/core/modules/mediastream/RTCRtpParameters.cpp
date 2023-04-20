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

#include "core/modules/mediastream/RTCRtpParameters.h"

namespace Starfish {

RTCRtpParameters RTCRtpParameters::toRTCRtpParameters(
    libwebrtc::scoped_refptr<libwebrtc::RTCRtpParameters>
        libwebrtcRTCRtpParameters)
{
    RTCRtpParameters rtcRtpParameters;

    // headerExtensions
    GCVector<RTCRtpHeaderExtensionParameters> headerExtensions;
    for (auto& libwebrtcHeaderExtension :
         libwebrtcRTCRtpParameters->header_extensions().std_vector()) {
        RTCRtpHeaderExtensionParameters headerExtension =
            RTCRtpHeaderExtensionParameters::toRTCRtpHeaderExtensionParameters(
                libwebrtcHeaderExtension);
        headerExtensions.push_back(headerExtension);
    }

    if (headerExtensions.size()) {
        rtcRtpParameters.setHeaderExtensions(headerExtensions);
    }

    // rtcp
    libwebrtc::scoped_refptr<libwebrtc::RTCRtcpParameters>
        libwebrtcRtcpParameters = libwebrtcRTCRtpParameters->rtcp_parameters();
    if (libwebrtcRtcpParameters) {
        RTCRtcpParameters rtcpParameters =
            RTCRtcpParameters::toRTCRtcpParameters(libwebrtcRtcpParameters);
        rtcRtpParameters.setRtcp(rtcpParameters);
    }

    // codecs
    GCVector<RTCRtpCodecParameters> codecs;
    for (auto& libwebrtcRTCRtpCodecParameters :
         libwebrtcRTCRtpParameters->codecs().std_vector()) {
        RTCRtpCodecParameters rtcRtpCodecParameters =
            RTCRtpCodecParameters::toRTCRtpCodecParameters(
                libwebrtcRTCRtpCodecParameters);
        codecs.push_back(rtcRtpCodecParameters);
    }

    if (codecs.size()) {
        rtcRtpParameters.setCodecs(codecs);
    }
    return rtcRtpParameters;
}

RTCRtpParameters::RTCRtpParameters()
{
}

} // namespace Starfish

#endif
