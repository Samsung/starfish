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

#include "core/modules/mediastream/RTCRtpEncodingParameters.h"

namespace Starfish {

RTCRtpEncodingParameters RTCRtpEncodingParameters::toRTCRtpEncodingParameters(
    libwebrtc::scoped_refptr<libwebrtc::RTCRtpEncodingParameters>
        libwebrtcRTCRtpEncodingParameters)
{
    RTCRtpEncodingParameters encoding;
    encoding.setActive(libwebrtcRTCRtpEncodingParameters->active());
    encoding.setMaxBitrate(
        libwebrtcRTCRtpEncodingParameters->max_bitrate_bps());
    encoding.setMaxFramerate(
        libwebrtcRTCRtpEncodingParameters->max_framerate());
    encoding.setScaleResolutionDownBy(
        libwebrtcRTCRtpEncodingParameters->scale_resolution_down_by());
    return encoding;
}

RTCRtpEncodingParameters::RTCRtpEncodingParameters()
    : m_active(true)
    , m_maxBitrate(0)
    , m_maxFramerate(0.0)
    , m_scaleResolutionDownBy(0.0)
{
}

} // namespace Starfish

#endif
