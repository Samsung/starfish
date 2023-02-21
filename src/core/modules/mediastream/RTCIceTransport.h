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

#ifndef __StarfishRTCIceTransport__
#define __StarfishRTCIceTransport__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "core/modules/mediastream/RTCIceCandidate.h"

// #include "rtc_ice_transport.h"

namespace Starfish {

enum RTCIceTransportState {
    New,
    Checking,
    Connected,
    Completed,
    Disconnected,
    Failed,
    Closed,
};

class RTCIceTransport : public EventTarget {
public:
    RTCIceTransport(ExecutionContext* executionContext);
    // RTCIceTransport(ExecutionContext* executionContext,
    //                 libwebrtc::scoped_refptr<libwebrtc::IceTransport>
    //                 backend);

    virtual ~RTCIceTransport();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(RTCIceTransport)
    virtual ExecutionContext* executionContext() const override;

    // String* state();
    GCVector<RTCIceCandidate*> getRemoteCandidates();
    Nullable<RTCIceCandidatePair> getSelectedCandidatePair();

    // void setBackend(libwebrtc::scoped_refptr<libwebrtc::IceTransport>
    // backend);

private:
    ExecutionContext* m_executionContext{ nullptr };
    // libwebrtc::scoped_refptr<libwebrtc::IceTransport> m_backend;
};
} // namespace Starfish
#endif
#endif
