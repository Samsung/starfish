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

#include "core/modules/mediastream/RTCConfiguration.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/mediastream/RTCCertificate.h"

namespace Starfish {
RTCConfiguration::RTCConfiguration()
{
}

GCVector<RTCCertificate*> RTCConfiguration::certificates()
{
    // FIXME: We need the current executionContext to create RTCCertificate.
    // Update the binding generator
    return m_certificates;
}

void RTCConfiguration::setCertificates(GCVector<RTCCertificate*>& certificates)
{
    m_certificates = certificates;
    m_backend.certificates.clear();
    for (auto certificate : certificates) {
        STARFISH_ASSERT(certificate->backend());
        m_backend.certificates.push_back(certificate->backend());
    }
}

std::vector<RTCIceServer> RTCConfiguration::iceServers()
{
    std::vector<RTCIceServer> iceServers;
    for (auto& is : m_backend.servers) {
        RTCIceServer iceServer(is);
        iceServers.push_back(iceServer);
    }
    return std::move(iceServers);
}
void RTCConfiguration::setIceServers(std::vector<RTCIceServer>& iceServers)
{
    m_backend.servers.clear();
    for (auto& iceServer : iceServers) {
        m_backend.servers.push_back(iceServer.backend());
    }
}

String* RTCConfiguration::iceTransportPolicy()
{
    switch (m_backend.type) {
    case webrtc::PeerConnectionInterface::IceTransportsType::kRelay:
        return String::createASCIIString("relay");
    case webrtc::PeerConnectionInterface::IceTransportsType::kAll:
        return String::createASCIIString("all");
    default:
        // NOTE: libwebrtc has more options that are not listed on the spec
        STARFISH_LOG_WARN("%s: Unsupported values\n", __func__);
        return String::emptyString;
    }
}

void RTCConfiguration::setIceTransportPolicy(String* iceTransportPolicy)
{
    if (iceTransportPolicy->equals("relay")) {
        m_backend.type =
            webrtc::PeerConnectionInterface::IceTransportsType::kRelay;
        m_hasValidIceTransportPolicy = true;
    } else if (iceTransportPolicy->equals("all")) {
        m_backend.type =
            webrtc::PeerConnectionInterface::IceTransportsType::kAll;
        m_hasValidIceTransportPolicy = true;
    } else {
        m_hasValidIceTransportPolicy = false;
    }
}

String* RTCConfiguration::bundlePolicy()
{
    switch (m_backend.bundle_policy) {
    case webrtc::PeerConnectionInterface::BundlePolicy::kBundlePolicyBalanced:
        return String::createASCIIString("balanced");
    case webrtc::PeerConnectionInterface::BundlePolicy::kBundlePolicyMaxCompat:
        return String::createASCIIString("max-compat");
    case webrtc::PeerConnectionInterface::BundlePolicy::kBundlePolicyMaxBundle:
        return String::createASCIIString("max-bundle");
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
}

void RTCConfiguration::setBundlePolicy(String* bundlePolicy)
{
    if (bundlePolicy->equals("balanced")) {
        m_backend.bundle_policy = webrtc::PeerConnectionInterface::
            BundlePolicy::kBundlePolicyBalanced;
        m_hasValidBundlePolicy = true;
    } else if (bundlePolicy->equals("max-compat")) {
        m_backend.bundle_policy = webrtc::PeerConnectionInterface::
            BundlePolicy::kBundlePolicyMaxCompat;
        m_hasValidBundlePolicy = true;
    } else if (bundlePolicy->equals("max-bundle")) {
        m_backend.bundle_policy = webrtc::PeerConnectionInterface::
            BundlePolicy::kBundlePolicyMaxBundle;
        m_hasValidBundlePolicy = true;
    } else {
        m_hasValidBundlePolicy = false;
    }
}

String* RTCConfiguration::rtcpMuxPolicy()
{
    switch (m_backend.rtcp_mux_policy) {
    case webrtc::PeerConnectionInterface::RtcpMuxPolicy::
        kRtcpMuxPolicyNegotiate:
        return String::createASCIIString("negotiate");
    case webrtc::PeerConnectionInterface::RtcpMuxPolicy::kRtcpMuxPolicyRequire:
        return String::createASCIIString("require");
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
}

void RTCConfiguration::setRtcpMuxPolicy(String* rtcpMuxPolicy)
{
    if (rtcpMuxPolicy->equals("negotiate")) {
        m_backend.rtcp_mux_policy = webrtc::PeerConnectionInterface::
            RtcpMuxPolicy::kRtcpMuxPolicyNegotiate;
        m_hasValidRtcpMuxPolicy = true;
    } else if (rtcpMuxPolicy->equals("require")) {
        m_backend.rtcp_mux_policy = webrtc::PeerConnectionInterface::
            RtcpMuxPolicy::kRtcpMuxPolicyRequire;
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

#endif
