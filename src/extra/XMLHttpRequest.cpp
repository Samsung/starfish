/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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
#include "XMLHttpRequest.h"
#include "dom/Event.h"
#include "dom/DOMException.h"
#include "platform/window/Window.h"

namespace StarFish {

XMLHttpRequest::XMLHttpRequest(Document* document)
    : m_networkRequest(new NetworkRequest(document))
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
    m_responseJsonObject = ScriptValueNull;
    m_responseBlob = nullptr;
#ifdef USE_ES6_FEATURE
    m_responseArrayBuffer = ScriptValueNull;
#endif
}

void XMLHttpRequest::send(String* body)
{
    if (m_networkRequest->readyState() != NetworkRequest::OPENED) {
        throw new DOMException(m_networkRequest->starFish()->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "InvalidStateError");
    }
    m_networkRequest->send(body);
}

void XMLHttpRequest::open(NetworkRequest::MethodType method, String* url, bool async, String* userName, String* password)
{
    if (method == NetworkRequest::UNKNOWN_METHOD)
        throw new DOMException(m_networkRequest->starFish()->window()->scriptBindingInstance(), DOMException::SYNTAX_ERR, "SYNTAX_ERR");
    if (!async && m_networkRequest->timeout() != 0)
        throw new DOMException(m_networkRequest->starFish()->window()->scriptBindingInstance(), DOMException::INVALID_ACCESS_ERR, "InvalidAccessError");
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
    if (m_networkRequest->readyState() == NetworkRequest::LOADING || m_networkRequest->readyState() == NetworkRequest::DONE) {
        throw new DOMException(m_networkRequest->starFish()->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "The response type cannot be set if the object's state is LOADING or DONE.");
    }
    // If the JavaScript global environment is a document environment and the synchronous flag is set, throw an "InvalidAccessError" exception.
    if (/*isMainThread() &&*/m_networkRequest->isSync()) {
        throw new DOMException(m_networkRequest->starFish()->window()->scriptBindingInstance(), DOMException::INVALID_ACCESS_ERR, "Failed to set the 'responseType' property on 'XMLHttpRequest': The response type cannot be changed for synchronous requests made from a document.");
    }
    // TODO If the JavaScript global environment is a worker environment and the given value is "document", terminate these steps.
    // Set the responseType attribute's value to the given value.
    m_responseType = type;
}

XMLHttpRequest::ResponseType XMLHttpRequest::responseType()
{
    return m_responseType;
}

ScriptValue XMLHttpRequest::response()
{
    if (m_responseType == ResponseType::Unspecified || m_responseType == ResponseType::Text) {
        return createScriptString(responseText());
    } else if (m_responseType == ResponseType::Json) {
        return m_responseJsonObject;
    } else if (m_responseType == ResponseType::BlobType) {
        if (m_responseBlob) {
            return m_responseBlob->scriptValue();
        }
        return ScriptValueNull;
    } else if (m_responseType == ResponseType::ArrayBuffer) {
#ifdef USE_ES6_FEATURE
        return m_responseArrayBuffer;
#else
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
#endif
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

String* XMLHttpRequest::responseText()
{
    if (!(m_responseType == ResponseType::Unspecified || m_responseType == ResponseType::Text))
        throw new DOMException(m_networkRequest->starFish()->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "Failed to read the 'responseText' property from 'XMLHttpRequest': The value is only accessible if the object's 'responseType' is '' or 'text'");

    return m_responseText;
}

void XMLHttpRequest::setTimeout(uint32_t timeout)
{
    if (m_networkRequest->isSync() == true)
        throw new DOMException(m_networkRequest->starFish()->window()->scriptBindingInstance(), DOMException::INVALID_ACCESS_ERR, "InvalidAccessError");
    m_networkRequest->setTimeout(timeout);
}

void XMLHttpRequest::onProgressEvent(NetworkRequest* request, bool isExplicitAction)
{
    String* eventName = String::emptyString;
    NetworkRequest::ProgressState progState = request->progressState();
    if (progState == NetworkRequest::PROGRESS) {
        eventName = request->starFish()->staticStrings()->m_progress.localName();
    } else if (progState == NetworkRequest::ERROR) {
        eventName = request->starFish()->staticStrings()->m_error.localName();
        if (!m_networkRequest->url()->isFileURL() && !m_networkRequest->url()->isDataURL() && request->isSync()) {
            throw new DOMException(m_networkRequest->starFish()->window()->scriptBindingInstance(), DOMException::NETWORK_ERR, "NetworkError");
        }
    } else if (progState == NetworkRequest::ABORT) {
        if (isExplicitAction) {
            return;
        }
        eventName = request->starFish()->staticStrings()->m_abort.localName();
    } else if (progState == NetworkRequest::TIMEOUT)
        eventName = request->starFish()->staticStrings()->m_timeout.localName();
    else if (progState == NetworkRequest::LOAD) {
        eventName = request->starFish()->staticStrings()->m_load.localName();
    } else if (progState == NetworkRequest::LOADEND)
        eventName = request->starFish()->staticStrings()->m_loadend.localName();
    else if (progState == NetworkRequest::LOADSTART)
        eventName = request->starFish()->staticStrings()->m_loadstart.localName();
    else
        STARFISH_RELEASE_ASSERT_NOT_REACHED();

    ProgressEvent* pe = new ProgressEvent(eventName, ProgressEventInit(false, false, request->total() > 0, request->loaded(), request->total()));
    EventTarget::dispatchEvent(this, pe);
}

void XMLHttpRequest::onReadyStateChange(NetworkRequest* request, bool fromExplicit)
{
    if (fromExplicit) {

        if (request->readyState() == NetworkRequest::ReadyState::DONE) {
            if (m_responseType == ResponseType::Unspecified || m_responseType == ResponseType::Text) {
                TextConverter textConverter(m_networkRequest->mimeType(), String::fromUTF8("UTF-8"), m_networkRequest->responseData().data(), m_networkRequest->responseData().size());
                m_responseText = textConverter.convert(m_networkRequest->responseData().data(), m_networkRequest->responseData().size(), true);
                m_networkRequest->responseData().clear();
            } else if (m_responseType == ResponseType::Json) {
                TextConverter cvt(m_networkRequest->mimeType(), String::fromUTF8("UTF-8"), m_networkRequest->responseData().data(), m_networkRequest->responseData().size());
                String* text = cvt.convert(m_networkRequest->responseData().data(), m_networkRequest->responseData().size(), true);
                m_responseJsonObject = parseJSON(text);
            } else if (m_responseType == ResponseType::BlobType) {
                void* buffer = GC_MALLOC_ATOMIC(m_networkRequest->responseData().size());
                memcpy(buffer, m_networkRequest->responseData().data(), m_networkRequest->responseData().size());
                m_responseBlob = new Blob(m_networkRequest->starFish(), m_networkRequest->responseData().size(), m_networkRequest->mimeType(), buffer, false, false);
                m_networkRequest->responseData().clear();
                m_networkRequest->responseData().shrink_to_fit();
            } else if (m_responseType == ResponseType::ArrayBuffer) {
#ifdef USE_ES6_FEATURE
                void* buffer = GC_MALLOC_ATOMIC(m_networkRequest->responseData().size());
                memcpy(buffer, m_networkRequest->responseData().data(), m_networkRequest->responseData().size());
                m_responseArrayBuffer = createArrayBuffer(buffer, m_networkRequest->responseData().size());
                m_networkRequest->responseData().clear();
                m_networkRequest->responseData().shrink_to_fit();
#else
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
#endif
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }

        String* eventType = request->starFish()->staticStrings()->m_readystatechange.localName();
        Event* e = new Event(eventType, EventInit(true, true));
        EventTarget::dispatchEvent(this, e);
    }
}

}
