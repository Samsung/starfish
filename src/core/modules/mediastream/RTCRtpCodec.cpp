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

#include "core/modules/mediastream/RTCRtpCodec.h"

namespace Starfish {

RTCRtpCodec RTCRtpCodec::toRTCRtpCodec(
    libwebrtc::scoped_refptr<libwebrtc::RTCRtpCodecParameters>
        libwebrtcRTCRtpCodecParameters)
{
    RTCRtpCodec rtcRtpCodec;

    libwebrtc::string libwebrtcMimeType =
        libwebrtcRTCRtpCodecParameters->mime_type();
    if (libwebrtcMimeType.size()) {
        rtcRtpCodec.setMimeType(String::createASCIIString(
            libwebrtcMimeType.c_string(), libwebrtcMimeType.size()));
    }

    rtcRtpCodec.setClockRate(libwebrtcRTCRtpCodecParameters->clock_rate());
    rtcRtpCodec.setChannels(libwebrtcRTCRtpCodecParameters->num_channels());

    libwebrtc::vector<std::pair<libwebrtc::string, libwebrtc::string>>
        libwebrtcRTCRtpCodecParametersParameters =
            libwebrtcRTCRtpCodecParameters->parameters();
    if (libwebrtcRTCRtpCodecParametersParameters.size()) {
        StringBuilder parametersBuilder;
        for (auto pair :
             libwebrtcRTCRtpCodecParametersParameters.std_vector()) {
            StringBuilder formatParameterBuilder;

            libwebrtc::string key = pair.first;
            if (key.size()) {
                formatParameterBuilder.appendString(
                    String::createASCIIString(key.c_string(), key.size()));
                formatParameterBuilder.appendChar('=');
            }

            libwebrtc::string value = pair.second;
            if (value.size()) {
                formatParameterBuilder.appendString(
                    String::createASCIIString(value.c_string(), value.size()));
            }

            if (parametersBuilder.length()) {
                parametersBuilder.appendChar(';');
            }

            if (formatParameterBuilder.length()) {
                parametersBuilder.appendString(
                    formatParameterBuilder.finalize());
            }
        }
        if (parametersBuilder.length()) {
            rtcRtpCodec.setSdpFmtpLine(parametersBuilder.finalize());
        }
    }

    return rtcRtpCodec;
}

RTCRtpCodec::RTCRtpCodec()
    : m_mimeType(String::emptyString)
    , m_clockRate(0)
    , m_channels(0)
    , m_sdpFmtpLine(String::emptyString)
{
}

} // namespace Starfish

#endif
