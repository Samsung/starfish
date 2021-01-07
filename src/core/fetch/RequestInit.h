/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishRequestInit__
#define __StarfishRequestInit__

#include "binding/ScriptWrappable.h"
#include "binding/BlobOrBufferSourceOrUSVStringOrReadableStreamUnion.h"
#include "core/fetch/Headers.h"

namespace Starfish {

typedef BlobOrBufferSourceOrUSVStringOrReadableStream BodyInit;

struct RequestInit : public gc {
    RequestInit()
    {
    }

    DEFINE_GETTER_SETTER_WITH_HASFLAG(String*, method, Method);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(String*, referrer, Referrer);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(String*, referrerPolicy, ReferrerPolicy);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(String*, mode, Mode);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(String*, credentials, Credentials);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(String*, cache, Cache);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(String*, redirect, Redirect);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(String*, integrity, Integrity);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(bool, keepalive, Keepalive);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(ScriptValue, body, Body);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(HeadersInit, headers, Headers);

private:
    bool m_hasMethod{ false };
    bool m_hasReferrer{ false };
    bool m_hasReferrerPolicy{ false };
    bool m_hasMode{ false };
    bool m_hasCredentials{ false };
    bool m_hasCache{ false };
    bool m_hasRedirect{ false };
    bool m_hasIntegrity{ false };
    bool m_hasKeepalive{ false };
    bool m_hasHeaders{ false };
    bool m_hasBody{ false };

    String* m_method{ String::emptyString };
    String* m_referrer{ String::emptyString };
    String* m_referrerPolicy{ String::emptyString };
    String* m_mode{ String::emptyString };
    String* m_credentials{ String::emptyString };
    String* m_cache{ String::emptyString };
    String* m_redirect{ String::emptyString };
    String* m_integrity{ String::emptyString };
    bool m_keepalive{ false };
    HeadersInit m_headers{ scriptUndefined() };
    ScriptValue m_body{ scriptUndefined() };
};
} // namespace Starfish
#endif
