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

#ifndef __StarfishRTCStats__
#define __StarfishRTCStats__

#include "binding/ScriptWrappable.h"
#include "rtc_peerconnection.h"

namespace Starfish {

struct RTCStats {
public:
    RTCStats() = default;
    RTCStats(double timestamp, String* type, String* id)
        : m_timestamp(timestamp)
        , m_type(type)
        , m_id(id)
    {
    }

    STARFISH_MAKE_STACK_ALLOCATED()

    DEFINE_GETTER_SETTER(double, timestamp, Timestamp)
    DEFINE_GETTER_SETTER(String*, type, Type)
    DEFINE_GETTER_SETTER(String*, id, Id)

    void fillStats(libwebrtc::scoped_refptr<libwebrtc::MediaRTCStats> stats);

    static ScriptValue createScriptValueFromMediaRTCStats(
        Escargot::ContextRef* contextRef,
        libwebrtc::scoped_refptr<libwebrtc::MediaRTCStats> stats);

protected:
    double m_timestamp = 0.0;
    String* m_type = nullptr;
    String* m_id = nullptr;
};

} // namespace Starfish

#endif
#endif
