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

#ifndef __StarfishRTCSessionDescription__
#define __StarfishRTCSessionDescription__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"
#include "core/util/String.h"

namespace webrtc {
class SessionDescriptionInterface;
enum class SdpType;
}

namespace Starfish {
class ExecutionContext;

enum class RTCSdpType { Offer, Pranswer, Answer, Rollback };

struct RTCSessionDescriptionInit {
    friend class RTCSessionDescription;
    friend class RTCPeerConnection;

public:
    RTCSessionDescriptionInit()
    {
    }

    RTCSessionDescriptionInit(RTCSdpType type, String* sdp)
        : m_type(type)
        , m_sdp(sdp)
    {
    }

    RTCSessionDescriptionInit(Nullable<RTCSdpType> type, String* sdp)
        : m_type(type)
        , m_sdp(sdp)
    {
    }

    RTCSessionDescriptionInit(Nullable<webrtc::SdpType> type, std::string sdp);
    RTCSessionDescriptionInit(webrtc::SdpType type, std::string sdp);

    String* type();
    void setType(String* type);
    DEFINE_GETTER_SETTER(String*, sdp, Sdp);

    Nullable<webrtc::SdpType> toSdpType();

private:
    Nullable<RTCSdpType> m_type;
    String* m_sdp{ String::emptyString };
};

class RTCSessionDescription : public EventTarget {
public:
    RTCSessionDescription(ExecutionContext* executionContext,
                          const webrtc::SessionDescriptionInterface* backend);
    RTCSessionDescription(ExecutionContext* executionContext,
                          RTCSessionDescriptionInit& sessionDescriptionInit);
    virtual ~RTCSessionDescription();
    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(RTCSessionDescription)
    virtual ExecutionContext* executionContext() const override;

    String* type();
    DEFINE_GETTER(String*, sdp)

private:
    ExecutionContext* m_executionContext;
    Nullable<RTCSdpType> m_type;
    String* m_sdp{ String::emptyString };
};
} // namespace Starfish

#endif
#endif
