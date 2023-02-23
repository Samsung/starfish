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

#include "core/modules/mediastream/RTCConfiguration.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/mediastream/RTCCertificate.h"

namespace Starfish {
RTCConfiguration::RTCConfiguration()
{
}

String* RTCConfiguration::iceTransportPolicy()
{
    if (!hasIceTransportPolicy()) {
        return String::createASCIIString("all");
    }

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
    m_hasIceTransportPolicy = true;
    m_hasValidIceTransportPolicy = true;

    if (iceTransportPolicy->equals("relay")) {
        m_iceTransportPolicy = RTCIceTransportPolicy::Relay;
    } else if (iceTransportPolicy->equals("all")) {
        m_iceTransportPolicy = RTCIceTransportPolicy::All;
    } else {
        m_hasValidIceTransportPolicy = false;
    }
}

String* RTCConfiguration::bundlePolicy()
{
    if (!hasBundlePolicy()) {
        return String::createASCIIString("balanced");
    }

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
    m_hasBundlePolicy = true;
    m_hasValidBundlePolicy = true;

    if (bundlePolicy->equals("balanced")) {
        m_bundlePolicy = RTCBundlePolicy::Balanced;
    } else if (bundlePolicy->equals("max-compat")) {
        m_bundlePolicy = RTCBundlePolicy::MaxCompat;
    } else if (bundlePolicy->equals("max-bundle")) {
        m_bundlePolicy = RTCBundlePolicy::MaxBundle;
    } else {
        m_hasValidBundlePolicy = false;
    }
}

String* RTCConfiguration::rtcpMuxPolicy()
{
    if (!hasRtcpMuxPolicy()) {
        return String::createASCIIString("require");
    }

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
    m_hasRtcpMuxPolicy = true;
    m_hasValidRtcpMuxPolicy = true;

    if (rtcpMuxPolicy->equals("negotiate")) {
        m_rtcpMuxPolicy = RTCRtcpMuxPolicy::Negotiate;
    } else if (rtcpMuxPolicy->equals("require")) {
        m_rtcpMuxPolicy = RTCRtcpMuxPolicy::Require;
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

libwebrtc::RTCConfiguration RTCConfiguration::genBackend()
{
    libwebrtc::RTCConfiguration config;
    libwebrtc::IceServer* iceServers = config.ice_servers;
    int i = 0;
    for (auto& iceServer : m_iceServers) {
        libwebrtc::IceServer& server = iceServers[i];
        if (iceServer.hasUsername()) {
            server.username = libwebrtc::string(
                iceServer.username()->toUTF8NonGCString().data());
        }

        if (iceServer.credential().isDOMStringValue()) {
            server.password = libwebrtc::string(iceServer.credential()
                                                    .getDOMStringValue()
                                                    ->toUTF8NonGCString()
                                                    .data());
        }

        for (auto url : iceServer.m_urls) {
            // server.urls.push_back(url->toUTF8NonGCString().data());
            server.uri = libwebrtc::string(url->toUTF8NonGCString().data());
        }
        i++;
    }

    if (m_iceTransportPolicy == RTCIceTransportPolicy::Relay) {
        config.type = libwebrtc::IceTransportsType::kRelay;
    } else if (m_iceTransportPolicy == RTCIceTransportPolicy::All) {
        config.type = libwebrtc::IceTransportsType::kAll;
    } else if (m_iceTransportPolicy == RTCIceTransportPolicy::NoHost) {
        config.type = libwebrtc::IceTransportsType::kNoHost;
    } else if (m_iceTransportPolicy == RTCIceTransportPolicy::None) {
        config.type = libwebrtc::IceTransportsType::kNone;
    }

    if (m_bundlePolicy == RTCBundlePolicy::Balanced) {
        config.bundle_policy = libwebrtc::BundlePolicy::kBundlePolicyBalanced;
    } else if (m_bundlePolicy == RTCBundlePolicy::MaxBundle) {
        config.bundle_policy = libwebrtc::BundlePolicy::kBundlePolicyMaxBundle;
    } else if (m_bundlePolicy == RTCBundlePolicy::MaxCompat) {
        config.bundle_policy = libwebrtc::BundlePolicy::kBundlePolicyMaxCompat;
    }

    if (m_rtcpMuxPolicy == RTCRtcpMuxPolicy::Negotiate) {
        config.rtcp_mux_policy =
            libwebrtc::RtcpMuxPolicy::kRtcpMuxPolicyNegotiate;
    } else if (m_rtcpMuxPolicy == RTCRtcpMuxPolicy::Require) {
        config.rtcp_mux_policy =
            libwebrtc::RtcpMuxPolicy::kRtcpMuxPolicyRequire;
    }

    config.ice_candidate_pool_size = m_iceCandidatePoolSize;
    return config;
}

} // namespace Starfish

#endif
