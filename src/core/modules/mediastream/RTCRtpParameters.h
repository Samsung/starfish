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

#ifndef __StarfishRTCRtpParameters__
#define __StarfishRTCRtpParameters__

#include "core/modules/mediastream/RTCRtpHeaderExtensionParameters.h"
#include "core/modules/mediastream/RTCRtcpParameters.h"
#include "core/modules/mediastream/RTCRtpCodecParameters.h"

#include "rtc_rtp_parameters.h"

namespace Starfish {

struct RTCRtpParameters {
    static RTCRtpParameters toRTCRtpParameters(
        libwebrtc::scoped_refptr<libwebrtc::RTCRtpParameters>
            libwebrtcRTCRtpParameters);

    RTCRtpParameters();

    // Define getter/setters
    DEFINE_GETTER_SETTER_WITH_HASFLAG(GCVector<RTCRtpHeaderExtensionParameters>,
                                      headerExtensions, HeaderExtensions);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(RTCRtcpParameters, rtcp, Rtcp);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(GCVector<RTCRtpCodecParameters>, codecs,
                                      Codecs);

    // Define memebers
    DEFINE_MEMBER_WITH_HASFLAG(GCVector<RTCRtpHeaderExtensionParameters>,
                               headerExtensions, HeaderExtensions);
    DEFINE_MEMBER_WITH_HASFLAG(RTCRtcpParameters, rtcp, Rtcp);
    DEFINE_MEMBER_WITH_HASFLAG(GCVector<RTCRtpCodecParameters>, codecs, Codecs);
};

} // namespace Starfish

#endif
#endif
