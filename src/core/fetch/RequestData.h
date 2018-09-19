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

#ifndef __StarFishRequestData__
#define __StarFishRequestData__

#include "binding/ScriptWrappable.h"
#include "platform/loader/ResourceURL.h"

namespace StarFish {

// NOTE: consider extracting enums from ResourceRequest
typedef ReferrerURL::ReferrerPolicy ReferrerPolicy;

class Request;
class RequestData : public gc {
    // TODO: encapsulate class members
public:
    enum class RequestMode {
        Navigate,
        SameOrigin,
        NoCORS,
        CORS,
    };

    enum class RequestCredentials {
        Omit,
        SameOrigin,
        Include,
    };

    enum class RequestCache {
        Default,
        NoStore,
        Reload,
        NoCache,
        ForceCache,
        OnlyIfCached,
    };

    enum class RequestRedirect {
        Follow,
        Error,
        Manual,
    };

    RequestData();

    String* m_method;
    String* m_referrer;
    ReferrerPolicy m_referrerPolicy;
    RequestMode m_mode;
    RequestCredentials m_credentials;
    RequestCache m_cache;
    RequestRedirect m_redirect;
    String* m_integrity;
    bool m_keepalive;

    String* m_url;
    static RequestMode requestModeFromString(String* inputString);
    static RequestCredentials requestCredentialsFromString(String* inputString);
    static RequestCache requestCacheFromString(String* inputString);
    static RequestRedirect requestRedirectFromString(String* inputString);
};
}

#endif
