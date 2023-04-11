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

#include "core/modules/mediastream/RTCRtpTransceiverInit.h"

#include "core/dom/DOMException.h"

namespace Starfish {

RTCRtpTransceiverInit::RTCRtpTransceiverInit()
    : m_direction(String::createASCIIString("sendrecv"))
{
}

libwebrtc::scoped_refptr<libwebrtc::RTCRtpTransceiverInit>
RTCRtpTransceiverInit::toLibwebrtcRtpTransceiverInit(
    ExecutionContext* executionContext)
{
    libwebrtc::RTCRtpTransceiverDirection direction =
        toLibwebrtcRTCRtpTransceiverDirection(executionContext);
    libwebrtc::vector<
        libwebrtc::scoped_refptr<libwebrtc::RTCRtpEncodingParameters>>
        encodings = toLibwebrtcSendEncondings(executionContext);
    libwebrtc::vector<libwebrtc::string>
        streamIds; // TODO: sequence<MediaStream> streams

    libwebrtc::scoped_refptr<libwebrtc::RTCRtpTransceiverInit> init =
        libwebrtc::RTCRtpTransceiverInit::Create(direction, streamIds,
                                                 encodings);
    return init;
}

libwebrtc::RTCRtpTransceiverDirection
RTCRtpTransceiverInit::toLibwebrtcRTCRtpTransceiverDirection(
    ExecutionContext* executionContext)
{
    if (m_direction->equals("sendrecv")) {
        return libwebrtc::RTCRtpTransceiverDirection::kSendRecv;
    } else if (m_direction->equals("sendonly")) {
        return libwebrtc::RTCRtpTransceiverDirection::kSendOnly;
    } else if (m_direction->equals("recvonly")) {
        return libwebrtc::RTCRtpTransceiverDirection::kRecvOnly;
    } else if (m_direction->equals("inactive")) {
        return libwebrtc::RTCRtpTransceiverDirection::kInactive;
    } else {
        throw new DOMException(executionContext, DOMException::SCRIPT_TYPE_ERR,
                               "RTCRtpTransceiverInit has invalid direction.");
    }
    return libwebrtc::RTCRtpTransceiverDirection::kInactive;
}

libwebrtc::vector<libwebrtc::scoped_refptr<libwebrtc::RTCRtpEncodingParameters>>
RTCRtpTransceiverInit::toLibwebrtcSendEncondings(
    ExecutionContext* executionContext)
{
    std::vector<libwebrtc::scoped_refptr<libwebrtc::RTCRtpEncodingParameters>>
        encodings;
    auto rtpEncodingParameter = libwebrtc::RTCRtpEncodingParameters::Create();

    for (auto& sendEncoding : m_sendEncodings) {
        if (sendEncoding.hasRid()) {
            String* rid = sendEncoding.rid();
            bool invalid = false;
            if (rid->length() > 16) {
                invalid = true;
            }

            auto bufferAcessData = rid->bufferAccessData();
            for (size_t i = 0; i < bufferAcessData.length; i++) {
                const char32_t c = bufferAcessData.charAt(i);
                if (!isdigit(c) && !isalpha(c)) {
                    invalid = true;
                }
            }

            if (invalid) {
                throw new DOMException(
                    executionContext, DOMException::SCRIPT_TYPE_ERR,
                    "RTCRtpTransceiverInit has invalid direction.");
            }

            rtpEncodingParameter->set_rid(
                sendEncoding.rid()->toUTF8NonGCString().data());
        }

        rtpEncodingParameter->set_active(sendEncoding.active());

        if (sendEncoding.hasMaxBitrate()) {
            rtpEncodingParameter->set_max_bitrate_bps(
                sendEncoding.maxBitrate());
        }

        if (sendEncoding.hasMaxFramerate()) {
            rtpEncodingParameter->set_max_framerate(
                sendEncoding.maxFramerate());
        }

        if (sendEncoding.hasScaleResolutionDownBy()) {
            rtpEncodingParameter->set_scale_resolution_down_by(
                sendEncoding.scaleResolutionDownBy());
        }

        encodings.push_back(rtpEncodingParameter);
    }
    return encodings;
}

} // namespace Starfish
#endif
