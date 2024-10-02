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

#ifndef __StarfishRTCSctpTransport__
#define __StarfishRTCSctpTransport__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

namespace Starfish {

enum class RTCSctpTransportState {
    Connecting,
    Connected,
    Closed,
};

class RTCSctpTransport : public EventTarget {
public:
    RTCSctpTransport(ExecutionContext* executionContext);
    // RTCSctpTransport(
    //     ExecutionContext* executionContext,
    //     libwebrtc::scoped_refptr<webrtc::SctpTransportInterface>
    //     rpcSctpTransport);
    virtual ~RTCSctpTransport();
    virtual ExecutionContext* executionContext() const;
    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(RTCSctpTransport)

    RTCDtlsTransport* transport();
    RTCSctpTransportState state();
    String* stateStr();

    double maxMessageSize();
    Optional<uint32_t> maxChannels();

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(statechange);
#undef VIRTUAL
#undef OVERRIDE

private:
    ExecutionContext* m_executionContext{ nullptr };
    // libwebrtc::scoped_refptr<webrtc::SctpTransportInterface> m_backend;
};
} // namespace Starfish
#endif
#endif
