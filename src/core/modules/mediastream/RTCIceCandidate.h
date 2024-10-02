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

#ifndef __StarfishRTCIceCandidate__
#define __StarfishRTCIceCandidate__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "rtc_ice_candidate.h"

namespace Starfish {
class ExecutionContext;

struct RTCIceCandidatePair {
    DEFINE_GETTER_SETTER(RTCIceCandidate*, local, Local);
    DEFINE_GETTER_SETTER(RTCIceCandidate*, remote, Remote);

    RTCIceCandidate* m_local;
    RTCIceCandidate* m_remote;
};

struct RTCIceCandidateInit : public gc {
    RTCIceCandidateInit()
    {
    }

    RTCIceCandidateInit(const std::string& candidate, const std::string& sdpMid,
                        int sdpMLineIndex);

    DEFINE_GETTER_SETTER(String*, candidate, Candidate)
    DEFINE_GETTER_SETTER(Optional<String*>, sdpMid, SdpMid)
    DEFINE_GETTER_SETTER(Optional<uint32_t>, sdpMLineIndex, SdpMLineIndex)
    DEFINE_GETTER_SETTER(Optional<String*>, usernameFragment, UsernameFragment)

    Optional<String*> ufrag()
    {
        return m_usernameFragment;
    }

    void setUfrag(Optional<String*> usernameFragment)
    {
        m_usernameFragment = usernameFragment;
    }

    String* m_candidate{ String::emptyString };
    Optional<String*> m_sdpMid;
    Optional<uint32_t> m_sdpMLineIndex;
    Optional<String*> m_usernameFragment;
};

class RTCIceCandidate : public ScriptWrappable {
public:
    RTCIceCandidate(ExecutionContext* executionContext,
                    RTCIceCandidateInit init = RTCIceCandidateInit());

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(RTCIceCandidate)

    DEFINE_GETTER(String*, candidate)
    DEFINE_GETTER(Optional<String*>, sdpMid)
    DEFINE_GETTER(Optional<uint32_t>, sdpMLineIndex)
    DEFINE_GETTER(Optional<String*>, usernameFragment)

    Optional<String*> ufrag()
    {
        return usernameFragment();
    }

    libwebrtc::scoped_refptr<libwebrtc::RTCIceCandidate> genBackend();

private:
    ExecutionContext* m_executionContext{ nullptr };
    String* m_candidate{ String::emptyString };
    Optional<String*> m_sdpMid;
    Optional<uint32_t> m_sdpMLineIndex;
    Optional<String*> m_usernameFragment;
};
} // namespace Starfish

#endif
#endif
