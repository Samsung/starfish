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

#ifndef __StarfishRTCConfiguration__
#define __StarfishRTCConfiguration__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

namespace Starfish {

enum class RTCIceTransportPolicy {
    Relay,
    All,
};

enum class RTCBundlePolicy {
    Balanced,
    MaxCompat,
    MaxBundle,
};

enum class RTCRtcpMuxPolicy {
    Negotiate,
    Require,
};

struct RTCConfiguration {
    friend class RTCPeerConnection;

public:
    RTCConfiguration();

    String* iceTransportPolicy();
    void setIceTransportPolicy(String* iceTransportPolicy);
    String* bundlePolicy();
    void setBundlePolicy(String* bundlePolicy);
    String* rtcpMuxPolicy();
    void setRtcpMuxPolicy(String* rtcpMuxPolicy);
    DEFINE_GETTER_SETTER(String*, peerIdentity, PeerIdentity);
    GCVector<RTCCertificate*>& certificates();
    void setCertificates(GCVector<RTCCertificate*>& certificates);

    bool isValid();

private:
    RTCIceTransportPolicy m_iceTransportPolicy{ RTCIceTransportPolicy::All };
    RTCBundlePolicy m_bundlePolicy{ RTCBundlePolicy::Balanced };
    RTCRtcpMuxPolicy m_rtcpMuxPolicy{ RTCRtcpMuxPolicy::Require };
    String* m_peerIdentity{ String::emptyString };
    GCVector<RTCCertificate*> m_certificates;

    bool m_hasValidIceTransportPolicy{ true };
    bool m_hasValidBundlePolicy{ true };
    bool m_hasValidRtcpMuxPolicy{ true };
};
}

#endif
