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

#ifndef __StarfishRequestData__
#define __StarfishRequestData__

namespace Starfish {

// NOTE: consider extracting enums from ResourceRequest

class Request;
class ResourceURL;

enum class MethodType {
    UNKNOWN,
    GET,
    HEAD,
    POST,
    PUT,
    DELETE,
    CONNECT,
    OPTIONS,
    TRACE,
    PATCH
};

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

enum class RequestDestination {
    Empty,
    Audio,
    AudioWorkLet,
    Document,
    Embed,
    Font,
    Image,
    Manifest,
    Object,
    PaintWorkLet,
    Report,
    Script,
    SharedWorker,
    Style,
    Track,
    Video,
    Worker,
    Xslt,
};

// RequestSyncLevel is used as the synchronous-flag
enum class RequestSyncLevel {
    SyncIfAlreadyLoaded,
    NeverSync,
    AlwaysSync,
};

class RequestData : public gc {
    // TODO: encapsulate class members
public:
    RequestData();

    MethodType m_method;
    ReferrerURL* m_referrer;
    RequestMode m_mode;
    RequestCredentials m_credentials;
    RequestCache m_cache;
    RequestRedirect m_redirect;
    RequestDestination m_destination;
    RequestSyncLevel m_syncLevel;

    String* m_integrity;
    bool m_keepalive;
    ResourceURL* m_url;

    static MethodType methodTypeFromString(String* inputString);
    static String* methodTypeString(MethodType method);
    static RequestMode requestModeFromString(String* inputString);
    static RequestCredentials requestCredentialsFromString(String* inputString);
    static RequestCache requestCacheFromString(String* inputString);
    static RequestRedirect requestRedirectFromString(String* inputString);
    static RequestDestination requestDestinationFromString(String* inputString);
};
}

#endif
