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

// https://w3c.github.io/webrtc-pc/#dfn-setparameters-validation-steps
bool RTCRtpSendParameters::validRTCRtpSendParameters(
    const RTCRtpSendParameters& oldRTCRtpSendParameters,
    const RTCRtpSendParameters& newRTCRtpSendParameters)
{
    // RTCRtpSendParameters
    // transactionId is read only.
    if (!oldRTCRtpSendParameters.transactionId()->equals(
            newRTCRtpSendParameters.transactionId())) {
        return false;
    }

    // RTCRtpCodingParameters
    // rid is read only
    const GCVector<RTCRtpEncodingParameters>& oldEcodings =
        oldRTCRtpSendParameters.encodings();
    const GCVector<RTCRtpEncodingParameters>& newEcodings =
        newRTCRtpSendParameters.encodings();

    if (oldEcodings.size() != newEcodings.size()) {
        return false;
    }

    for (int i = 0; i < newEcodings.size(); i++) {
        if (!oldEcodings[i].rid()->equals(newEcodings[i].rid())) {
            return false;
        }
    }

    // RTCRtpParameters
    //   headerExtensions, rtcp, codecs are read only.
    const GCVector<RTCRtpHeaderExtensionParameters>& oldHeaderExtensions =
        oldRTCRtpSendParameters.headerExtensions();
    const GCVector<RTCRtpHeaderExtensionParameters>& newHeaderExtensions =
        newRTCRtpSendParameters.headerExtensions();

    if (oldHeaderExtensions.size() != newHeaderExtensions.size()) {
        return false;
    }

    // RTCRtpHeaderExtensionParameters
    // uri, id, encrypted are read only.
    for (int i = 0; i < newHeaderExtensions.size(); i++) {
        const auto& oldHeaderExtension = oldHeaderExtensions[i];
        const auto& newHeaderExtension = newHeaderExtensions[i];
        if (!oldHeaderExtension.uri()->equals(newHeaderExtension.uri())) {
            return false;
        }
        if (oldHeaderExtension.id() != newHeaderExtension.id()) {
            return false;
        }
        if (oldHeaderExtension.encrypted() != newHeaderExtension.encrypted()) {
            return false;
        }
    }

    // RTCRtcpParameters
    // cname, reducedSize is read only.
    if (!oldRTCRtpSendParameters.rtcp().cname()->equals(
            newRTCRtpSendParameters.rtcp().cname())) {
        return false;
    }

    if (oldRTCRtpSendParameters.rtcp().reducedSize() !=
        newRTCRtpSendParameters.rtcp().reducedSize()) {
        return false;
    }

    const GCVector<RTCRtpCodecParameters>& oldCodecs =
        oldRTCRtpSendParameters.codecs();
    const GCVector<RTCRtpCodecParameters>& newCodecs =
        newRTCRtpSendParameters.codecs();
    if (oldCodecs.size() != newCodecs.size()) {
        return false;
    }

    for (size_t i = 0; i < newCodecs.size(); i++) {
        const auto& oldCodec = oldCodecs[i];
        const auto& newCodec = newCodecs[i];
        // RTCRtpCodecParameters
        // payloadType is read only.
        if (oldCodec.payloadType() != newCodec.payloadType()) {
            return false;
        }

        // These properties are not marked as read-only in the specification.
        // However, all other major browsers treat them as read-only.
        if (!oldCodec.mimeType()->equals(newCodec.mimeType())) {
            return false;
        }
        if (oldCodec.clockRate() != newCodec.clockRate()) {
            return false;
        }
        if (oldCodec.channels() != newCodec.channels()) {
            return false;
        }
        if (!oldCodec.sdpFmtpLine()->equals(newCodec.sdpFmtpLine())) {
            return false;
        }
    }
    return true;
}

RTCRtpSendParameters::RTCRtpSendParameters()
{
}

} // namespace Starfish

#endif
