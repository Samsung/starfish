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

#include "core/modules/mediastream/RTCSessionDescription.h"

#include "core/dom/ExecutionContext.h"
#include "api/peer_connection_interface.h"

namespace Starfish {

String* RTCSessionDescriptionInit::type()
{
    switch (m_type) {
    case RTCSdpType::Offer:
        return String::createASCIIString("offer");
    case RTCSdpType::Pranswer:
        return String::createASCIIString("pranswer");
    case RTCSdpType::Answer:
        return String::createASCIIString("answer");
    // FIXME: libwebrtc does not support rollback
    // case RTCSdpType::Rollback:
    //     return String::createASCIIString("rollback");
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
}

void RTCSessionDescriptionInit::setType(String* type)
{
    if (type->equals("offer")) {
        m_type = RTCSdpType::Offer;
    } else if (type->equals("pranswer")) {
        m_type = RTCSdpType::Pranswer;
    } else if (type->equals("answer")) {
        m_type = RTCSdpType::Answer;
    }
    // FIXME: libwebrtc does not support rollback
    // else if (type->equals("rollback")) {
    //     m_type = RTCSdpType::Rollback;
    // }
}

RTCSessionDescription::RTCSessionDescription(
    ExecutionContext* executionContext,
    const webrtc::SessionDescriptionInterface* backend)
    : EventTarget()
    , m_executionContext(executionContext)
{
    // NOTE: backend is owned by native peerconnection
    RTCSessionDescriptionInit init;
    init.setType(String::createASCIIString(backend->type().data(),
                                           backend->type().length()));
    m_type = init.m_type;

    std::string sdp;
    backend->ToString(&sdp);
    m_sdp = String::createASCIIString(sdp.data(), sdp.length());
}

RTCSessionDescription::RTCSessionDescription(
    ExecutionContext* executionContext,
    RTCSessionDescriptionInit& sessionDescriptionInit)
    : EventTarget()
    , m_executionContext(executionContext)
{
    m_type = sessionDescriptionInit.m_type;
    m_sdp = sessionDescriptionInit.m_sdp;
}

RTCSessionDescription::~RTCSessionDescription()
{
}

ScriptBindingInstance* RTCSessionDescription::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

ExecutionContext* RTCSessionDescription::executionContext() const
{
    return m_executionContext;
}

String* RTCSessionDescription::type()
{
    switch (m_type) {
    case RTCSdpType::Offer:
        return String::createASCIIString("offer");
    case RTCSdpType::Pranswer:
        return String::createASCIIString("pranswer");
    case RTCSdpType::Answer:
        return String::createASCIIString("answer");
    // FIXME: libwebrtc does not support rollback
    // case RTCSdpType::Rollback:
    //     return String::createASCIIString("rollback");
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
}
}

#endif
