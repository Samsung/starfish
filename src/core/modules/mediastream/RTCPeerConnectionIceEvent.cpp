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

#include "core/modules/mediastream/RTCPeerConnectionIceEvent.h"

#include "core/dom/ExecutionContext.h"

namespace Starfish {

RTCPeerConnectionIceEvent::RTCPeerConnectionIceEvent(
    ExecutionContext* executionContext)
    : Event(executionContext)
{
}

RTCPeerConnectionIceEvent::RTCPeerConnectionIceEvent(
    ExecutionContext* executionContext, String* type,
    RTCPeerConnectionIceEventInit init)
    : Event(executionContext, type, { init.bubbles(), init.cancelable() })
    , m_executionContext(executionContext)
    , m_candidate(init.m_candidate)
    , m_url(init.m_url)
{
}

ScriptBindingInstance* RTCPeerConnectionIceEvent::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}
} // namespace Starfish

#endif
