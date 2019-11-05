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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/mediastream/RTCDataChannel.h"

#include "core/dom/ExecutionContext.h"

namespace Starfish {

RTCDataChannel::RTCDataChannel(ExecutionContext* executionContext)
    : RTCDataChannel(executionContext, nullptr)
{
}

RTCDataChannel::RTCDataChannel(
    ExecutionContext* executionContext,
    rtc::scoped_refptr<webrtc::DataChannelInterface> dataChannel)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_backend(dataChannel)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) { ((RTCDataChannel*)obj)->~RTCDataChannel(); },
        NULL, NULL, NULL);
}

RTCDataChannel::~RTCDataChannel()
{
}

ExecutionContext* RTCDataChannel::executionContext() const
{
    return m_executionContext;
}

ScriptBindingInstance* RTCDataChannel::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}
} // namespace Starfish

#endif
