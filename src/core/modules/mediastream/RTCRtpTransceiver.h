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

#ifndef __StarfishRTCRtpTransceiver__
#define __StarfishRTCRtpTransceiver__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "api/peer_connection_interface.h"

namespace Starfish {

class RTCRtpTransceiver : public ScriptWrappable {
public:
    RTCRtpTransceiver(ExecutionContext* executionContext);
    RTCRtpTransceiver(
        ExecutionContext* executionContext,
        rtc::scoped_refptr<webrtc::RtpTransceiverInterface> rptTransceiver);
    virtual ~RTCRtpTransceiver();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(RTCRtpTransceiver)

private:
    ExecutionContext* m_executionContext{ nullptr };
};
}
#endif
#endif
