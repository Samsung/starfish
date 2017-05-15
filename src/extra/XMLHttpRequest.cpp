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

#include "StarFish.h"
#include "dom/Document.h"
#include "dom/DOMException.h"
#include "dom/ProgressEvent.h"
#include "extra/Blob.h"
#include "extra/XMLHttpRequest.h"
#include "platform/network/NetworkRequest.h"
#include "platform/window/Window.h"

namespace StarFish {

DEFINE_EVENT_LISTENER(XMLHttpRequestEventTarget, loadstart);
DEFINE_EVENT_LISTENER(XMLHttpRequestEventTarget, progress);
DEFINE_EVENT_LISTENER(XMLHttpRequestEventTarget, abort);
DEFINE_EVENT_LISTENER(XMLHttpRequestEventTarget, error);
DEFINE_EVENT_LISTENER(XMLHttpRequestEventTarget, load);
DEFINE_EVENT_LISTENER(XMLHttpRequestEventTarget, timeout);
DEFINE_EVENT_LISTENER(XMLHttpRequestEventTarget, loadend);

XMLHttpRequest::XMLHttpRequest(::StarFish::Document* document)
    : XMLHttpRequestEventTarget(document)
    , m_networkRequest(new NetworkRequest(document))
{
    /*
    GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
        STARFISH_LOG_INFO("XMLHttpRequest::~XMLHttpRequest\n");
    }, NULL, NULL, NULL);
    */

    m_responseType = ResponseType::Unspecified;
    initResponseData();
    m_networkRequest->addNetworkRequestClient(this);
}

void XMLHttpRequest::initResponseData()
{
    m_responseText = String::emptyString;
    m_responseJsonObject = ESValue(ESValue::ESNull);
    m_responseBlob = nullptr;
#ifdef USE_ES6_FEATURE
    m_responseArrayBuffer = ESValue(ESValue::ESNull);
#endif
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
    if (m_networkRequest->readyState() != NetworkRequest::OPENED) {
        throw new DOMException(DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }
    m_networkRequest->send(body);
}

static NetworkRequest::MethodType toMethodType(String* input)
{
    String* lowerMethod = input->toLower();
    if (lowerMethod->equals("post")) {
        return NetworkRequest::POST_METHOD;
    } else if (lowerMethod->equals("get")) {
        return NetworkRequest::GET_METHOD;
    }
    return NetworkRequest::UNKNOWN_METHOD;
}

DEFINE_EVENT_LISTENER(XMLHttpRequest, readystatechange);

void XMLHttpRequest::open(String* method, String* url)
{
    open(toMethodType(method), url, true, String::emptyString,
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
    open(toMethodType(method), url, async, uValue, pValue);
}

void XMLHttpRequest::open(NetworkRequest::MethodType method, String* url,
                          bool async, String* userName, String* password)
{
    if (method == NetworkRequest::UNKNOWN_METHOD) {
        throw new DOMException(DOMException::SYNTAX_ERR, "SYNTAX_ERR");
    }
    if (!async && m_networkRequest->timeout() != 0) {
        throw new DOMException(DOMException::INVALID_ACCESS_ERR,
                               "InvalidAccessError");
    }
    m_networkRequest->open(method, url, async, userName, password);
    initResponseData();
}

void XMLHttpRequest::abort()
{
    m_networkRequest->abort();
    initResponseData();
}

void XMLHttpRequest::setResponseType(ResponseType type)
{
    // If the state is LOADING or DONE, throw an "InvalidStateError" exception.
    if (m_networkRequest->readyState() == NetworkRequest::LOADING ||
        m_networkRequest->readyState() == NetworkRequest::DONE) {
        throw new DOMException(
            DOMException::INVALID_STATE_ERR,
            "The response type cannot be set if the object's state is LOADING "
            "or DONE.");
    }
    // If the JavaScript global environment is a document environment and the
    // synchronous flag is set, throw an "InvalidAccessError" exception.
    if (/*isMainThread() &&*/ m_networkRequest->isSync()) {
        throw new DOMException(
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
#ifdef USE_ES6_FEATURE
        type = ArrayBuffer;
#endif
    } else if (typeStr->equals("blob")) {
        type = Blob;
    } else if (typeStr->equals("document")) {
        type = Document;
    } else if (typeStr->equals("json")) {
        type = Json;
    } else if (typeStr->equals("text")) {
        type = Text;
    } else {
        STARFISH_LOG_ERROR(
            "The provided value '%s' is not a valid enum value of "
            "type XMLHttpRequestResponseType.",
            typeStr->utf8Data());
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
    return m_networkRequest->readyState();
}

uint16_t XMLHttpRequest::status() const
{
    return m_networkRequest->status();
}

ScriptValue XMLHttpRequest::response() const
{
    ScriptValue result;

    if (m_responseType == ResponseType::Unspecified ||
        m_responseType == ResponseType::Text) {
        result = createScriptString(responseText());
    } else if (m_responseType == ResponseType::Json) {
        result = m_responseJsonObject;
    } else if (m_responseType == ResponseType::Blob) {
        if (m_responseBlob) {
            result = m_responseBlob->scriptValue();
        } else {
            result = ESValue(ESValue::ESNull);
        }
    } else if (m_responseType == ResponseType::ArrayBuffer) {
#ifdef USE_ES6_FEATURE
        result = m_responseArrayBuffer;
#else
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
#endif
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
    return m_networkRequest->timeout();
}

void XMLHttpRequest::setTimeout(uint32_t timeout)
{
    if (m_networkRequest->isSync() == true) {
        throw new DOMException(DOMException::INVALID_ACCESS_ERR,
                               "InvalidAccessError");
    }
    m_networkRequest->setTimeout(timeout);
}

void XMLHttpRequest::setRequestHeader(String* h, String* c)
{
    h = h->trim();
    c = c->trim();
    if (m_networkRequest->readyState() != NetworkRequest::OPENED) {
        throw new DOMException(DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }
    if (h->length() == 0) {
        throw new DOMException(DOMException::SYNTAX_ERR, "InvalidStateError");
    }
    m_networkRequest->setRequestHeader(h, c);
}

void XMLHttpRequest::onProgressEvent(NetworkRequest* request,
                                     bool isExplicitAction)
{
    String* eventName = String::emptyString;
    NetworkRequest::ProgressState progState = request->progressState();
    if (progState == NetworkRequest::PROGRESS) {
        eventName =
            request->starFish()->staticStrings()->m_progress.localName();
    } else if (progState == NetworkRequest::ERROR) {
        eventName = request->starFish()->staticStrings()->m_error.localName();
        if (!m_networkRequest->url()->isFileURL() &&
            !m_networkRequest->url()->isDataURL() && request->isSync()) {
            throw new DOMException(DOMException::NETWORK_ERR, "NetworkError");
        }
    } else if (progState == NetworkRequest::ABORT) {
        if (isExplicitAction) {
            return;
        }
        eventName = request->starFish()->staticStrings()->m_abort.localName();
    } else if (progState == NetworkRequest::TIMEOUT) {
        eventName = request->starFish()->staticStrings()->m_timeout.localName();
    } else if (progState == NetworkRequest::LOAD) {
        eventName = request->starFish()->staticStrings()->m_load.localName();
    } else if (progState == NetworkRequest::LOADEND) {
        eventName = request->starFish()->staticStrings()->m_loadend.localName();
    } else if (progState == NetworkRequest::LOADSTART) {
        eventName =
            request->starFish()->staticStrings()->m_loadstart.localName();
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    ProgressEvent* pe = new ProgressEvent(
        eventName, ProgressEventInit(false, false, request->total() > 0,
                                     request->loaded(), request->total()));
    EventTarget::dispatchEvent(this, pe);
}

void XMLHttpRequest::onReadyStateChange(NetworkRequest* request,
                                        bool fromExplicit)
{
    if (fromExplicit) {
        if (request->readyState() == NetworkRequest::ReadyState::DONE) {
            if (m_responseType == ResponseType::Unspecified ||
                m_responseType == ResponseType::Text) {
                TextConverter textConverter(
                    m_networkRequest->responseMimeType(),
                    String::fromUTF8("UTF-8"),
                    m_networkRequest->response().data(),
                    m_networkRequest->response().size());
                m_responseText = textConverter.convert(
                    m_networkRequest->response().data(),
                    m_networkRequest->response().size(), true);
                m_networkRequest->response().clear();
            } else if (m_responseType == ResponseType::Json) {
                TextConverter cvt(m_networkRequest->responseMimeType(),
                                  String::fromUTF8("UTF-8"),
                                  m_networkRequest->response().data(),
                                  m_networkRequest->response().size());
                String* text =
                    cvt.convert(m_networkRequest->response().data(),
                                m_networkRequest->response().size(), true);
                m_responseJsonObject = parseJSON(text);
            } else if (m_responseType == ResponseType::Blob) {
                void* buffer = GC_MALLOC_ATOMIC_IGNORE_OFF_PAGE(
                    m_networkRequest->response().size());
                memcpy(buffer, m_networkRequest->response().data(),
                       m_networkRequest->response().size());
                m_responseBlob = new ::StarFish::Blob(
                    m_networkRequest->starFish(),
                    m_networkRequest->response().size(),
                    m_networkRequest->responseMimeType(), buffer, false, false);
                m_networkRequest->response().clear();
                m_networkRequest->response().shrink_to_fit();
            } else if (m_responseType == ResponseType::ArrayBuffer) {
#ifdef USE_ES6_FEATURE
                void* buffer = GC_MALLOC_ATOMIC_IGNORE_OFF_PAGE(
                    m_networkRequest->response().size());
                memcpy(buffer, m_networkRequest->response().data(),
                       m_networkRequest->response().size());
                m_responseArrayBuffer = createArrayBuffer(
                    buffer, m_networkRequest->response().size());
                m_networkRequest->response().clear();
                m_networkRequest->response().shrink_to_fit();
#else
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
#endif
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }

        String* eventType = request->starFish()
                                ->staticStrings()
                                ->m_readystatechange.localName();
        Event* e = new Event(eventType, EventInit(true, true));
        EventTarget::dispatchEvent(this, e);
    }
}
}
