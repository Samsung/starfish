/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_ENABLE_WEBSOCKET
#include <EscargotPublic.h>
#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Event.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptWrappable.h"
#include "core/dom/DOMException.h"
#include "core/modules/networking/WebSocket.h"
#include "core/fileapi/Blob.h"
#include "core/modules/networking/SocketLWS.h"
#include "core/dom/ExecutionContext.h"

using namespace Escargot;

namespace Starfish {

static inline String* binaryTypeToString(BinaryType binaryType)
{
    if (binaryType == BinaryType::Blob) {
        return String::createASCIIString("blob");
    } else if (binaryType == BinaryType::Arraybuffer) {
        return String::createASCIIString("arraybuffer");
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    return String::createASCIIString("blob");
}

static inline BinaryType stringToBinaryType(String* binaryType)
{
    if (binaryType->equals("blob", 4)) {
        return BinaryType::Blob;
    } else if (binaryType->equals("arraybuffer", 11)) {
        return BinaryType::Arraybuffer;
    }
    return BinaryType::Blob;
}

WebSocket::WebSocket(ExecutionContext* executionContext, String* url)
    : EventTarget()
    , m_readyState(ReadyState::CONNECTING)
    , m_binaryType(BinaryType::Blob)
    , m_url(String::emptyString)
    , m_extensions(String::emptyString)
    , m_protocol(String::emptyString)
    , m_executionContext(executionContext)
{
    init(url, nullptr);
}

WebSocket::WebSocket(ExecutionContext* executionContext, String* url,
                     String* protocols)
    : EventTarget()
    , m_readyState(ReadyState::CONNECTING)
    , m_binaryType(BinaryType::Blob)
    , m_url(String::emptyString)
    , m_extensions(String::emptyString)
    , m_protocol(String::emptyString)
    , m_executionContext(executionContext)
{
    init(url, protocols);
}

void WebSocket::init(String* url, String* protocol)
{
    // https://html.spec.whatwg.org/multipage/web-sockets.html#dom-websocket

    // Let urlRecord be the result of applying the URL parser to url.
    // If urlRecord is failure, then throw a "SyntaxError" DOMException.
    // TODO

    // If urlRecord's scheme is not "ws" or "wss", then throw a "SyntaxError"
    // DOMException.
    if (!url->startsWith("ws") && !url->startsWith("wss")) {
        close();
        throw new DOMException(executionContext(), DOMException::SYNTAX_ERR,
                               "url's scheme is not \"ws\" or \"wss\"");
    }

    // If urlRecord's fragment is non-null, then throw a "SyntaxError"
    // DOMException.
    // TODO

    // If protocols is a string, set protocols to a sequence consisting of just
    // that string.
    // If any of the values in protocols occur more than once or otherwise fail
    // to match the requirements for elements that comprise the value of
    // Sec-WebSocket-Protocol fields as defined by The WebSocket protocol, then
    // throw a "SyntaxError" DOMException. [WSP]
    // TODO
    m_url = url;
    m_socketLWS = new SocketLWS(this);
    m_socketLWS->connect(url);
}

String* WebSocket::url()
{
    return m_url;
}

uint64_t WebSocket::bufferedAmount()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return 0;
}

void WebSocket::close()
{
    close(0, String::emptyString);
}

void WebSocket::close(uint16_t code)
{
    close(code, String::emptyString);
}

void WebSocket::close(String* reason)
{
    close(0, reason);
}

void WebSocket::close(uint16_t code, String* reason)
{
    if (m_socketLWS) {
        setReadyState(WebSocket::ReadyState::CLOSING);
        m_socketLWS->close();
    } else {
        setReadyState(WebSocket::ReadyState::CLOSED);
        String* eventName = executionContext()
                                ->starfish()
                                ->staticStrings()
                                ->m_close.localName();
        Event* e = new Event(executionContext(), eventName);
        this->EventTarget::dispatchEventByUA(this, e);
    }
}

DEFINE_EVENT_LISTENER(WebSocket, open);
DEFINE_EVENT_LISTENER(WebSocket, error);
DEFINE_EVENT_LISTENER(WebSocket, close);
DEFINE_EVENT_LISTENER(WebSocket, message);

String* WebSocket::binaryType()
{
    return binaryTypeToString(m_binaryType);
}

void WebSocket::setBinaryType(String* value)
{
    m_binaryType = stringToBinaryType(value);
}

void WebSocket::send(const void* buf, size_t len, int type)
{
    // https://html.spec.whatwg.org/multipage/web-sockets.html#dom-websocket-send
    // The send(data) method transmits data using the connection. If the
    // readyState attribute is CONNECTING, it must throw an "InvalidStateError"
    // DOMException.
    if (m_readyState == ReadyState::CONNECTING) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "readyState attribute should not be CONNECTING");
    }

    m_socketLWS->send(buf, len, type);
}

void WebSocket::send(String* data)
{
    UTF8StringDataNonGCStd dataString = data->toUTF8NonGCString();
    send(dataString.c_str(), dataString.length(), 0);
}

void WebSocket::send(Blob* data)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void WebSocket::send(ScriptArrayBuffer data)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void WebSocket::send(ScriptArrayBufferView data)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}
}
#endif
