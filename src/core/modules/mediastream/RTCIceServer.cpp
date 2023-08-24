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

#include "core/modules/mediastream/RTCIceServer.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

RTCIceServer::RTCIceServer()
{
}

RTCIceServer::RTCIceServer(DOMStringOrSequenceOfDOMString urls,
                           String* username)
{
}

DOMStringOrSequenceOfDOMString RTCIceServer::urls() const
{
    return DOMStringOrSequenceOfDOMString::createSequenceOfDOMString(m_urls);
}

void RTCIceServer::setUrls(DOMStringOrSequenceOfDOMString& value)
{
    if (value.isNoneValue()) {
        return;
    }

    m_urls.clear();
    if (value.isDOMStringValue()) {
        String* url = value.getDOMStringValue();
        m_urls.push_back(url);
    } else if (value.isSequenceOfDOMStringValue()) {
        for (String* url : value.getSequenceOfDOMStringValue()) {
            m_urls.push_back(url);
        }
    }
}

String* RTCIceServer::username() const
{
    if (!hasUsername()) {
        return String::emptyString;
    }

    return m_username;
}

String* RTCIceServer::credentialType() const
{
    if (!hasValidCredentialType()) {
        return String::emptyString;
    }

    switch (m_credentialType) {
    case RTCIceCredentialType::Password:
        return String::createASCIIString("password");
    case RTCIceCredentialType::OAuth:
        return String::createASCIIString("oauth");
    default:
        return String::emptyString;
    }

    return String::emptyString;
}

void RTCIceServer::setCredentialType(String* type)
{
    m_hasValidCredentialType = true;
    if (type->equals("password")) {
        m_credentialType = RTCIceCredentialType::Password;
    } else if (type->equals("oauth")) {
        m_credentialType = RTCIceCredentialType::OAuth;
    } else {
        m_hasValidCredentialType = false;
    }
}
} // namespace Starfish
#endif
