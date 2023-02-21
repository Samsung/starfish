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

#ifndef __StarfishRTCIceServer__
#define __StarfishRTCIceServer__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "binding/generated/DOMStringOrSequenceOfDOMStringUnion.h"
#include "core/modules/mediastream/RTCOAuthCredential.h"
#include "binding/generated/DOMStringOrRTCOAuthCredentialUnion.h"

namespace Starfish {

enum class RTCIceCredentialType {
    Password,
    OAuth,
};

struct RTCIceServer : public gc {
    RTCIceServer();
    RTCIceServer(DOMStringOrSequenceOfDOMString urls,
                 String* username = String::emptyString);
    virtual ~RTCIceServer(){};

    DOMStringOrSequenceOfDOMString urls();
    void setUrls(DOMStringOrSequenceOfDOMString& value);

    String* username();
    DEFINE_SETTER_WITH_HASFLAG(String*, username, Username);
    DEFINE_HASFLAG_GETTER(Username);

    DEFINE_GETTER_SETTER(DOMStringOrRTCOAuthCredential, credential, Credential)

    String* credentialType();
    void setCredentialType(String* type);

    bool hasValidCredentialType()
    {
        return m_hasValidCredentialType;
    }

    GCVector<String*> m_urls;
    String* m_username{ String::emptyString };
    DOMStringOrRTCOAuthCredential m_credential;
    RTCIceCredentialType m_credentialType{ RTCIceCredentialType::Password };

    bool m_hasUsername{ false };
    bool m_hasValidCredentialType{ true };
};
} // namespace Starfish
#endif
#endif
