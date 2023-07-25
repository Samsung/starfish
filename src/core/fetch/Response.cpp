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
#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "platform/network/http/HTTPHeaderMap.h"
#include "core/fetch/Headers.h"
#include "core/fetch/Response.h"
#include "platform/loader/ResourceURL.h"

namespace Starfish {

Response::Response(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , Body(executionContext)
    , m_executionContext(executionContext)
    , m_headers(Headers(executionContext))
    , m_responseData(new ResponseData())
{
    m_headers.setGuard(Guard::Response);
#if defined(STARFISH_ENABLE_SERVICE_WORKER)
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            Response* self = static_cast<Response*>(obj);
            self->~Response();
        },
        nullptr, nullptr, nullptr);
#endif
}

Response::Response(ExecutionContext* executionContext, Nullable<BodyInit>& body)
    : ScriptWrappable(this)
    , Body(executionContext, body)
    , m_executionContext(executionContext)
    , m_headers(Headers(executionContext))
    , m_responseData(new ResponseData())
{
    m_headers.setGuard(Guard::Response);
    handleBodyInit(body);
    setStatusText(String::createASCIIString("OK"));

#if defined(STARFISH_ENABLE_SERVICE_WORKER)
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            Response* self = static_cast<Response*>(obj);
            self->~Response();
        },
        nullptr, nullptr, nullptr);
#endif
}

Response::Response(ExecutionContext* executionContext, Nullable<BodyInit>& body,
                   ResponseInit& init)
    : ScriptWrappable(this)
    , Body(executionContext, body)
    , m_executionContext(executionContext)
    , m_headers(Headers(executionContext))
    , m_responseData(new ResponseData())
{
    m_headers.setGuard(Guard::Response);

    if (init.status() < 200 || init.status() > 599) {
        throw new DOMException(executionContext,
                               DOMException::Code::SCRIPT_RANGE_ERR);
    }
    setStatus(init.status());

    if (!isValidReasonPhrase(init.statusText())) {
        throw new DOMException(executionContext,
                               DOMException::Code::SCRIPT_TYPE_ERR);
    }

    setStatusText(init.statusText());

    if (init.hasHeaders() && !isNullOrUndefinedScriptValue(init.headers())) {
        m_headers.fill(init.headers());
    }

    handleBodyInit(body);

    setMimeType(m_headers.extractMIMEType());

#if defined(STARFISH_ENABLE_SERVICE_WORKER)
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            Response* self = static_cast<Response*>(obj);
            self->~Response();
        },
        nullptr, nullptr, nullptr);
#endif
}

Response::~Response()
{
    //  Destructors should be called for members that allocate memory
    //  internally, such as std::string, but are not gc targets.
}

ScriptBindingInstance* Response::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

void Response::handleBodyInit(Nullable<BodyInit>& body)
{
    const auto st = status();
    if (body.hasValue()) {
        if (st == 101 || st == 204 || st == 205 || st == 304) {
            throw new DOMException(executionContext(),
                                   DOMException::Code::SCRIPT_TYPE_ERR);
        }

        setBodyInit(body.getValue());

        if (m_headers.noCheckValidHas("content-type") == false) {
            m_headers.noCheckValidSet("content-type",
                                      m_contentType->toUTF8NonGCString());
        }
    }
}

bool Response::isValidReasonPhrase(String* text)
{
    for (size_t i = 0; i < text->length(); i++) {
        auto c = text->charAt(i);
        if (c == 0x7F || c > 0xFF || (c < 0x20 && c != '\t'))
            return false;
    }
    return true;
}

bool Response::isValidRedirectStatus(uint32_t status)
{
    return (status == 301 || status == 302 || status == 303 || status == 307 ||
            status == 308);
}

Response* Response::error(ExecutionContext* executionContext)
{
    Response* response = new Response(executionContext);
    response->setStatus(0);
    response->setStatusText(String::emptyString);
    response->setType(ResponseType::Error);
    response->m_bodyInit = nullptr;
    response->headers()->setGuard(Guard::Immutable);

    return response;
}

Response* Response::redirect(ExecutionContext* executionContext, String* url)
{
    ResourceURL parsedUrl = ResourceURL(url);

    if (!parsedUrl.isValid()) {
        throw new DOMException(executionContext,
                               DOMException::Code::SCRIPT_TYPE_ERR);
    }

    Response* response = new Response(executionContext);
    response->setStatus(302);
    response->setStatusText(String::emptyString);
    response->setType(ResponseType::Default);

    response->headers()->setGuard(Guard::Immutable);
    response->headers()->noCheckValidSet(
        "location", parsedUrl.string()->toUTF8NonGCString());

    return response;
}

Response* Response::redirect(ExecutionContext* executionContext, String* url,
                             unsigned short status)
{
    if (!isValidRedirectStatus(status)) {
        throw new DOMException(executionContext,
                               DOMException::Code::SCRIPT_RANGE_ERR);
    }

    Response* response = redirect(executionContext, url);
    response->setStatus(status);
    return response;
}

Response* Response::cloneWithoutBody()
{
    Response* clonedResponse = new Response(executionContext());
    clonedResponse->copyResponseData(this);
    clonedResponse->m_headers.copyHeaders(&m_headers);
    return clonedResponse;
}

Response* Response::clone()
{
    if (bodyDisturbedOrLocked()) {
        throw new DOMException(executionContext(),
                               DOMException::Code::SCRIPT_TYPE_ERR);
    }

    Response* clonedResponse = cloneWithoutBody();
    clonedResponse->copyBody(this);
    return clonedResponse;
}

void Response::copyResponseData(Response* src)
{
    auto url = src->url()->toUTF8NonGCString();
    setUrl(String::fromUTF8(url.data(), url.length()));
    setRedirected(src->redirected());
    setStatus(src->status());
    setType(src->typeValue());
    auto utf8Data = src->statusText()->toUTF8NonGCString();
    setStatusText(String::fromUTF8(utf8Data.data(), utf8Data.size()));
}

String* Response::url()
{
    return m_responseData->m_url;
}

void Response::setUrl(String* url)
{
    m_responseData->m_url = url;
}

bool Response::redirected()
{
    return m_responseData->m_redirected;
}

void Response::setRedirected(bool value)
{
    m_responseData->m_redirected = value;
}

uint32_t Response::status()
{
    return m_responseData->m_status;
}

void Response::setStatus(uint32_t status)
{
    m_responseData->m_status = status;
}

void Response::setType(ResponseType type)
{
    m_responseData->m_type = type;
}

ResponseType Response::typeValue()
{
    return m_responseData->m_type;
}

String* Response::type()
{
    return ResponseData::reponseTypeString(typeValue());
}

bool Response::ok()
{
    return 200 <= status() && 299 >= status();
}

void Response::setOk(bool ok)
{
    m_responseData->m_ok = ok;
}

String* Response::statusText()
{
    return m_responseData->m_statusText;
}

void Response::setStatusText(String* statusText)
{
    m_responseData->m_statusText = statusText;
}

String* Response::mimeType()
{
    return m_responseData->m_mimeType;
}

void Response::setMimeType(String* mimeType)
{
    m_responseData->m_mimeType = mimeType;
}

void Response::setHeadersFromHeaderMap(const HeaderMap& map)
{
    for (auto& pair : map) {
        auto key =
            String::createASCIIString(pair.first.c_str(), pair.first.length());
        auto value = String::createASCIIString(pair.second.c_str(),
                                               pair.second.length());
        m_headers.append(key, value);
    }
}

} // namespace Starfish
