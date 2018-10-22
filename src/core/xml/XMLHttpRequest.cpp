/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/dom/DOMParser.h"
#include "core/dom/ProgressEvent.h"
#include "core/fileapi/Blob.h"
#include "core/xml/XMLHttpRequest.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/page/Window.h"
#include "platform/network/http/HTTPStatus.h"

namespace StarFish {

DEFINE_EVENT_LISTENER(XMLHttpRequestEventTarget, loadstart);
DEFINE_EVENT_LISTENER(XMLHttpRequestEventTarget, progress);
DEFINE_EVENT_LISTENER(XMLHttpRequestEventTarget, abort);
DEFINE_EVENT_LISTENER(XMLHttpRequestEventTarget, error);
DEFINE_EVENT_LISTENER(XMLHttpRequestEventTarget, load);
DEFINE_EVENT_LISTENER(XMLHttpRequestEventTarget, timeout);
DEFINE_EVENT_LISTENER(XMLHttpRequestEventTarget, loadend);

class XMLHttpRequestResourceRequestClient : public ResourceRequestClient {
public:
    XMLHttpRequestResourceRequestClient(XMLHttpRequest* xhr)
        : m_xhr(xhr)
    {
    }
    void onProgressEvent(ResourceRequest* request, bool isExplicitAction)
    {
        String* eventName = String::emptyString;
        ProgressState progState = request->progressState();
        if (progState == ProgressState::Progress) {
            eventName =
                request->starFish()->staticStrings()->m_progress.localName();
        } else if (progState == ProgressState::InError) {
            eventName =
                request->starFish()->staticStrings()->m_error.localName();
            if (!m_xhr->m_resourceRequest->url()->isFileURL() &&
                !m_xhr->m_resourceRequest->url()->isDataURL() &&
                request->isSync()) {
                throw new DOMException(
                    m_xhr->scriptBindingInstance()->ownerDocument(),
                    DOMException::NETWORK_ERR, "NetworkError");
            }
        } else if (progState == ProgressState::Abort) {
            if (isExplicitAction) {
                return;
            }
            eventName =
                request->starFish()->staticStrings()->m_abort.localName();
        } else if (progState == ProgressState::TimeOut) {
            eventName =
                request->starFish()->staticStrings()->m_timeout.localName();
        } else if (progState == ProgressState::Load) {
            eventName =
                request->starFish()->staticStrings()->m_load.localName();
        } else if (progState == ProgressState::LoadEnd) {
            eventName =
                request->starFish()->staticStrings()->m_loadend.localName();
        } else if (progState == ProgressState::LoadStart) {
            eventName =
                request->starFish()->staticStrings()->m_loadstart.localName();
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }

        ProgressEvent* pe = new ProgressEvent(
            m_xhr->scriptBindingInstance()->ownerDocument(), eventName);
        pe->setLengthComputable(request->total() > 0);
        pe->setLoaded(request->loaded());
        pe->setTotal(request->total());
        m_xhr->EventTarget::dispatchEventByUA(m_xhr, pe);
    }

    void onReadyStateChange(ResourceRequest* request, bool fromExplicit)
    {
        if (fromExplicit) {
            if (request->readyState() == ReadyState::Done) {
                if (m_xhr->m_responseType ==
                        XMLHttpRequest::ResponseType::Unspecified ||
                    m_xhr->m_responseType ==
                        XMLHttpRequest::ResponseType::Text) {
                    TextConverter textConverter(
                        m_xhr->m_resourceRequest->responseMimeType(),
                        String::fromUTF8("UTF-8"),
                        m_xhr->m_resourceRequest->response().data(),
                        m_xhr->m_resourceRequest->response().size());
                    m_xhr->m_responseText = textConverter.convert(
                        m_xhr->m_resourceRequest->response().data(),
                        m_xhr->m_resourceRequest->response().size(), true);
                    m_xhr->m_resourceRequest->response().clear();
                } else if (m_xhr->m_responseType ==
                           XMLHttpRequest::ResponseType::Json) {
                    TextConverter cvt(
                        m_xhr->m_resourceRequest->responseMimeType(),
                        String::fromUTF8("UTF-8"),
                        m_xhr->m_resourceRequest->response().data(),
                        m_xhr->m_resourceRequest->response().size());
                    String* text = cvt.convert(
                        m_xhr->m_resourceRequest->response().data(),
                        m_xhr->m_resourceRequest->response().size(), true);
                    m_xhr->m_responseJsonObject =
                        parseJSON(m_xhr->scriptBindingInstance(), text);
                } else if (m_xhr->m_responseType ==
                           XMLHttpRequest::ResponseType::Blob) {
                    void* buffer =
                        calloc(1, m_xhr->m_resourceRequest->response().size());
                    memcpy(buffer, m_xhr->m_resourceRequest->response().data(),
                           m_xhr->m_resourceRequest->response().size());
                    m_xhr->m_responseBlob = new ::StarFish::Blob(
                        m_xhr->scriptBindingInstance()->ownerDocument(),
                        m_xhr->m_resourceRequest->response().size(),
                        m_xhr->m_resourceRequest->responseMimeType(), buffer,
                        false, false);
                    m_xhr->m_resourceRequest->response().clear();
                    m_xhr->m_resourceRequest->response().shrink_to_fit();
                } else if (m_xhr->m_responseType ==
                           XMLHttpRequest::ResponseType::ArrayBuffer) {
                    void* buffer =
                        calloc(1, m_xhr->m_resourceRequest->response().size());
                    memcpy(buffer, m_xhr->m_resourceRequest->response().data(),
                           m_xhr->m_resourceRequest->response().size());
                    m_xhr->m_responseArrayBuffer = createArrayBuffer(
                        m_xhr->scriptBindingInstance(), buffer,
                        m_xhr->m_resourceRequest->response().size());
                    m_xhr->m_resourceRequest->response().clear();
                    m_xhr->m_resourceRequest->response().shrink_to_fit();
                } else if (m_xhr->m_responseType ==
                           XMLHttpRequest::ResponseType::Document) {
                    void* buffer =
                        calloc(1, m_xhr->m_resourceRequest->response().size());
                    memcpy(buffer, m_xhr->m_resourceRequest->response().data(),
                           m_xhr->m_resourceRequest->response().size());
                    DOMParser* parser = new DOMParser(m_xhr->document());
                    m_xhr->m_responseXML = parser->parseFromString(
                        String::fromUTF8(static_cast<const char*>(buffer)),
                        request->responseMimeType());
                    free(buffer);
                    m_xhr->m_resourceRequest->response().clear();
                    m_xhr->m_resourceRequest->response().shrink_to_fit();
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
            }

            String* eventType = request->starFish()
                                    ->staticStrings()
                                    ->m_readystatechange.localName();
            Event* e =
                new Event(m_xhr->scriptBindingInstance()->ownerDocument(),
                          eventType, EventInit(true, true));
            m_xhr->EventTarget::dispatchEventByUA(m_xhr, e);
        }
    }

    XMLHttpRequest* m_xhr;
};

XMLHttpRequest::XMLHttpRequest(::StarFish::Document* document)
    : XMLHttpRequestEventTarget(document)
    , m_resourceRequest(new ResourceRequest(document))
    , m_withCredentials(false)
{
    /*
    GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
        STARFISH_LOG_INFO("XMLHttpRequest::~XMLHttpRequest\n");
    }, NULL, NULL, NULL);
    */

    m_responseType = ResponseType::Unspecified;
    initResponseData();
    m_resourceRequest->addResourceRequestClient(
        new XMLHttpRequestResourceRequestClient(this));
}

void XMLHttpRequest::initResponseData()
{
    m_responseText = String::emptyString;
    m_responseJsonObject = scriptNull();
    m_responseBlob = nullptr;
    m_responseArrayBuffer = scriptNull();
    m_responseXML = nullptr;
}

void XMLHttpRequest::send(Nullable<String*> body)
{
    if (body.hasValue()) {
        send(body.getValue());
    } else {
        send(String::emptyString);
    }
}

void XMLHttpRequest::send(String* body)
{
    if (m_resourceRequest->readyState() != ReadyState::Opened) {
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }
    m_resourceRequest->send(body, false);
}

DEFINE_EVENT_LISTENER(XMLHttpRequest, readystatechange);

void XMLHttpRequest::open(String* method, String* url)
{
    open(RequestData::methodTypeFromString(method), url, true,
         String::emptyString, String::emptyString);
}

void XMLHttpRequest::open(String* method, String* url, bool async,
                          Nullable<String*> userName,
                          Nullable<String*> password)
{
    String* uValue =
        userName.hasValue() ? userName.getValue() : String::emptyString;
    String* pValue =
        password.hasValue() ? password.getValue() : String::emptyString;
    open(RequestData::methodTypeFromString(method), url, async, uValue, pValue);
}

void XMLHttpRequest::open(MethodType method, String* url, bool async,
                          String* userName, String* password)
{
    if (method == MethodType::UNKNOWN) {
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::SYNTAX_ERR, "SYNTAX_ERR");
    }
    if (!async && m_resourceRequest->timeout() != 0) {
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::INVALID_ACCESS_ERR,
                               "InvalidAccessError");
    }

    RequestData* reqData = new RequestData();
    reqData->m_method = method;
    reqData->m_url = new ResourceURL(url, document()->baseURL()->baseURI());
    reqData->m_referrer = new ReferrerURL(document()->documentURI(),
                                          document()->referrerPolicy());
    if (userName->length()) {
        reqData->m_url->setUsername(userName);
    }
    if (password->length()) {
        reqData->m_url->setPassword(password);
    }
    if (m_withCredentials) {
        reqData->m_credentials = RequestCredentials::Include;
    } else {
        reqData->m_credentials = RequestCredentials::SameOrigin;
    }
    m_resourceRequest->open(reqData, async);

    initResponseData();
}

void XMLHttpRequest::abort()
{
    m_resourceRequest->abort();
    initResponseData();
}

void XMLHttpRequest::setResponseType(ResponseType type)
{
    // If the state is LOADING or DONE, throw an "InvalidStateError" exception.
    if (m_resourceRequest->readyState() == ReadyState::Loading ||
        m_resourceRequest->readyState() == ReadyState::Done) {
        throw new DOMException(
            scriptBindingInstance()->ownerDocument(),
            DOMException::INVALID_STATE_ERR,
            "The response type cannot be set if the object's state is LOADING "
            "or DONE.");
    }
    // If the JavaScript global environment is a document environment and the
    // synchronous flag is set, throw an "InvalidAccessError" exception.
    if (m_resourceRequest->isSync()) {
        throw new DOMException(
            scriptBindingInstance()->ownerDocument(),
            DOMException::INVALID_ACCESS_ERR,
            "Failed to set the 'responseType' property on 'XMLHttpRequest': "
            "The response type cannot be changed for synchronous requests made "
            "from a document.");
    }
    // TODO If the JavaScript global environment is a worker environment and the
    // given value is "document", terminate these steps.
    // Set the responseType attribute's value to the given value.
    m_responseType = type;
}

void XMLHttpRequest::setResponseType(String* typeStr)
{
    XMLHttpRequest::ResponseType type = Unspecified;

    if (typeStr->equals("arraybuffer")) {
        type = ArrayBuffer;
    } else if (typeStr->equals("blob")) {
        type = Blob;
    } else if (typeStr->equals("document")) {
        type = Document;
    } else if (typeStr->equals("json")) {
        type = Json;
    } else if (typeStr->equals("text")) {
        type = Text;
    } else {
        auto s = typeStr->toUTF8NonGCString();
        STARFISH_LOG_ERROR("setResponseType: Invalid value given: %s\n",
                           s.data());
    }

    setResponseType(type);
}

XMLHttpRequest::ResponseType XMLHttpRequest::responseTypeValue() const
{
    return m_responseType;
}

String* XMLHttpRequest::responseType() const
{
    switch (m_responseType) {
    case Unspecified:
        return String::emptyString;
    case ArrayBuffer:
        return String::createASCIIString("arraybuffer");
    case Blob:
        return String::createASCIIString("blob");
    case Document:
        return String::createASCIIString("document");
    case Json:
        return String::createASCIIString("json");
    case Text:
        return String::createASCIIString("text");
    }
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

uint8_t XMLHttpRequest::readyState() const
{
    return static_cast<uint8_t>(m_resourceRequest->readyState());
}

uint16_t XMLHttpRequest::status() const
{
    if (m_resourceRequest->readyState() == ReadyState::Unset ||
        m_resourceRequest->readyState() == ReadyState::Opened) {
        return 0;
    }
    return m_resourceRequest->status();
}

String* XMLHttpRequest::statusText() const
{
    if (m_resourceRequest->readyState() == ReadyState::Unset ||
        m_resourceRequest->readyState() == ReadyState::Opened) {
        return String::emptyString;
    }
    return httpStatusCodeToText(m_resourceRequest->status());
}

ScriptValue XMLHttpRequest::response() const
{
    ScriptValue result;

    if (m_responseType == ResponseType::Unspecified ||
        m_responseType == ResponseType::Text) {
        result = scriptStringToScriptValue(createScriptString(responseText()));
    } else if (m_responseType == ResponseType::Json) {
        result = m_responseJsonObject;
    } else if (m_responseType == ResponseType::Blob) {
        if (m_responseBlob) {
            result = m_responseBlob->scriptValue();
        } else {
            result = scriptNull();
        }
    } else if (m_responseType == ResponseType::ArrayBuffer) {
        result = m_responseArrayBuffer;
    } else if (m_responseType == ResponseType::Document) {
        result = m_responseXML->scriptValue();
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
#ifdef STARFISH_TC_COVERAGE
    STARFISH_LOG_INFO("&&&response\n");
#endif

    return result;
}

String* XMLHttpRequest::responseText() const
{
    if (!(m_responseType == ResponseType::Unspecified ||
          m_responseType == ResponseType::Text)) {
        throw new DOMException(
            const_cast<XMLHttpRequest*>(this)
                ->scriptBindingInstance()
                ->ownerDocument(),
            DOMException::INVALID_STATE_ERR,
            "Failed to read the 'responseText' property from 'XMLHttpRequest': "
            "The value is only accessible if the object's 'responseType' is '' "
            "or 'text'");
    }

#ifdef STARFISH_TC_COVERAGE
    STARFISH_LOG_INFO("&&&responseText\n");
#endif
    return m_responseText;
}

Document* XMLHttpRequest::responseXML() const
{
    if (!(m_responseType == ResponseType::Unspecified ||
          m_responseType == ResponseType::Document)) {
        throw new DOMException(
            const_cast<XMLHttpRequest*>(this)
                ->scriptBindingInstance()
                ->ownerDocument(),
            DOMException::INVALID_STATE_ERR,
            "Failed to read the 'responseXML' property from 'XMLHttpRequest': "
            "The value is only accessible if the object's 'responseType' is '' "
            "or 'document'");
    }

#ifdef STARFISH_TC_COVERAGE
    STARFISH_LOG_INFO("&&&responseXML\n");
#endif
    return m_responseXML;
}

uint32_t XMLHttpRequest::timeout() const
{
    return m_resourceRequest->timeout();
}

void XMLHttpRequest::setTimeout(uint32_t timeout)
{
    if (m_resourceRequest->isSync() == true) {
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::INVALID_ACCESS_ERR,
                               "InvalidAccessError");
    }
    m_resourceRequest->setTimeout(timeout);
}

bool XMLHttpRequest::withCredentials() const
{
    return m_withCredentials;
}

void XMLHttpRequest::setWithCredentials(bool value)
{
    if (m_resourceRequest->readyState() != ReadyState::Opened) {
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }
    m_withCredentials = value;
}

void XMLHttpRequest::setRequestHeader(String* header, String* value)
{
    header = header->trim();
    value = value->trim();
    if (m_resourceRequest->readyState() != ReadyState::Opened) {
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }
    if (header->length() == 0) {
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::SYNTAX_ERR, "InvalidStateError");
    }

    if (HTTPUtil::isUnsafeHeader(header)) {
        STARFISH_LOG_WARN("Refused to set unsafe header \"%s\"",
                          header->toUTF8NonGCString().data());
        return;
    }

    m_resourceRequest->setRequestHeader(header, value);
}

String* XMLHttpRequest::getAllResponseHeaders()
{
    if (readyState() < static_cast<uint8_t>(ReadyState::HeadersReceived) ||
        m_resourceRequest->isError()) {
        return String::emptyString;
    }

    StringBuilder sb;
    const HeaderMap& map = m_resourceRequest->responseHeaderMap();

    for (const auto& it : map) {
        const auto& key = it.first;
        const auto& value = it.second;

        if ((key.compare(HTTPHeaderMap::kSetCookie) == 0) ||
            (key.compare(HTTPHeaderMap::kSetCookie2) == 0)) {
            continue;
        }
        sb.appendString(key.c_str());
        sb.appendChar(':');
        sb.appendChar(' ');
        sb.appendString(value.c_str());
        sb.appendChar('\r');
        sb.appendChar('\n');
    }

    return sb.finalize();
}

Nullable<String*> XMLHttpRequest::getResponseHeader(String* name)
{
    if (readyState() < static_cast<uint8_t>(ReadyState::HeadersReceived) ||
        m_resourceRequest->isError()) {
        return nullptr;
    }
    if (name->length() == 0 || !name->containsOnlyASCIIChars() ||
        name->equalsIgnoreCase(HTTPHeaderMap::kSetCookie) ||
        name->equalsIgnoreCase(HTTPHeaderMap::kSetCookie2)) {
        return nullptr;
    }

    const HeaderMap& map = m_resourceRequest->responseHeaderMap();

    for (const auto& pair : map) {
        if (name->equalsIgnoreCase(pair.first.data())) {
            return String::createASCIIString(pair.second.data());
        }
    }

    return nullptr;
}
}
