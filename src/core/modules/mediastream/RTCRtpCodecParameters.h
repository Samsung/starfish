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

#ifndef __StarfishRTCRtpCodecParameters__
#define __StarfishRTCRtpCodecParameters__

#include "core/modules/mediastream/RTCRtpCodec.h"

#include "rtc_rtp_parameters.h"

namespace Starfish {

struct RTCRtpCodecParameters : public RTCRtpCodec {
    static RTCRtpCodecParameters toRTCRtpCodecParameters(
        libwebrtc::scoped_refptr<libwebrtc::RTCRtpCodecParameters>
            libwebrtcRTCRtpCodecParameters);

    RTCRtpCodecParameters();

    // Define getter/setters
    DEFINE_GETTER_SETTER_WITH_HASFLAG(uint8_t, payloadType, PayloadType);

    // Define memebers
    DEFINE_MEMBER_WITH_HASFLAG(uint8_t, payloadType, PayloadType);
};

} // namespace Starfish

#endif
#endif
