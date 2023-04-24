/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishRTCRtpSender__
#define __StarfishRTCRtpSender__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "core/modules/mediastream/RTCRtpSendParameters.h"

#include "rtc_rtp_sender.h"

namespace Starfish {
class RTCRtpTransceiver;

class RTCRtpSender : public ScriptWrappable {
public:
    RTCRtpSender(ExecutionContext* executionContext,
                 RTCRtpTransceiver* transceiver,
                 libwebrtc::scoped_refptr<libwebrtc::RTCRtpSender> rtpSender);

    virtual ~RTCRtpSender();

    void dispose();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(RTCRtpSender)

    MediaStreamTrack* track();

    bool setTrack(MediaStreamTrack* track);

    RTCDtlsTransport* transport();

    Promise* setParameters(RTCRtpSendParameters parameters);

    RTCRtpSendParameters getParameters();

    Promise* replaceTrack(MediaStreamTrack* withTrack);

    void setStreams(GCVector<MediaStream*>& streams);

    libwebrtc::scoped_refptr<libwebrtc::RTCRtpSender> backend();

private:
    ExecutionContext* m_executionContext = nullptr;
    RTCRtpTransceiver* m_transceiver = nullptr;
    libwebrtc::scoped_refptr<libwebrtc::RTCRtpSender> m_backend;
    MediaStreamTrack* m_track = nullptr;
    libwebrtc::scoped_refptr<libwebrtc::RTCRtpParameters>
        m_lastReturnedLibwebrtcRTCRtpParameters;
};
} // namespace Starfish
#endif
#endif
