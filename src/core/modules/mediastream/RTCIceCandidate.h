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

struct RTCIceCandidateInit : public gc {
    RTCIceCandidateInit()
    {
    }

    RTCIceCandidateInit(std::string& candidate, std::string& sdpMid,
                        int sdpMLineIndex);

    DEFINE_GETTER_SETTER(String*, candidate, Candidate)
    DEFINE_GETTER_SETTER(Nullable<String*>, sdpMid, SdpMid)
    DEFINE_GETTER_SETTER(Nullable<uint32_t>, sdpMLineIndex, SdpMLineIndex)
    DEFINE_GETTER_SETTER(Nullable<String*>, usernameFragment, UsernameFragment)

    Nullable<String*> ufrag()
    {
        return m_usernameFragment;
    }

    void setUfrag(Nullable<String*> usernameFragment)
    {
        m_usernameFragment = usernameFragment;
    }

    String* m_candidate{ String::emptyString };
    Nullable<String*> m_sdpMid;
    Nullable<uint32_t> m_sdpMLineIndex;
    Nullable<String*> m_usernameFragment;
};

class RTCIceCandidate : public ScriptWrappable {
public:
    RTCIceCandidate(ExecutionContext* executionContext,
                    RTCIceCandidateInit init = RTCIceCandidateInit());
    virtual ~RTCIceCandidate();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(RTCIceCandidate)

    DEFINE_GETTER(String*, candidate)
    DEFINE_GETTER(Nullable<String*>, sdpMid)
    DEFINE_GETTER(Nullable<uint32_t>, sdpMLineIndex)
    DEFINE_GETTER(Nullable<String*>, usernameFragment)

    Nullable<String*> ufrag()
    {
        return usernameFragment();
    }

    std::unique_ptr<webrtc::IceCandidateInterface> genBackend();

private:
    ExecutionContext* m_executionContext{ nullptr };
    String* m_candidate{ String::emptyString };
    Nullable<String*> m_sdpMid;
    Nullable<uint32_t> m_sdpMLineIndex;
    Nullable<String*> m_usernameFragment;
};
}
#endif
#endif
