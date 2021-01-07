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

#ifndef __StarfishRTCPeerConnectionIceErrorEvent__
#define __StarfishRTCPeerConnectionIceErrorEvent__

#include "core/dom/Event.h"
#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

namespace Starfish {
class ExecutionContext;

struct RTCPeerConnectionIceErrorEventInit : public EventInit {
    DEFINE_GETTER_SETTER(String*, hostCandidate, HostCandidate)

    String* m_hostCandidate{ nullptr };
};

class RTCPeerConnectionIceErrorEvent : public Event {
public:
    RTCPeerConnectionIceErrorEvent(ExecutionContext* executionContext);
    RTCPeerConnectionIceErrorEvent(ExecutionContext* executionContext,
                                   String* type);
    RTCPeerConnectionIceErrorEvent(
        ExecutionContext* executionContext, String* type,
        RTCPeerConnectionIceErrorEventInit eventEventInit);
    virtual ~RTCPeerConnectionIceErrorEvent(){};

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(RTCPeerConnectionIceErrorEvent)

    DEFINE_GETTER(String*, hostCandidate);

private:
    ExecutionContext* m_executionContext{ nullptr };
    String* m_hostCandidate{ nullptr };
};
} // namespace Starfish
#endif
#endif
