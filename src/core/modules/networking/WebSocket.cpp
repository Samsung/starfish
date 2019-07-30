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
#include "core/modules/networking/BinaryType.h"
#include "core/fileapi/Blob.h"
#include "core/modules/networking/SocketLWS.h"
#include "core/dom/ExecutionContext.h"

using namespace Escargot;

namespace Starfish {
WebSocket::WebSocket(ExecutionContext* executionContext, String* url)
    : EventTarget()
    , m_executionContext(executionContext)
{
    init(url, nullptr);
}

WebSocket::WebSocket(ExecutionContext* executionContext, String* url,
                     String* protocols)
    : EventTarget()
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
        throw new DOMException(executionContext(), DOMException::SYNTAX_ERR,
                               "urlRecord's scheme is not \"ws\" or \"wss\"");
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

    m_socketLWS = new SocketLWS(this);
    m_socketLWS->connect(url);
}

String* WebSocket::url()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
}

void WebSocket::setUrl(String* url)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

uint16_t WebSocket::readyState()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return 0;
}

uint64_t WebSocket::bufferedAmount()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return 0;
}

String* WebSocket::extensions()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
}

String* WebSocket::protocol()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
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
    m_socketLWS->close();
}

DEFINE_EVENT_LISTENER(WebSocket, open);
DEFINE_EVENT_LISTENER(WebSocket, error);
DEFINE_EVENT_LISTENER(WebSocket, close);
DEFINE_EVENT_LISTENER(WebSocket, message);

String* WebSocket::binaryType()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
}

void WebSocket::setBinaryType(String* value)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}

void WebSocket::send(String* data)
{
    UTF8StringDataNonGCStd dataString = data->toUTF8NonGCString();
    m_socketLWS->send(dataString.c_str(), dataString.length(), 0);
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
