/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#include "core/modules/mediastream/RTCStats.h"

#include "EscargotPublic.h"
#include "core/modules/mediastream/RTCPeerConnectionStats.h"

using namespace Escargot;

namespace Starfish {

extern ValueRef* toValueRefFromRTCPeerConnectionStats(
    ExecutionStateRef* state, const RTCPeerConnectionStats& from);

template <typename T>
ScriptValue RTCStateToScriptValue(ContextRef* contextRef, T state,
                                  ValueRef* func(ExecutionStateRef*, const T&))
{
    return Evaluator::execute(
               contextRef,
               [](ExecutionStateRef* executionStateRef, T stats,
                  ValueRef* func(ExecutionStateRef*, const T&)) -> ValueRef* {
                   return func(executionStateRef, stats);
               },
               state, func)
        .result;
}

ScriptValue RTCStats::createScriptValueFromMediaRTCStats(
    ContextRef* contextRef,
    libwebrtc::scoped_refptr<libwebrtc::MediaRTCStats> mediaRTCStats)
{
    RTCStats rtcStats;
    rtcStats.fillStats(mediaRTCStats);

    if (rtcStats.type()->equals("peer-connection")) {
        RTCPeerConnectionStats rtcPeerConnectionStats(rtcStats);
        rtcPeerConnectionStats.fillStats(mediaRTCStats);

        return RTCStateToScriptValue(contextRef, rtcPeerConnectionStats,
                                     toValueRefFromRTCPeerConnectionStats);
    } else {
        // TODO: Implement other stats derived from RTCStats.
        STARFISH_UNSUPPORTED("Stats types other than peer-connection");
        return scriptUndefined();
    }
}

void RTCStats::fillStats(
    libwebrtc::scoped_refptr<libwebrtc::MediaRTCStats> mediaRTCStats)
{
    auto id = mediaRTCStats->id().std_string();
    auto type = mediaRTCStats->type().std_string();

    m_id = String::createASCIIString(id.c_str(), id.length());
    m_type = String::createASCIIString(type.c_str(), type.length());
    // TODO : Apply https://www.w3.org/TR/hr-time-3/#clock-resolution
    m_timestamp = mediaRTCStats->timestamp_us() / 1000.0;
}

} // namespace Starfish

#endif
