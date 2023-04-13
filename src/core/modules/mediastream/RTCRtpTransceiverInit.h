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

#ifndef __StarfishRTCRtpTransceiverInit__
#define __StarfishRTCRtpTransceiverInit__

#include "core/modules/mediastream/RTCRtpEncodingParameters.h"

#include "rtc_rtp_transceiver.h"

namespace Starfish {

class MediaStream;

enum RTCRtpTransceiverDirection {
    Sendrecv,
    Sendonly,
    Recvonly,
    Inactive,
    Stopped
};

struct RTCRtpTransceiverInit {
    RTCRtpTransceiverInit();

    // Define getter/setters
    DEFINE_GETTER_SETTER(String*, direction, Direction);
    DEFINE_GETTER_SETTER(GCVector<MediaStream*>, streams, Streams);
    DEFINE_GETTER_SETTER(GCVector<RTCRtpEncodingParameters>, sendEncodings,
                         SendEncodings);

    libwebrtc::scoped_refptr<libwebrtc::RTCRtpTransceiverInit>
    toLibwebrtcRtpTransceiverInit(ExecutionContext* executionContext);

    libwebrtc::RTCRtpTransceiverDirection toLibwebrtcRTCRtpTransceiverDirection(
        ExecutionContext* executionContext);
    libwebrtc::vector<
        libwebrtc::scoped_refptr<libwebrtc::RTCRtpEncodingParameters>>
    toLibwebrtcSendEncondings(ExecutionContext* executionContext);
    libwebrtc::vector<libwebrtc::string> toLibwebrtcStreamIds(
        ExecutionContext* executionContext);

    // Define memebers
    String* m_direction = nullptr;
    GCVector<MediaStream*> m_streams;
    GCVector<RTCRtpEncodingParameters> m_sendEncodings;
};

} // namespace Starfish

#endif
#endif
