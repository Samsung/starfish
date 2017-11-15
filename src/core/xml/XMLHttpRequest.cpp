/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
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
        ResourceRequest::ProgressState progState = request->progressState();
        if (progState == ResourceRequest::PROGRESS) {
            eventName =
                request->starFish()->staticStrings()->m_progress.localName();
        } else if (progState == ResourceRequest::ERROR) {
            eventName =
                request->starFish()->staticStrings()->m_error.localName();
            if (!m_xhr->m_resourceRequest->url()->isFileURL() &&
                !m_xhr->m_resourceRequest->url()->isDataURL() &&
                request->isSync()) {
                throw new DOMException(
                    m_xhr->scriptBindingInstance()->ownerDocument(),
                    DOMException::NETWORK_ERR, "NetworkError");
            }
        } else if (progState == ResourceRequest::ABORT) {
            if (isExplicitAction) {
                return;
            }
            eventName =
                request->starFish()->staticStrings()->m_abort.localName();
        } else if (progState == ResourceRequest::TIMEOUT) {
            eventName =
                request->starFish()->staticStrings()->m_timeout.localName();
        } else if (progState == ResourceRequest::LOAD) {
            eventName =
                request->starFish()->staticStrings()->m_load.localName();
        } else if (progState == ResourceRequest::LOADEND) {
            eventName =
                request->starFish()->staticStrings()->m_loadend.localName();
        } else if (progState == ResourceRequest::LOADSTART) {
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
            if (request->readyState() == ResourceRequest::ReadyState::DONE) {
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
    if (m_resourceRequest->readyState() != ResourceRequest::OPENED) {
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }
    m_resourceRequest->send(body, false);
}

DEFINE_EVENT_LISTENER(XMLHttpRequest, readystatechange);

void XMLHttpRequest::open(String* method, String* url)
{
    open(ResourceRequest::toMethodType(method), url, true, String::emptyString,
         String::emptyString);
}

void XMLHttpRequest::open(String* method, String* url, bool async,
                          Nullable<String*> userName,
                          Nullable<String*> password)
{
    String* uValue =
        userName.hasValue() ? userName.getValue() : String::emptyString;
    String* pValue =
        password.hasValue() ? userName.getValue() : String::emptyString;
    open(ResourceRequest::toMethodType(method), url, async, uValue, pValue);
}

void XMLHttpRequest::open(ResourceRequest::MethodType method, String* url,
                          bool async, String* userName, String* password)
{
    if (method == ResourceRequest::UNKNOWN_METHOD) {
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::SYNTAX_ERR, "SYNTAX_ERR");
    }
    if (!async && m_resourceRequest->timeout() != 0) {
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::INVALID_ACCESS_ERR,
                               "InvalidAccessError");
    }
    m_resourceRequest->open(method, url, async, document()->documentURI(),
                            userName, password);
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
    if (m_resourceRequest->readyState() == ResourceRequest::LOADING ||
        m_resourceRequest->readyState() == ResourceRequest::DONE) {
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
    return m_resourceRequest->readyState();
}

uint16_t XMLHttpRequest::status() const
{
    if (m_resourceRequest->readyState() == ResourceRequest::UNSENT ||
        m_resourceRequest->readyState() == ResourceRequest::OPENED) {
        return 0;
    }
    return m_resourceRequest->status();
}

String* XMLHttpRequest::statusText() const
{
    if (m_resourceRequest->readyState() == ResourceRequest::UNSENT ||
        m_resourceRequest->readyState() == ResourceRequest::OPENED) {
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

void XMLHttpRequest::setRequestHeader(String* h, String* c)
{
    h = h->trim();
    c = c->trim();
    if (m_resourceRequest->readyState() != ResourceRequest::OPENED) {
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }
    if (h->length() == 0) {
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::SYNTAX_ERR, "InvalidStateError");
    }
    m_resourceRequest->setRequestHeader(h, c);
}

String* XMLHttpRequest::getAllResponseHeaders()
{
    if (readyState() < ResourceRequest::HEADERS_RECEIVED ||
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
    if (readyState() < ResourceRequest::HEADERS_RECEIVED ||
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
