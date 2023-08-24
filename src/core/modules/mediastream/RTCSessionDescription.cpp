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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/mediastream/RTCSessionDescription.h"

#include "core/dom/ExecutionContext.h"
#include "rtc_peerconnection.h"

namespace Starfish {

RTCSessionDescriptionInit::RTCSessionDescriptionInit(
    Nullable<libwebrtc::RTCSessionDescription::SdpType> type, std::string sdp)
{
    if (type.hasValue()) {
        if (type.value() == libwebrtc::RTCSessionDescription::SdpType::kOffer) {
            m_type = RTCSdpType::Offer;
        } else if (type.value() ==
                   libwebrtc::RTCSessionDescription::SdpType::kPrAnswer) {
            m_type = RTCSdpType::Pranswer;
        } else if (type.value() ==
                   libwebrtc::RTCSessionDescription::SdpType::kAnswer) {
            m_type = RTCSdpType::Answer;
        }
    }

    m_sdp = String::createASCIIString(sdp.c_str(), sdp.length());
}

RTCSessionDescriptionInit::RTCSessionDescriptionInit(
    libwebrtc::RTCSessionDescription::SdpType type, std::string sdp)
{
    if (type == libwebrtc::RTCSessionDescription::SdpType::kOffer) {
        m_type = RTCSdpType::Offer;
    } else if (type == libwebrtc::RTCSessionDescription::SdpType::kPrAnswer) {
        m_type = RTCSdpType::Pranswer;
    } else if (type == libwebrtc::RTCSessionDescription::SdpType::kAnswer) {
        m_type = RTCSdpType::Answer;
    }
    m_sdp = String::createASCIIString(sdp.c_str(), sdp.length());
}

String* RTCSessionDescriptionInit::type() const
{
    if (!m_type.hasValue()) {
        return String::emptyString;
    }

    switch (m_type.value()) {
    case RTCSdpType::Offer:
        return String::createASCIIString("offer");
    case RTCSdpType::Pranswer:
        return String::createASCIIString("pranswer");
    case RTCSdpType::Answer:
        return String::createASCIIString("answer");
    case RTCSdpType::Rollback:
        return String::createASCIIString("rollback");
    default:
        return String::emptyString;
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
    } else if (type->equals("rollback")) {
        m_type = RTCSdpType::Rollback;
    }
}

Nullable<libwebrtc::RTCSessionDescription::SdpType>
RTCSessionDescriptionInit::toSdpType()
{
    if (!m_type.hasValue()) {
        return Nullable<libwebrtc::RTCSessionDescription::SdpType>();
    }

    switch (m_type.value()) {
    case RTCSdpType::Offer:
        return Nullable<libwebrtc::RTCSessionDescription::SdpType>(
            libwebrtc::RTCSessionDescription::SdpType::kOffer);
    case RTCSdpType::Pranswer:
        return Nullable<libwebrtc::RTCSessionDescription::SdpType>(
            libwebrtc::RTCSessionDescription::SdpType::kPrAnswer);
    case RTCSdpType::Answer:
        return Nullable<libwebrtc::RTCSessionDescription::SdpType>(
            libwebrtc::RTCSessionDescription::SdpType::kAnswer);
    case RTCSdpType::Rollback:
        return Nullable<libwebrtc::RTCSessionDescription::SdpType>();
    default:
        return Nullable<libwebrtc::RTCSessionDescription::SdpType>();
    }
}

RTCSessionDescription::RTCSessionDescription(
    ExecutionContext* executionContext,
    libwebrtc::RTCSessionDescription* backend)
    : EventTarget()
    , m_executionContext(executionContext)
{
    // NOTE: backend is owned by native peerconnection

    RTCSessionDescriptionInit init;

    std::string type = backend->type().std_string();
    std::string sdp = backend->sdp().std_string();

    init.setType(String::createASCIIString(type.data(), type.length()));
    m_type = init.m_type;
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
    RTCSessionDescriptionInit init(m_type, m_sdp);
    return init.type();
}
} // namespace Starfish

#endif
