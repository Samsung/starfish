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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/mediastream/RTCConfiguration.h"

namespace Starfish {
RTCConfiguration::RTCConfiguration()
{
}

GCVector<RTCCertificate*>& RTCConfiguration::certificates()
{
    return m_certificates;
}

void RTCConfiguration::setCertificates(GCVector<RTCCertificate*>& certificates)
{
    m_certificates = certificates;
}

String* RTCConfiguration::iceTransportPolicy()
{
    switch (m_iceTransportPolicy) {
    case RTCIceTransportPolicy::Relay:
        return String::createASCIIString("relay");
    case RTCIceTransportPolicy::All:
        return String::createASCIIString("all");
    default:
        return String::emptyString;
    }
}

void RTCConfiguration::setIceTransportPolicy(String* iceTransportPolicy)
{
    if (iceTransportPolicy->equalsIgnoreCase("relay")) {
        m_iceTransportPolicy = RTCIceTransportPolicy::Relay;
        m_hasValidIceTransportPolicy = true;
    } else if (iceTransportPolicy->equalsIgnoreCase("all")) {
        m_iceTransportPolicy = RTCIceTransportPolicy::All;
        m_hasValidIceTransportPolicy = true;
    } else {
        m_hasValidIceTransportPolicy = false;
    }
}

String* RTCConfiguration::bundlePolicy()
{
    switch (m_bundlePolicy) {
    case RTCBundlePolicy::Balanced:
        return String::createASCIIString("balanced");
    case RTCBundlePolicy::MaxCompat:
        return String::createASCIIString("max-compat");
    case RTCBundlePolicy::MaxBundle:
        return String::createASCIIString("max-bundle");
    default:
        return String::emptyString;
    }
}

void RTCConfiguration::setBundlePolicy(String* bundlePolicy)
{
    if (bundlePolicy->equalsIgnoreCase("balanced")) {
        m_bundlePolicy = RTCBundlePolicy::Balanced;
        m_hasValidBundlePolicy = true;
    } else if (bundlePolicy->equalsIgnoreCase("max-compat")) {
        m_bundlePolicy = RTCBundlePolicy::MaxCompat;
        m_hasValidBundlePolicy = true;
    } else if (bundlePolicy->equalsIgnoreCase("max-bundle")) {
        m_bundlePolicy = RTCBundlePolicy::MaxBundle;
        m_hasValidBundlePolicy = true;
    } else {
        m_hasValidBundlePolicy = false;
    }
}

String* RTCConfiguration::rtcpMuxPolicy()
{
    switch (m_rtcpMuxPolicy) {
    case RTCRtcpMuxPolicy::Negotiate:
        return String::createASCIIString("negotiate");
    case RTCRtcpMuxPolicy::Require:
        return String::createASCIIString("require");
    default:
        return String::emptyString;
    }
}

void RTCConfiguration::setRtcpMuxPolicy(String* rtcpMuxPolicy)
{
    if (rtcpMuxPolicy->equalsIgnoreCase("negotiate")) {
        m_rtcpMuxPolicy = RTCRtcpMuxPolicy::Negotiate;
        m_hasValidRtcpMuxPolicy = true;
    } else if (rtcpMuxPolicy->equalsIgnoreCase("require")) {
        m_rtcpMuxPolicy = RTCRtcpMuxPolicy::Require;
        m_hasValidRtcpMuxPolicy = true;
    } else {
        m_hasValidRtcpMuxPolicy = false;
    }
}

bool RTCConfiguration::isValid()
{
    if (m_hasValidIceTransportPolicy && m_hasValidBundlePolicy &&
        m_hasValidRtcpMuxPolicy) {
        return true;
    }
    return false;
}
}
