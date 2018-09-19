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

#include "StarFishConfig.h"
#include "core/fetch/RequestData.h"

namespace StarFish {

RequestData::RequestData()
    : m_method(String::createASCIIString("GET"))
    , m_referrer(String::createASCIIString("about:client"))
    , m_referrerPolicy(ReferrerURL::Empty)
    , m_mode(RequestMode::NoCORS)
    , m_credentials(RequestCredentials::SameOrigin)
    , m_cache(RequestCache::Default)
    , m_redirect(RequestRedirect::Follow)
    , m_integrity(String::emptyString)
    , m_keepalive(false)
{
}

RequestData::RequestMode RequestData::requestModeFromString(String* inputString)
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

RequestData::RequestCredentials RequestData::requestCredentialsFromString(
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

RequestData::RequestCache RequestData::requestCacheFromString(
    String* inputString)
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

RequestData::RequestRedirect RequestData::requestRedirectFromString(
    String* inputString)
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
