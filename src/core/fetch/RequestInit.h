/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishRequestInit__
#define __StarfishRequestInit__

#include "binding/ScriptWrappable.h"
#include "binding/BlobOrBufferSourceOrUSVStringOrReadableStreamUnion.h"
#include "core/fetch/Headers.h"

namespace Starfish {

typedef BlobOrBufferSourceOrUSVStringOrReadableStream BodyInit;

struct RequestInit : public gc {
    RequestInit()
        : m_method(String::emptyString)
        , m_referrer(String::emptyString)
        , m_referrerPolicy(String::emptyString)
        , m_mode(String::emptyString)
        , m_credentials(String::emptyString)
        , m_cache(String::emptyString)
        , m_redirect(String::emptyString)
        , m_integrity(String::emptyString)
        , m_keepalive(false)
        , m_headers(scriptUndefined())
        , m_body(scriptUndefined())
    {
    }

    String* m_method;
    String* m_referrer;
    String* m_referrerPolicy;
    String* m_mode;
    String* m_credentials;
    String* m_cache;
    String* m_redirect;
    String* m_integrity;
    bool m_keepalive;
    HeadersInit m_headers;
    ScriptValue m_body;

    DEFINE_GETTER_SETTER(String*, method, Method);
    DEFINE_GETTER_SETTER(String*, referrer, Referrer);
    DEFINE_GETTER_SETTER(String*, referrerPolicy, ReferrerPolicy);
    DEFINE_GETTER_SETTER(String*, mode, Mode);
    DEFINE_GETTER_SETTER(String*, credentials, Credentials);
    DEFINE_GETTER_SETTER(String*, cache, Cache);
    DEFINE_GETTER_SETTER(String*, redirect, Redirect);
    DEFINE_GETTER_SETTER(String*, integrity, Integrity);
    DEFINE_GETTER_SETTER(bool, keepalive, Keepalive);
    DEFINE_GETTER_SETTER(ScriptValue, body, Body);
    DEFINE_GETTER_SETTER(HeadersInit, headers, Headers);
};
}
#endif
