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

#include "core/modules/mediastream/RTCRtpSender.h"

#include "core/dom/ExecutionContext.h"

namespace Starfish {

RTCRtpSender::RTCRtpSender(ExecutionContext* executionContext)
    : RTCRtpSender(executionContext, nullptr)
{
}

RTCRtpSender::RTCRtpSender(
    ExecutionContext* executionContext,
    rtc::scoped_refptr<webrtc::RtpSenderInterface> rtpSender)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_backend(rtpSender)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) { ((RTCRtpSender*)obj)->~RTCRtpSender(); },
        NULL, NULL, NULL);
}

RTCRtpSender::~RTCRtpSender()
{
}

ScriptBindingInstance* RTCRtpSender::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

MediaStreamTrack* RTCRtpSender::track()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
}

RTCDtlsTransport* RTCRtpSender::transport()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
}

Promise* RTCRtpSender::setParameters(RTCRtpSendParameters parameters)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
}

RTCRtpSendParameters RTCRtpSender::getParameters()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    RTCRtpSendParameters result;
    return result;
}

Promise* RTCRtpSender::replaceTrack(MediaStreamTrack* withTrack)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
}

void RTCRtpSender::setStreams(GCVector<MediaStream*>& streams)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

rtc::scoped_refptr<webrtc::RtpSenderInterface> RTCRtpSender::backend()
{
    return m_backend;
}
}

#endif
