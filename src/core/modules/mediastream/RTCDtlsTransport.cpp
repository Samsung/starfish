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

#include "core/modules/mediastream/RTCDtlsTransport.h"

#include "core/dom/ExecutionContext.h"

#include "core/modules/mediastream/RTCIceTransport.h"
#include "api/dtls_transport_interface.h"

namespace Starfish {

RTCDtlsTransport::RTCDtlsTransport(ExecutionContext* executionContext)
    : RTCDtlsTransport(executionContext, nullptr)
{
}

RTCDtlsTransport::RTCDtlsTransport(
    ExecutionContext* executionContext,
    rtc::scoped_refptr<webrtc::DtlsTransportInterface> rtpDtlsTransport)
    : EventTarget()
    , m_executionContext(executionContext)
{
    m_backend = rtpDtlsTransport;

    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj,
                 void* cd) { ((RTCDtlsTransport*)obj)->~RTCDtlsTransport(); },
        NULL, NULL, NULL);
}

RTCDtlsTransport::~RTCDtlsTransport()
{
}

ScriptBindingInstance* RTCDtlsTransport::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

ExecutionContext* RTCDtlsTransport::executionContext() const
{
    return m_executionContext;
}

RTCIceTransport* RTCDtlsTransport::iceTransport()
{
    if (!m_backend) {
        return nullptr;
    }

    rtc::scoped_refptr<webrtc::IceTransportInterface> iceTransport =
        m_backend->ice_transport();
    if (!iceTransport) {
        return nullptr;
    }

    if (m_iceTransport) {
        m_iceTransport->setBackend(iceTransport);
    } else {
        m_iceTransport = new RTCIceTransport(executionContext(), iceTransport);
    }

    return m_iceTransport;
}

String* RTCDtlsTransport::stateStr()
{
    webrtc::DtlsTransportInformation info = m_backend->Information();
    switch (info.state()) {
    case webrtc::DtlsTransportState::kNew:
        return String::createASCIIString("new");
    case webrtc::DtlsTransportState::kConnecting:
        return String::createASCIIString("connecting");
    case webrtc::DtlsTransportState::kConnected:
        return String::createASCIIString("connected");
    case webrtc::DtlsTransportState::kClosed:
        return String::createASCIIString("closed");
    case webrtc::DtlsTransportState::kFailed:
        return String::createASCIIString("failed");
    default:
        return String::createASCIIString("failed");
    }
}

GCVector<ScriptArrayBuffer> RTCDtlsTransport::getRemoteCertificates()
{
    GCVector<ScriptArrayBuffer> buffer;
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return buffer;
}

DEFINE_EVENT_LISTENER(RTCDtlsTransport, statechange);
DEFINE_EVENT_LISTENER(RTCDtlsTransport, error);

} // namespace Starfish

#endif
