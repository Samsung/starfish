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

#include "core/modules/mediastream/RTCError.h"

#include "core/dom/ExecutionContext.h"

namespace Starfish {

String* RTCErrorInit::errorDetail()
{
    switch (m_errorDetail) {
    case RTCErrorDetailType::DataChannelFailure:
        return String::createASCIIString("data-channel-failure");
    case RTCErrorDetailType::DtlsFailure:
        return String::createASCIIString("dtls-failure");
    case RTCErrorDetailType::FingerprintFailure:
        return String::createASCIIString("fingerprint-failure");
    case RTCErrorDetailType::IdpBadScriptFailure:
        return String::createASCIIString("idp-bad-script-failure");
    case RTCErrorDetailType::IdpExecutionFailure:
        return String::createASCIIString("idp-execution-failure");
    case RTCErrorDetailType::IdpLoadFailure:
        return String::createASCIIString("idp-load-failure");
    case RTCErrorDetailType::IdpNeedLogin:
        return String::createASCIIString("idp-need-login");
    case RTCErrorDetailType::IdpTimeout:
        return String::createASCIIString("idp-timeout");
    case RTCErrorDetailType::IdpTlsFailure:
        return String::createASCIIString("idp-tls-failure");
    case RTCErrorDetailType::IdpTokenExpired:
        return String::createASCIIString("idp-token-expired");
    case RTCErrorDetailType::IdpTokenInvalid:
        return String::createASCIIString("idp-token-invalid");
    case RTCErrorDetailType::SctpFailure:
        return String::createASCIIString("sctp-failure");
    case RTCErrorDetailType::SdpSyntaxError:
        return String::createASCIIString("sdp-syntax-error");
    case RTCErrorDetailType::HardwareEncoderNotAvailable:
        return String::createASCIIString("hardware-encoder-not-available");
    case RTCErrorDetailType::HardwareEncoderError:
        return String::createASCIIString("hardware-encoder-error");
    default:
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    return String::emptyString;
}
void RTCErrorInit::setErrorDetail(String* errorDetail)
{
    if (errorDetail->equals("data-channel-failure")) {
        m_errorDetail = RTCErrorDetailType::DataChannelFailure;
    } else if (errorDetail->equals("dtls-failure")) {
        m_errorDetail = RTCErrorDetailType::DtlsFailure;
    } else if (errorDetail->equals("fingerprint-failure")) {
        m_errorDetail = RTCErrorDetailType::FingerprintFailure;
    } else if (errorDetail->equals("idp-bad-script-failure")) {
        m_errorDetail = RTCErrorDetailType::IdpBadScriptFailure;
    } else if (errorDetail->equals("idp-execution-failure")) {
        m_errorDetail = RTCErrorDetailType::IdpExecutionFailure;
    } else if (errorDetail->equals("idp-load-failure")) {
        m_errorDetail = RTCErrorDetailType::IdpLoadFailure;
    } else if (errorDetail->equals("idp-need-login")) {
        m_errorDetail = RTCErrorDetailType::IdpNeedLogin;
    } else if (errorDetail->equals("idp-timeout")) {
        m_errorDetail = RTCErrorDetailType::IdpTimeout;
    } else if (errorDetail->equals("idp-tls-failure")) {
        m_errorDetail = RTCErrorDetailType::IdpTlsFailure;
    } else if (errorDetail->equals("idp-token-expired")) {
        m_errorDetail = RTCErrorDetailType::IdpTokenExpired;
    } else if (errorDetail->equals("idp-token-invalid")) {
        m_errorDetail = RTCErrorDetailType::IdpTokenInvalid;
    } else if (errorDetail->equals("sctp-failure")) {
        m_errorDetail = RTCErrorDetailType::SctpFailure;
    } else if (errorDetail->equals("sdp-syntax-error")) {
        m_errorDetail = RTCErrorDetailType::SdpSyntaxError;
    } else if (errorDetail->equals("hardware-encoder-not-available")) {
        m_errorDetail = RTCErrorDetailType::HardwareEncoderNotAvailable;
    } else if (errorDetail->equals("hardware-encoder-error")) {
        m_errorDetail = RTCErrorDetailType::HardwareEncoderError;
    }
}

RTCError::RTCError(ExecutionContext* executionContext, RTCErrorInit init,
                   String* message)
    : DOMException(executionContext, Code::DOM_EXCEPTION, "")
    , m_executionContext(executionContext)
{
    m_errorDetail = init.m_errorDetail;
}

ScriptBindingInstance* RTCError::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

String* RTCError::errorDetailStr()
{
    RTCErrorInit init;
    init.m_errorDetail = m_errorDetail;
    return init.errorDetail();
}

} // namespace Starfish

#endif
