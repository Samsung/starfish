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
    , m_data(new RequestData())
    , m_headers(new Headers(executionContext))
    , m_body(new Body(executionContext))
{
    initialize(&input);
}

Request::Request(ExecutionContext* executionContext, RequestInfo& input,
                 RequestInit& init)
    : ScriptWrappable(this)
    , m_data(new RequestData())
    , m_headers(new Headers(executionContext))
    , m_body(new Body(executionContext))
{
    initialize(&input, &init);
}

Request::Request(ExecutionContext* executionContext, RequestData* data)
    : ScriptWrappable(this)
    , m_data(new RequestData(*data))
    , m_headers(new Headers(executionContext))
    , m_body(new Body(executionContext))
{
}

static ReferrerURL* computeReferrer(String* referrer,
                                    ExecutionContext* executionContext)
{
    // TODO: remove checking 'undefined' and 'about:blank'
    if (referrer->equals("about:blank") || referrer->isEmpty() ||
        referrer->equals("undefined")) {
        return new ReferrerURL(String::emptyString, ReferrerPolicy::NoReferrer);
    }

    String* contextOrigin = executionContext->baseURL()->origin();

    ReferrerURL* url =
        new ReferrerURL(new ResourceURL(referrer, contextOrigin));

    if (url->isValid() == false) {
        throw new DOMException(executionContext,
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "Referrer is not a valid URL.");
    }

    if (url->protocol()->equals("about") && url->pathname()->equals("client")) {
        return new ReferrerURL(String::createASCIIString("about:client"));
    }

    if (url->origin()->equals(contextOrigin) == false) {
        return new ReferrerURL(String::createASCIIString("about:client"));
    }

    return url;
}

void Request::initialize(RequestInfo* input, NULLABLE RequestInit* init)
{
    // FIXME : Apply the https://fetch.spec.whatwg.org/#dom-request
    STARFISH_ASSERT(input != nullptr);

    String* fallbackMode = String::emptyString;

    // check input is string or Request
    if (input->isRequestValue()) {
        Request* request = input->getRequestValue();
        RequestData* data = request->m_data;

        // Without the URL every use of the new request - Request.url,
        // fetch(request.clone()) - dereferences a null ResourceURL.
        m_data->m_url = data->m_url;
        m_data->m_method = data->m_method;
        m_data->m_referrer = data->m_referrer;
        m_data->m_mode = data->m_mode;
        m_data->m_credentials = data->m_credentials;
        m_data->m_cache = data->m_cache;
        m_data->m_redirect = data->m_redirect;
        m_data->m_integrity = data->m_integrity;
        m_data->m_keepalive = data->m_keepalive;

        if (!init || init->headers()->isUndefinedOrNull()) {
            m_headers->copyHeaders(request->m_headers);
        }

        if (m_body->bodyDisturbedOrLocked()) {
            throw new DOMException(executionContext(),
                                   DOMException::Code::SCRIPT_TYPE_ERR,
                                   "Request input is disturbed or locked");
        }
        m_body->copyBody(request->requestBody());

    } else {
        if (input->isUSVStringValue()) {
            m_data->m_url =
                new ResourceURL(input->getUSVStringValue(),
                                executionContext()->baseURL()->baseURI());
            fallbackMode = String::createASCIIString("cors");
        } else {
            return; // ignore or read the result of toString
        }
    }

    if (init) {
        buildRequestInit(init, fallbackMode);
    } else {
        if (input->isUSVStringValue()) {
            fallbackMode = String::createASCIIString("cors");
            m_data->m_mode = RequestData::requestModeFromString(fallbackMode);
        }
    }
}

void Request::buildRequestInit(RequestInit* init, String* fallbackMode)
{
    if (m_data->m_mode == RequestMode::Navigate) {
        m_data->m_mode = RequestMode::SameOrigin;
    }

    // TODO: Unset reload-navigation and history-navigation flag

    if (init->hasReferrer()) {
        m_data->m_referrer =
            computeReferrer(init->referrer(), executionContext());
    }

    if (init->hasReferrerPolicy()) {
        if (ReferrerURL::isValidPolicy(init->referrerPolicy())) {
            m_data->m_referrer->SetPolicy(
                ReferrerURL::policyFromString(init->referrerPolicy()));
        }
    }

    if (!HeadersData::isValidHTTPToken(m_data->m_method) ||
        FetchUtils::isForbiddenMethod(m_data->m_method)) {
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_TYPE_ERR,
                               "SCRIPT_TYPE_ERR");
    }

    String* mode = fallbackMode;
    if (init->hasMode()) {
        mode = init->mode();
    }

    if (mode->equals("navigate")) {
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_TYPE_ERR,
                               "SCRIPT_TYPE_ERR");
    }

    if (!mode->isEmpty()) {
        m_data->m_mode = RequestData::requestModeFromString(mode);
    }

    if (init->hasCredentials()) {
        m_data->m_credentials =
            RequestData::requestCredentialsFromString(init->credentials());
    }

    if (init->hasCache()) {
        m_data->m_cache = RequestData::requestCacheFromString(init->cache());
    }

    if (m_data->m_cache == RequestCache::OnlyIfCached &&
        m_data->m_mode != RequestMode::SameOrigin) {
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_TYPE_ERR);
    }

    if (init->hasRedirect()) {
        m_data->m_redirect =
            RequestData::requestRedirectFromString(init->redirect());
    }

    if (init->hasIntegrity()) {
        m_data->m_integrity = init->integrity();
    }

    if (init->hasKeepalive()) {
        m_data->m_keepalive = init->keepalive();
    }

    if (init->hasMethod()) {
        if (!HeadersData::isValidHTTPToken(init->method()) ||
            FetchUtils::isForbiddenMethod(init->method())) {
            throw new DOMException(executionContext(),
                                   DOMException::SCRIPT_TYPE_ERR);
        }
        m_data->m_method = FetchUtils::normalizeMethod(init->method());
    }

    if (init->hasHeaders()) {
        m_headers->fill(init->headers());
    }

    // Set body
    if (init->hasBody() && init->body().hasValue()) {
        checkMethodCanHaveBody();

        m_body->setBodyInit(init->body().value());
    } else {
        m_body->setBodyInit(nullptr);
    }

    if (!m_body->contentType()->isEmpty() &&
        !m_headers->noCheckValidHas("content-type")) {
        m_headers->noCheckValidSet("content-type", CSTR(m_body->contentType()));
    }
}

void Request::checkMethodCanHaveBody()
{
    if (m_data->m_method->equals("GET") || m_data->m_method->equals("HEAD")) {
        throw new DOMException(executionContext(),
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "Request cannot have a body");
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
    return m_headers;
}

String* Request::method()
{
    return m_data->m_method;
}

String* Request::url()
{
    return m_data->m_url->urlString();
}

String* Request::destination()
{
    return String::emptyString;
}

String* Request::referrer()
{
    auto referrerString = m_data->m_referrer->urlString();
    if (referrerString->equals("no-referrer") ||
        referrerString->equals("about:blank")) {
        return String::emptyString;
    } else if (referrerString->equals("about:client")) {
        return referrerString;
    }

    return m_data->m_referrer->serialize();
}

String* Request::referrerPolicy()
{
    return m_data->m_referrer->referrerPolicyString();
}

String* Request::mode()
{
    switch (m_data->m_mode) {
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
    switch (m_data->m_credentials) {
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
    switch (m_data->m_cache) {
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
    switch (m_data->m_redirect) {
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
    return m_data->m_integrity;
}

bool Request::keepalive()
{
    return m_data->m_keepalive;
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
