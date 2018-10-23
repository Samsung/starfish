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
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "platform/network/http/HTTPHeaderMap.h"
#include "core/fetch/Headers.h"
#include "core/fetch/Response.h"
#include "platform/loader/ResourceURL.h"

namespace Starfish {

Response::Response(Document* document, uint32_t status, std::string type,
                   std::string statusText)
    : ScriptWrappable(this)
    , Body(document->window())
    , m_instance(document->scriptBindingInstance())
    , m_responseInit()
    , m_headers(Headers(document))
    , m_type(String::createASCIIString(type.data()))
    , m_url(String::emptyString)
    , m_redirected(false)
    , m_ok(true)
    , m_status(status)
    , m_statusText(String::createASCIIString(statusText.data()))
    , m_mimeType(String::emptyString)
{
    m_headers.setGuard(Headers::Guard::Response);
}

Response::Response(Document* document, Nullable<BodyInit>& body)
    : Response(document)
{
    handleBodyInit(body);
}

Response::Response(Document* document, Nullable<BodyInit>& body,
                   ResponseInit& init)
    : Response(document)
{
    m_status = init.status();

    if (m_status < 200 || m_status > 599) {
        throw new DOMException(document, DOMException::Code::SCRIPT_RANGE_ERR);
    }

    if (!isValidReasonPhrase(init.statusText())) {
        throw new DOMException(document, DOMException::Code::SCRIPT_TYPE_ERR);
    }

    m_statusText = init.statusText();

    if (!isNullOrUndefinedScriptValue(init.headers())) {
        m_headers.fill(init.headers());
    }

    handleBodyInit(body);

    m_mimeType = m_headers.extractMIMEType();
}

void Response::handleBodyInit(Nullable<BodyInit>& body)
{
    if (body.hasValue()) {
        if (m_status == 101 || m_status == 204 || m_status == 205 ||
            m_status == 304) {
            throw new DOMException(document(),
                                   DOMException::Code::SCRIPT_TYPE_ERR);
        }

        setBody(body.getValue());

        if (m_contentType != nullptr &&
            !m_headers.noCheckValidHas("content-type")) {
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

Response* Response::error(Document* document)
{
    Response* response = new Response(document, 200, "error", "");
    response->m_ok = false;
    response->m_body = nullptr;
    response->m_status = 0;
    response->headers()->setGuard(Headers::Guard::Immutable);

    return response;
}

Response* Response::redirect(Document* document, String* url)
{
    ResourceURL parsedUrl = ResourceURL(url);

    if (!parsedUrl.isValid()) {
        throw new DOMException(document, DOMException::Code::SCRIPT_TYPE_ERR);
    }

    Response* response = new Response(document, 302, "default", "");
    response->headers()->setGuard(Headers::Guard::Immutable);
    response->headers()->noCheckValidSet(
        "location", parsedUrl.string()->toUTF8NonGCString());

    return response;
}

Response* Response::redirect(Document* document, String* url,
                             unsigned short status)
{
    if (!isValidRedirectStatus(status)) {
        throw new DOMException(document, DOMException::Code::SCRIPT_RANGE_ERR);
    }

    Response* response = redirect(document, url);
    response->setStatus(status);
    return response;
}

Response* Response::clone()
{
    Response* clonedResponse =
        new Response(document(), 200, m_type->toUTF8NonGCString().data(),
                     m_statusText->toUTF8NonGCString().data());

    clonedResponse->copyResponseData(this);
    clonedResponse->m_headers.copyHeaders(&m_headers);
    clonedResponse->copyBody(this);

    return clonedResponse;
}

void Response::copyResponseData(Response* src)
{
    auto url = src->url()->toUTF8NonGCString();
    m_url = String::fromUTF8(url.data(), url.length());
    m_redirected = src->redirected();
    m_status = src->status();
    m_ok = src->ok();
}
};
