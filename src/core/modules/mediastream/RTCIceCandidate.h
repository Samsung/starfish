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

#ifndef __StarfishRTCIceCandidate__
#define __StarfishRTCIceCandidate__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "api/peer_connection_interface.h"

namespace Starfish {
class ExecutionContext;

struct RTCIceCandidateInit {
    DEFINE_GETTER_SETTER(String*, candidate, Candidate)

    Nullable<String*> sdpMid()
    {
        return m_sdpMid;
    }

    void setSdpMid(Nullable<String*> sdpMid)
    {
        m_sdpMid = sdpMid;
    }

    Nullable<uint32_t> sdpMLineIndex()
    {
        return m_sdpMLineIndex;
    }

    void setSdpMLineIndex(Nullable<uint32_t> sdpMLineIndex)
    {
        m_sdpMLineIndex = sdpMLineIndex;
    }

    String* m_candidate{ String::emptyString };
    Nullable<String*> m_sdpMid;
    Nullable<uint32_t> m_sdpMLineIndex;
};

class RTCIceCandidate : public ScriptWrappable {
public:
    RTCIceCandidate(ExecutionContext* executionContext);
    RTCIceCandidate(ExecutionContext* executionContext,
                    RTCIceCandidateInit init);
    RTCIceCandidate(ExecutionContext* executionContext,
                    webrtc::IceCandidateInterface* candidate);
    virtual ~RTCIceCandidate();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(RTCIceCandidate)

    String* candidate();

    const webrtc::IceCandidateInterface* backend()
    {
        return m_backend.get();
    }

private:
    ExecutionContext* m_executionContext{ nullptr };
    std::unique_ptr<const webrtc::IceCandidateInterface> m_backend;
};
}
#endif
#endif
