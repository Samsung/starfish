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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/mediastream/RTCSctpTransport.h"

#include "core/modules/mediastream/RTCDtlsTransport.h"
#include "core/dom/ExecutionContext.h"

#include "api/sctp_transport_interface.h"

namespace Starfish {

RTCSctpTransport::RTCSctpTransport(ExecutionContext* executionContext)
    : RTCSctpTransport(executionContext, nullptr)
{
}

RTCSctpTransport::RTCSctpTransport(
    ExecutionContext* executionContext,
    rtc::scoped_refptr<webrtc::SctpTransportInterface> sctpTransport)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_backend(sctpTransport)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj,
                 void* cd) { ((RTCSctpTransport*)obj)->~RTCSctpTransport(); },
        NULL, NULL, NULL);
}

RTCSctpTransport::~RTCSctpTransport()
{
}

ScriptBindingInstance* RTCSctpTransport::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

ExecutionContext* RTCSctpTransport::executionContext() const
{
    return m_executionContext;
}

RTCDtlsTransport* RTCSctpTransport::transport()
{
    if (!m_backend) {
        return nullptr;
    }

    rtc::scoped_refptr<webrtc::DtlsTransportInterface> transport =
        m_backend->dtls_transport();

    if (!transport) {
        return nullptr;
    }

    return new RTCDtlsTransport(executionContext(), transport);
}

RTCSctpTransportState RTCSctpTransport::state()
{
    if (!m_backend) {
        return RTCSctpTransportState::Closed;
    }

    webrtc::SctpTransportInformation info = m_backend->Information();
    switch (info.state()) {
    case webrtc::SctpTransportState::kConnecting:
        return RTCSctpTransportState::Connecting;
    case webrtc::SctpTransportState::kConnected:
        return RTCSctpTransportState::Connected;
    case webrtc::SctpTransportState::kClosed:
        return RTCSctpTransportState::Closed;
    default:
        // WebRTC has more internal states
        return RTCSctpTransportState::Closed;
    }
}

String* RTCSctpTransport::stateStr()
{
    if (!m_backend) {
        return String::createASCIIString("closed");
    }

    webrtc::SctpTransportInformation info = m_backend->Information();
    switch (info.state()) {
    case webrtc::SctpTransportState::kConnecting:
        return String::createASCIIString("connecting");
    case webrtc::SctpTransportState::kConnected:
        return String::createASCIIString("connected");
    case webrtc::SctpTransportState::kClosed:
        return String::createASCIIString("closed");
    default:
        // WebRTC has more internal states
        return String::createASCIIString("closed");
    }
}

double RTCSctpTransport::maxMessageSize()
{
    if (!m_backend) {
        return 0;
    }

    webrtc::SctpTransportInformation info = m_backend->Information();
    absl::optional<double> size = info.MaxMessageSize();
    if (!size.has_value()) {
        return 0;
    }

    return size.value();
}

Nullable<uint32_t> RTCSctpTransport::maxChannels()
{
    Nullable<uint32_t> result;
    if (!m_backend) {
        return result;
    }

    webrtc::SctpTransportInformation info = m_backend->Information();
    absl::optional<int> channels = info.MaxChannels();
    if (!channels.has_value()) {
        return result;
    }

    result = channels.value();
    return result;
}

DEFINE_EVENT_LISTENER(RTCSctpTransport, statechange);

} // namespace Starfish

#endif
