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

#include "core/modules/mediastream/RTCIceCandidate.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

RTCIceCandidateInit::RTCIceCandidateInit(std::string& candidate,
                                         std::string& sdpMid, int sdpMLineIndex)
{
    m_candidate =
        String::createASCIIString(candidate.c_str(), candidate.length());
    m_sdpMid = String::createASCIIString(sdpMid.c_str(), sdpMid.length());
    m_sdpMLineIndex = sdpMLineIndex;
}

RTCIceCandidate::RTCIceCandidate(ExecutionContext* executionContext,
                                 RTCIceCandidateInit init)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
{
    m_candidate = init.m_candidate;
    m_sdpMid = init.m_sdpMid;
    m_sdpMLineIndex = init.m_sdpMLineIndex;
    m_usernameFragment = init.m_usernameFragment;

    if (!m_sdpMid.hasValue() && !m_sdpMLineIndex.hasValue()) {
        throw new DOMException(executionContext, DOMException::SCRIPT_TYPE_ERR,
                               "TypeError");
    }

    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj,
                 void* cd) { ((RTCIceCandidate*)obj)->~RTCIceCandidate(); },
        NULL, NULL, NULL);
}

RTCIceCandidate::~RTCIceCandidate()
{
}

ScriptBindingInstance* RTCIceCandidate::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

std::unique_ptr<webrtc::IceCandidateInterface> RTCIceCandidate::genBackend()
{
    std::string sdpMid;
    if (m_sdpMid.hasValue()) {
        sdpMid = m_sdpMid.getValue()->toUTF8NonGCString();
    }

    std::string sdp(m_candidate->toUTF8NonGCString());

    uint32_t sdpMLineIndex = 0;
    if (m_sdpMLineIndex.hasValue()) {
        sdpMLineIndex = m_sdpMLineIndex.getValue();
    }

    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::IceCandidateInterface> candidate =
        std::unique_ptr<webrtc::IceCandidateInterface>(
            webrtc::CreateIceCandidate(sdpMid, sdpMLineIndex, sdp, &error));

    if (!candidate) {
        STARFISH_LOG_WARN("IceCandidate was not created");
    }

    return std::move(candidate);
}
}

#endif
