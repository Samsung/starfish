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

#include "StarfishConfig.h"
#include "binding/generated/RequestOrUSVStringUnion.h"
#include "binding/generated/BlobOrBufferSourceOrUSVStringOrReadableStreamUnion.h"
#include "binding/ScriptBindingInstance.h"
#include "platform/loader/ResourceURL.h"
#include "core/fetch/Request.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/fetch/FetchUtils.h"

#include <EscargotPublic.h>
using namespace Escargot;

namespace Starfish {

extern BlobOrBufferSourceOrUSVStringOrReadableStream
toBlobOrBufferSourceOrUSVStringOrReadableStreamFromValueRef(
    ExecutionStateRef* state, ValueRef* from);
extern ValueRef* toValueRefFromBlobOrBufferSourceOrUSVStringOrReadableStream(
    ExecutionStateRef* state,
    BlobOrBufferSourceOrUSVStringOrReadableStream& from);
extern bool isBlobOrBufferSourceOrUSVString(ExecutionStateRef* state,
                                            ValueRef* from);

static BodyInit toBodyInitFromValueRef(ContextRef* ctx, ValueRef* from)
{
    BlobOrBufferSourceOrUSVStringOrReadableStream body;
    Evaluator::execute(
        ctx,
        [](ExecutionStateRef* state, ValueRef* from,
           BlobOrBufferSourceOrUSVStringOrReadableStream* body) -> ValueRef* {
            *body = toBlobOrBufferSourceOrUSVStringOrReadableStreamFromValueRef(
                state, from);

            return ValueRef::createUndefined();
        },
        from, &body);

    return body;
}

Request::Request(ExecutionContext* executionContext, RequestInfo& input)
    : ScriptWrappable(this)
    , Body(executionContext)
    , m_headers(Headers(executionContext))
{
    initialize(&input);
}

Request::Request(ExecutionContext* executionContext, RequestInfo& input,
                 RequestInit& init)
    : ScriptWrappable(this)
    , Body(executionContext)
    , m_headers(Headers(executionContext))
{
    initialize(&input, &init);
}

Request::Request(ExecutionContext* executionContext, RequestData* data)
    : ScriptWrappable(this)
    , Body(executionContext)
    , m_data(*data)
    , m_headers(Headers(executionContext))
{
}

static String* computeReferrer(String* referrer,
                               ExecutionContext* executionContext)
{
    // TODO: remove checking 'undefined' and 'about:blank'
    if (referrer->equals("about:blank") || referrer->isEmpty() ||
        referrer->equals("undefined")) {
        return String::emptyString;
    }

    String* contextOrigin = executionContext->baseURL()->origin();

    ResourceURL url(referrer, contextOrigin);

    // STARFISH_LOG_INFO("%s %s %s %s"
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

void Request::initialize(RequestInfo* input, NULLABLE RequestInit* init)
{
    // FIXME : Apply the https://fetch.spec.whatwg.org/#dom-request
    STARFISH_ASSERT(input != nullptr);

    String* fallbackMode = nullptr;
    String* fallbackCredentials = nullptr;

    // check input is string or Request
    if (input->isRequestValue()) {
        Request* request = input->getRequestValue();
        RequestData* data = &request->m_data;

        m_data.m_method = data->m_method;
        m_data.m_referrer = data->m_referrer;
        m_data.m_mode = data->m_mode;
        m_data.m_credentials = data->m_credentials;
        m_data.m_cache = data->m_cache;
        m_data.m_redirect = data->m_redirect;
        m_data.m_integrity = data->m_integrity;
        m_data.m_keepalive = data->m_keepalive;

        if (!init || init->headers()->isUndefinedOrNull()) {
            m_headers.copyHeaders(&request->m_headers);
        }

        copyBody(request);

    } else {
        if (input->isUSVStringValue()) {
            m_data.m_url =
                new ResourceURL(input->getUSVStringValue(),
                                executionContext()->baseURL()->baseURI());
            fallbackMode = String::createASCIIString("cors");
            fallbackCredentials = String::createASCIIString("same-origin");
        } else {
            return; // ignore or read the result of toString
        }
    }

    if (init) {
        if (init->hasMethod()) {
            m_data.m_method = init->method();
        } else {
            // FIXME : Remove this line after apply a
            // https://fetch.spec.whatwg.org/#dom-request
            m_data.m_method = String::createASCIIString("undefined");
        }

        if (!HeadersData::isValidHTTPToken(m_data.m_method) ||
            FetchUtils::isForbiddenMethod(m_data.m_method)) {
            throw new DOMException(executionContext(),
                                   DOMException::SCRIPT_TYPE_ERR,
                                   "SCRIPT_TYPE_ERR");
        }

        m_data.m_method = FetchUtils::normalizeMethod(m_data.m_method);

        String* referrerPolicy = String::emptyString;
        if (init->hasReferrerPolicy()) {
            referrerPolicy = init->referrerPolicy();
        }
        String* referrer = String::emptyString;
        if (init->hasReferrer()) {
            referrer = init->referrer();
        }

        if (referrer->isEmpty()) {
            m_data.m_referrer =
                new ReferrerURL(new ResourceURL(referrer), referrerPolicy);
        } else {
            m_data.m_referrer = new ReferrerURL(
                new ResourceURL(referrer,
                                executionContext()->baseURL()->baseURI()),
                referrerPolicy);
        }

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
        if (!body->isUndefinedOrNull()) {
            if (m_data.m_method->equals("GET") ||
                m_data.m_method->equals("HEAD")) {
                throw new DOMException(executionContext(),
                                       DOMException::Code::SCRIPT_TYPE_ERR);
            }

            this->setBodyInit(toBodyInitFromValueRef(
                this->scriptBindingInstance()->scriptContext(), body));
            if (this->contentType() != nullptr) {
                if (!m_headers.noCheckValidHas("content-type")) {
                    m_headers.noCheckValidSet("content-type",
                                              CSTR(this->contentType()));
                }
            }
        } else {
            this->setBodyInit(nullptr);
        }
    } else {
        if (input->isUSVStringValue()) {
            STARFISH_ASSERT(fallbackMode);
            STARFISH_ASSERT(fallbackCredentials);
            fallbackMode = String::createASCIIString("cors");
            fallbackCredentials = String::createASCIIString("same-origin");
            m_data.m_mode = RequestData::requestModeFromString(fallbackMode);
            m_data.m_credentials =
                RequestData::requestCredentialsFromString(fallbackCredentials);
        }
    }
}

ScriptBindingInstance* Request::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

Request* Request::clone()
{
    RequestInfo requestInfo = RequestOrUSVString::createRequest(this);
    return new Request(executionContext(), requestInfo);
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
    return m_data.m_url->urlString();
}

String* Request::destination()
{
    return String::emptyString;
}

String* Request::referrer()
{
    return computeReferrer(m_data.m_referrer->urlString(), executionContext());
}

String* Request::referrerPolicy()
{
    return m_data.m_referrer->referrerPolicyString();
}

String* Request::mode()
{
    switch (m_data.m_mode) {
    case RequestMode::Navigate:
        return String::createASCIIString("navigate");
    case RequestMode::SameOrigin:
        return String::createASCIIString("same-origin");
    case RequestMode::NoCORS:
        return String::createASCIIString("no-cors");
    case RequestMode::CORS:
        return String::createASCIIString("cors");
    default:
        break;
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::emptyString;
}

String* Request::credentials()
{
    switch (m_data.m_credentials) {
    case RequestCredentials::Omit:
        return String::createASCIIString("omit");
    case RequestCredentials::SameOrigin:
        return String::createASCIIString("same-origin");
    case RequestCredentials::Include:
        return String::createASCIIString("include");
    default:
        break;
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::emptyString;
}

String* Request::cache()
{
    switch (m_data.m_cache) {
    case RequestCache::Default:
        return String::createASCIIString("default");
    case RequestCache::NoStore:
        return String::createASCIIString("no-store");
    case RequestCache::Reload:
        return String::createASCIIString("reload");
    case RequestCache::NoCache:
        return String::createASCIIString("no-cache");
    case RequestCache::ForceCache:
        return String::createASCIIString("force-cache");
    case RequestCache::OnlyIfCached:
        return String::createASCIIString("only-if-cached");
    default:
        break;
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::emptyString;
}

String* Request::redirect()
{
    switch (m_data.m_redirect) {
    case RequestRedirect::Follow:
        return String::createASCIIString("follow");
    case RequestRedirect::Error:
        return String::createASCIIString("error");
    case RequestRedirect::Manual:
        return String::createASCIIString("manual");
    default:
        break;
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::emptyString;
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
} // namespace Starfish
