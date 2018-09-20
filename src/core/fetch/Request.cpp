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
#include "binding/RequestOrUSVStringUnion.h"
#include "binding/BlobOrBufferSourceOrUSVStringUnion.h"
#include "platform/loader/ResourceURL.h"
#include "core/fetch/Request.h"
#include "core/page/Window.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"

#include <EscargotPublic.h>
using namespace Escargot;

namespace StarFish {

extern BlobOrBufferSourceOrUSVString
toBlobOrBufferSourceOrUSVStringFromValueRef(ExecutionStateRef* state,
                                            ValueRef* from);
extern ValueRef* toValueRefFromBlobOrBufferSourceOrUSVString(
    ExecutionStateRef* state, BlobOrBufferSourceOrUSVString& from);
extern bool isBlobOrBufferSourceOrUSVString(ExecutionStateRef* state,
                                            ValueRef* from);

static BodyInit toBodyInitFromValueRef(ContextRef* ctx, ValueRef* from)
{
    ExecutionStateRef* state = ExecutionStateRef::create(ctx);
    BlobOrBufferSourceOrUSVString body =
        toBlobOrBufferSourceOrUSVStringFromValueRef(state, from);
    return body;
}

Request::Request(Window* window, RequestInfo& input)
    : ScriptWrappable(this)
    , Body(window)
    , m_headers(Headers(window->document()))
{
    initialize(&input);
}

Request::Request(Window* window, RequestInfo& input, RequestInit& init)
    : ScriptWrappable(this)
    , Body(window)
    , m_headers(Headers(window->document()))
{
    initialize(&input, &init);
}

static String* computeReferrer(String* referrer, Document* document)
{
    // TODO: remove checking 'undefined'
    if (referrer->isEmpty() || referrer->equals("undefined")) {
        return String::createASCIIString("");
    }

    String* contextOrigin = document->baseURL()->origin();

    ResourceURL url(referrer, contextOrigin);

    // STARFISH_LOG_INFO("%s %s %s %s\n"
    //                 , CSTR(url.urlString())
    //                 , CSTR(url.origin())
    //                 , CSTR(referrer)
    //                 , CSTR(contextOrigin));

    if (url.isValid() == false) {
        if (url.urlString()->startsWith("/")) {
            return contextOrigin->concat(referrer);
        }
        // TODO: throw an exception
        return String::createASCIIString("no-referrer");
    }

    if (url.protocol()->equals("about") && url.pathname()->equals("client")) {
        return String::createASCIIString("about:client");
    }

    if (url.origin()->equals(contextOrigin) == false) {
        return String::createASCIIString("about:client");
    }

    return url.urlString();
}

void Request::initialize(RequestInfo* input, RequestInit* init)
{
    // check input is string or Request
    if (input->isRequestValue()) {
        Request* request = input->getRequestValue();
        RequestData* data = &request->m_data;

        m_data.m_method = data->m_method;
        m_data.m_referrer = data->m_referrer;
        m_data.m_referrerPolicy = data->m_referrerPolicy;
        m_data.m_mode = data->m_mode;
        m_data.m_credentials = data->m_credentials;
        m_data.m_cache = data->m_cache;
        m_data.m_redirect = data->m_redirect;
        m_data.m_integrity = data->m_integrity;
        m_data.m_keepalive = data->m_keepalive;

        m_headers.copyHeaders(&request->m_headers);
        copyBody(request);

    } else {
        if (input->isUSVStringValue()) {
            String* tmp = input->getUSVStringValue();
            m_data.m_url = tmp;
        } else {
            return; // ignore or read the result of toString
        }
    }

    if (init) {
        m_data.m_method = init->method()->toASCIIUpper();
        m_data.m_referrer = computeReferrer(init->referrer(), document());
        m_data.m_referrerPolicy =
            ReferrerURL::policyFromString(init->referrerPolicy());
        m_data.m_mode = RequestData::requestModeFromString(init->mode());
        m_data.m_credentials =
            RequestData::requestCredentialsFromString(init->credentials());
        m_data.m_cache = RequestData::requestCacheFromString(init->cache());
        m_data.m_redirect =
            RequestData::requestRedirectFromString(init->redirect());
        m_data.m_integrity = init->integrity();
        m_data.m_keepalive = init->keepalive();

        // Set header
        m_headers.fill(init->headers());

        // Set body
        // NOTE: it's not supported to generate bindings for a composite type
        // in a dictionary (e.g, BodyInit of RequestInit).So it's given as
        // ScriptValue type.
        ScriptValue body = init->body();
        if (!body->isUndefined() && !body->isNull()) {
            if (m_data.m_method->equals("GET") ||
                m_data.m_method->equals("HEAD")) {
                throw new DOMException(document(),
                                       DOMException::Code::SCRIPT_TYPE_ERR);
            }

            this->setBody(toBodyInitFromValueRef(
                this->scriptBindingInstance()->scriptContext(), body));
            if (this->contentType() != nullptr) {
                // NOTE: check the action in case that the type already exists
                // if (m_headers.noCheckValidHas("content-type"))
                m_headers.noCheckValidSet("content-type",
                                          CSTR(this->contentType()));
            }
        }
    }
}

Request* Request::clone()
{
    RequestInfo requestInfo = RequestOrUSVString::createRequest(this);
    return new Request(this->window(), requestInfo);
}

Headers* Request::headers()
{
    return &m_headers;
}

String* Request::method()
{
    return m_data.m_method;
}

String* Request::url()
{
    return m_data.m_url;
}

String* Request::destination()
{
    return String::emptyString;
}

String* Request::referrer()
{
    return m_data.m_referrer;
}

String* Request::referrerPolicy()
{
    switch (m_data.m_referrerPolicy) {
    case ReferrerURL::NoReferrer:
        return String::createASCIIString("no-referrer");
    case ReferrerURL::NoReferrerWhenDowngrade:
        return String::createASCIIString("no-referrer-when-downgrade");
    case ReferrerURL::Origin:
        return String::createASCIIString("origin");
    case ReferrerURL::OriginWhenCrossOrigin:
        return String::createASCIIString("origin-when-cross-origin");
    case ReferrerURL::SameOrigin:
        return String::createASCIIString("same-origin");
    case ReferrerURL::StrictOrigin:
        return String::createASCIIString("strict-origin");
    case ReferrerURL::StrictOriginWhenCrossOrigin:
        return String::createASCIIString("strict-origin-when-cross-origin");
    case ReferrerURL::UnsafeUrl:
        return String::createASCIIString("unsafe-url");
    case ReferrerURL::Empty:
        return String::createASCIIString("");
    default:
        break;
    }

    STARFISH_RELEASE_ASSERT_NOT_REACHED();
    return String::createASCIIString("");
}

String* Request::mode()
{
    switch (m_data.m_mode) {
    case RequestData::RequestMode::Navigate:
        return String::createASCIIString("navigate");
    case RequestData::RequestMode::SameOrigin:
        return String::createASCIIString("same-origin");
    case RequestData::RequestMode::NoCORS:
        return String::createASCIIString("no-cors");
    case RequestData::RequestMode::CORS:
        return String::createASCIIString("cors");
    default:
        break;
    }

    STARFISH_RELEASE_ASSERT_NOT_REACHED();
    return String::createASCIIString("");
}

String* Request::credentials()
{
    switch (m_data.m_credentials) {
    case RequestData::RequestCredentials::Omit:
        return String::createASCIIString("omit");
    case RequestData::RequestCredentials::SameOrigin:
        return String::createASCIIString("same-origin");
    case RequestData::RequestCredentials::Include:
        return String::createASCIIString("include");
    default:
        break;
    }

    STARFISH_RELEASE_ASSERT_NOT_REACHED();
    return String::createASCIIString("");
}

String* Request::cache()
{
    switch (m_data.m_cache) {
    case RequestData::RequestCache::Default:
        return String::createASCIIString("default");
    case RequestData::RequestCache::NoStore:
        return String::createASCIIString("no-store");
    case RequestData::RequestCache::Reload:
        return String::createASCIIString("reload");
    case RequestData::RequestCache::NoCache:
        return String::createASCIIString("no-cache");
    case RequestData::RequestCache::ForceCache:
        return String::createASCIIString("force-cache");
    case RequestData::RequestCache::OnlyIfCached:
        return String::createASCIIString("only-if-cached");
    default:
        break;
    }

    STARFISH_RELEASE_ASSERT_NOT_REACHED();
    return String::createASCIIString("");
}

String* Request::redirect()
{
    switch (m_data.m_redirect) {
    case RequestData::RequestRedirect::Follow:
        return String::createASCIIString("follow");
    case RequestData::RequestRedirect::Error:
        return String::createASCIIString("error");
    case RequestData::RequestRedirect::Manual:
        return String::createASCIIString("manual");
    default:
        break;
    }

    STARFISH_RELEASE_ASSERT_NOT_REACHED();
    return String::createASCIIString("");
}

String* Request::integrity()
{
    return m_data.m_integrity;
}

bool Request::keepalive()
{
    return m_data.m_keepalive;
}

bool Request::isReloadNavigation()
{
    return false;
}

bool Request::isHistoryNavigation()
{
    return false;
}
}
