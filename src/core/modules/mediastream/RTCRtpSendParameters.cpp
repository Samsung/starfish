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

#include "core/modules/mediastream/RTCRtpSendParameters.h"

namespace Starfish {

RTCRtpSendParameters RTCRtpSendParameters::toRTCRtpSendParameters(
    libwebrtc::scoped_refptr<libwebrtc::RTCRtpParameters>
        libwebrtcRTCRtpParameters)
{
    RTCRtpSendParameters rtcRtpSendParameters;

    // RTCRtpParameters
    RTCRtpParameters rtcRtpParameters =
        RTCRtpParameters::toRTCRtpParameters(libwebrtcRTCRtpParameters);

    if (rtcRtpParameters.hasHeaderExtensions()) {
        rtcRtpSendParameters.setHeaderExtensions(
            rtcRtpParameters.headerExtensions());
    }

    if (rtcRtpParameters.hasRtcp()) {
        rtcRtpSendParameters.setCodecs(rtcRtpParameters.codecs());
    }

    if (rtcRtpParameters.hasCodecs()) {
        rtcRtpSendParameters.setCodecs(rtcRtpParameters.codecs());
    }

    // transactionId
    libwebrtc::string libwebrtcTransactionId =
        libwebrtcRTCRtpParameters->transaction_id();
    if (libwebrtcTransactionId.size()) {
        rtcRtpSendParameters.setTransactionId(String::createASCIIString(
            libwebrtcTransactionId.c_string(), libwebrtcTransactionId.size()));
    }

    // encodings
    GCVector<RTCRtpEncodingParameters> encodings;
    for (auto& libwebrtcEncoding :
         libwebrtcRTCRtpParameters->encodings().std_vector()) {
        RTCRtpEncodingParameters encoding =
            RTCRtpEncodingParameters::toRTCRtpEncodingParameters(
                libwebrtcEncoding);
        encodings.push_back(encoding);
    }

    if (encodings.size()) {
        rtcRtpSendParameters.setEncodings(encodings);
    }

    return rtcRtpSendParameters;
}

RTCRtpSendParameters::RTCRtpSendParameters()
{
}

} // namespace Starfish

#endif
