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

#ifndef __StarfishRTCRtpEncodingParameters__
#define __StarfishRTCRtpEncodingParameters__

#include "core/modules/mediastream/RTCRtpCodingParameters.h"

#include "rtc_rtp_parameters.h"

namespace Starfish {

struct RTCRtpEncodingParameters : public RTCRtpCodingParameters {
    static RTCRtpEncodingParameters toRTCRtpEncodingParameters(
        libwebrtc::scoped_refptr<libwebrtc::RTCRtpEncodingParameters>
            libwebrtcRTCRtpEncodingParameters);

    RTCRtpEncodingParameters();

    // Define getter/setters
    DEFINE_GETTER_SETTER(bool, active, Active);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(uint32_t, maxBitrate, MaxBitrate);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, maxFramerate, MaxFramerate);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(double, scaleResolutionDownBy,
                                      ScaleResolutionDownBy);

    // Define memebers
    bool m_active = true;
    DEFINE_MEMBER_WITH_HASFLAG(uint32_t, maxBitrate, MaxBitrate);
    DEFINE_MEMBER_WITH_HASFLAG(double, maxFramerate, MaxFramerate);
    DEFINE_MEMBER_WITH_HASFLAG(double, scaleResolutionDownBy,
                               ScaleResolutionDownBy);
};

} // namespace Starfish

#endif
#endif
