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

#ifndef __StarfishRTCError__
#define __StarfishRTCError__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "core/dom/DOMException.h"

namespace Starfish {
class ExecutionContext;

enum class RTCErrorDetailType {
    DataChannelFailure,
    DtlsFailure,
    FingerprintFailure,
    IdpBadScriptFailure,
    IdpExecutionFailure,
    IdpLoadFailure,
    IdpNeedLogin,
    IdpTimeout,
    IdpTlsFailure,
    IdpTokenExpired,
    IdpTokenInvalid,
    SctpFailure,
    SdpSyntaxError,
    HardwareEncoderNotAvailable,
    HardwareEncoderError
};

struct RTCErrorInit : public gc {
    String* errorDetail();
    void setErrorDetail(String* errorDetail);

    RTCErrorDetailType m_errorDetail;
};

class RTCError : public DOMException {
public:
    RTCError(ExecutionContext* executionContext, RTCErrorInit init,
             String* message = String::emptyString);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(RTCError)

    String* errorDetailStr();

private:
    ExecutionContext* m_executionContext{ nullptr };

    RTCErrorDetailType m_errorDetail;
};
} // namespace Starfish
#endif
#endif
