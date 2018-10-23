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

#include "StarfishConfig.h"
#include "core/fetch/RequestData.h"

namespace Starfish {

RequestData::RequestData()
    : m_method(MethodType::GET)
    , m_referrer(new ReferrerURL(String::createASCIIString("about:client"),
                                 ReferrerPolicy::Empty))
    , m_mode(RequestMode::CORS)
    , m_credentials(RequestCredentials::SameOrigin)
    , m_cache(RequestCache::Default)
    , m_redirect(RequestRedirect::Follow)
    , m_integrity(String::emptyString)
    , m_keepalive(false)
    , m_url(nullptr)
{
}

MethodType RequestData::methodTypeFromString(String* input)
{
    String* upper = input->toASCIIUpper();
    if (upper->equals("GET")) {
        return MethodType::GET;
    } else if (upper->equals("HEAD")) {
        return MethodType::HEAD;
    } else if (upper->equals("POST")) {
        return MethodType::POST;
    } else if (upper->equals("PUT")) {
        return MethodType::PUT;
    } else if (upper->equals("DELETE")) {
        return MethodType::DELETE;
    } else if (upper->equals("CONNECT")) {
        return MethodType::CONNECT;
    } else if (upper->equals("OPTIONS")) {
        return MethodType::OPTIONS;
    } else if (upper->equals("TRACE")) {
        return MethodType::TRACE;
    } else if (upper->equals("PATCH")) {
        return MethodType::PATCH;
    }
    return MethodType::UNKNOWN;
}

String* RequestData::methodTypeString(MethodType method)
{
    switch (method) {
    case MethodType::GET:
        return String::createASCIIString("GET");
    case MethodType::HEAD:
        return String::createASCIIString("HEAD");
    case MethodType::POST:
        return String::createASCIIString("POST");
    case MethodType::PUT:
        return String::createASCIIString("PUT");
    case MethodType::DELETE:
        return String::createASCIIString("DELETE");
    case MethodType::CONNECT:
        return String::createASCIIString("CONNECT");
    case MethodType::OPTIONS:
        return String::createASCIIString("OPTIONS");
    case MethodType::TRACE:
        return String::createASCIIString("TRACE");
    case MethodType::PATCH:
        return String::createASCIIString("PATCH");
    default:
        return String::emptyString;
    }
}

RequestMode RequestData::requestModeFromString(String* inputString)
{
    if (inputString->equalsIgnoreCase("navigate")) {
        return RequestMode::Navigate;
    } else if (inputString->equalsIgnoreCase("same-origin")) {
        return RequestMode::SameOrigin;
    } else if (inputString->equalsIgnoreCase("no-cors")) {
        return RequestMode::NoCORS;
    } else if (inputString->equalsIgnoreCase("cors")) {
        return RequestMode::CORS;
    }
    return RequestMode::NoCORS;
}

RequestCredentials RequestData::requestCredentialsFromString(
    String* inputString)
{
    if (inputString->equalsIgnoreCase("omit")) {
        return RequestCredentials::Omit;
    } else if (inputString->equalsIgnoreCase("same-origin")) {
        return RequestCredentials::SameOrigin;
    } else if (inputString->equalsIgnoreCase("include")) {
        return RequestCredentials::Include;
    }
    return RequestCredentials::Omit;
}

RequestCache RequestData::requestCacheFromString(String* inputString)
{
    if (inputString->equalsIgnoreCase("default")) {
        return RequestCache::Default;
    } else if (inputString->equalsIgnoreCase("no-store")) {
        return RequestCache::NoStore;
    } else if (inputString->equalsIgnoreCase("reload")) {
        return RequestCache::Reload;
    } else if (inputString->equalsIgnoreCase("no-cache")) {
        return RequestCache::NoCache;
    } else if (inputString->equalsIgnoreCase("force-cache")) {
        return RequestCache::ForceCache;
    } else if (inputString->equalsIgnoreCase("only-if-cached")) {
        return RequestCache::OnlyIfCached;
    }
    return RequestCache::Default;
}

RequestRedirect RequestData::requestRedirectFromString(String* inputString)
{
    if (inputString->equalsIgnoreCase("follow")) {
        return RequestRedirect::Follow;
    } else if (inputString->equalsIgnoreCase("error")) {
        return RequestRedirect::Error;
    } else if (inputString->equalsIgnoreCase("manual")) {
        return RequestRedirect::Manual;
    }
    return RequestRedirect::Follow;
}
}
