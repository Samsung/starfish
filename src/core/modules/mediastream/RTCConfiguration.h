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

#ifndef __StarfishRTCConfiguration__
#define __StarfishRTCConfiguration__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "core/modules/mediastream/RTCIceServer.h"

#include "rtc_types.h"

namespace Starfish {
class RTCCertificate;

enum class RTCIceTransportPolicy { Relay, All, NoHost, None };

enum class RTCBundlePolicy {
    Balanced,
    MaxCompat,
    MaxBundle,
};

enum class RTCRtcpMuxPolicy {
    Negotiate,
    Require,
};

struct RTCConfiguration : public gc {
    friend class RTCPeerConnection;

public:
    RTCConfiguration();
    virtual ~RTCConfiguration(){};

    DEFINE_GETTER_SETTER_WITH_HASFLAG(GCVector<RTCIceServer>, iceServers,
                                      IceServers);

    String* iceTransportPolicy();
    void setIceTransportPolicy(String* iceTransportPolicy);
    DEFINE_HASFLAG_GETTER(IceTransportPolicy);

    String* bundlePolicy();
    void setBundlePolicy(String* bundlePolicy);
    DEFINE_HASFLAG_GETTER(BundlePolicy);

    String* rtcpMuxPolicy();
    void setRtcpMuxPolicy(String* rtcpMuxPolicy);
    DEFINE_HASFLAG_GETTER(RtcpMuxPolicy);

    DEFINE_GETTER_SETTER(String*, peerIdentity, PeerIdentity);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(GCVector<RTCCertificate*>, certificates,
                                      Certificates);
    DEFINE_GETTER_SETTER(uint8_t, iceCandidatePoolSize, IceCandidatePoolSize);

    bool isValid();

    libwebrtc::RTCConfiguration genBackend();

private:
    GCVector<RTCIceServer> m_iceServers;
    RTCIceTransportPolicy m_iceTransportPolicy{ RTCIceTransportPolicy::All };
    RTCBundlePolicy m_bundlePolicy{ RTCBundlePolicy::Balanced };
    RTCRtcpMuxPolicy m_rtcpMuxPolicy{ RTCRtcpMuxPolicy::Require };
    String* m_peerIdentity{ String::emptyString };
    GCVector<RTCCertificate*> m_certificates;
    uint8_t m_iceCandidatePoolSize{ 0 };

    bool m_hasIceServers{ true };
    bool m_hasIceTransportPolicy{ false };
    bool m_hasBundlePolicy{ false };
    bool m_hasRtcpMuxPolicy{ false };
    bool m_hasCertificates{ true };

    bool m_hasValidIceTransportPolicy{ true };
    bool m_hasValidBundlePolicy{ true };
    bool m_hasValidRtcpMuxPolicy{ true };
};
} // namespace Starfish

#endif
#endif
