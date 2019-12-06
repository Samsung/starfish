/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#include "api/peer_connection_interface.h"

namespace Starfish {

struct RTCRtpParameters {
};

struct RTCRtpSendParameters : public RTCRtpParameters {
    DEFINE_GETTER_SETTER(String*, transactionId, TransactionId);

    String* m_transactionId{ String::emptyString };
};

class RTCRtpSender : public ScriptWrappable {
public:
    RTCRtpSender(ExecutionContext* executionContext);
    RTCRtpSender(ExecutionContext* executionContext,
                 rtc::scoped_refptr<webrtc::RtpSenderInterface> rtpSender);
    virtual ~RTCRtpSender();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(RTCRtpSender)

    MediaStreamTrack* track();
    void setTrack(MediaStreamTrack* track);
    RTCDtlsTransport* transport();
    Promise* setParameters(RTCRtpSendParameters parameters);
    RTCRtpSendParameters getParameters();
    Promise* replaceTrack(MediaStreamTrack* withTrack);
    void setStreams(GCVector<MediaStream*>& streams);

    rtc::scoped_refptr<webrtc::RtpSenderInterface> backend();

private:
    ExecutionContext* m_executionContext{ nullptr };
    rtc::scoped_refptr<webrtc::RtpSenderInterface> m_backend;

    MediaStreamTrack* m_track{ nullptr };
};
}
#endif
#endif
