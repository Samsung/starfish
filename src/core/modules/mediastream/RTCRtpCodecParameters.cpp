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

#include "core/modules/mediastream/RTCRtpCodecParameters.h"

namespace Starfish {

RTCRtpCodecParameters RTCRtpCodecParameters::toRTCRtpCodecParameters(
    libwebrtc::scoped_refptr<libwebrtc::RTCRtpCodecParameters>
        libwebrtcRTCRtpCodecParameters)
{
    RTCRtpCodec rtcRtpCodec =
        RTCRtpCodec::toRTCRtpCodec(libwebrtcRTCRtpCodecParameters);

    RTCRtpCodecParameters rtcRtpCodecParameters;
    if (rtcRtpCodec.hasMimeType()) {
        rtcRtpCodecParameters.setMimeType(rtcRtpCodec.mimeType());
    }

    if (rtcRtpCodec.hasClockRate()) {
        rtcRtpCodecParameters.setClockRate(rtcRtpCodec.clockRate());
    }

    if (rtcRtpCodec.hasChannels()) {
        rtcRtpCodecParameters.setChannels(rtcRtpCodec.channels());
    }

    if (rtcRtpCodec.hasSdpFmtpLine()) {
        rtcRtpCodecParameters.setSdpFmtpLine(rtcRtpCodec.sdpFmtpLine());
    }

    rtcRtpCodecParameters.setPayloadType(
        libwebrtcRTCRtpCodecParameters->payload_type());

    return rtcRtpCodecParameters;
}

RTCRtpCodecParameters::RTCRtpCodecParameters()
    : m_payloadType(0)
{
}

} // namespace Starfish

#endif
