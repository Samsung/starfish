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

#ifndef __StarfishRTCDtlsTransport__
#define __StarfishRTCDtlsTransport__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "api/peer_connection_interface.h"

namespace Starfish {

enum class RTCDtlsTransportState {
    New,
    Connecting,
    Connected,
    Closed,
    Failed,
};

class RTCDtlsTransport : public EventTarget {
public:
    RTCDtlsTransport(ExecutionContext* executionContext);
    RTCDtlsTransport(
        ExecutionContext* executionContext,
        rtc::scoped_refptr<webrtc::DtlsTransportInterface> rpcDtlsTransport);
    virtual ~RTCDtlsTransport();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(RTCDtlsTransport)
    ExecutionContext* executionContext() const;

    RTCIceTransport* iceTransport();
    String* stateStr();
    GCVector<ScriptArrayBuffer> getRemoteCertificates();

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(statechange);
    DECLARE_EVENT_LISTENER(error);
#undef VIRTUAL
#undef OVERRIDE

private:
    ExecutionContext* m_executionContext{ nullptr };
    rtc::scoped_refptr<webrtc::DtlsTransportInterface> m_backend;
    RTCIceTransport* m_iceTransport{ nullptr };
};
}
#endif
#endif
