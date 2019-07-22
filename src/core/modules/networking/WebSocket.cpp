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
#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Event.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptWrappable.h"
#include "core/dom/DOMException.h"
#include "core/modules/networking/WebSocket.h"
#include "core/modules/networking/BinaryType.h"
#include "core/fileapi/Blob.h"

#include <EscargotPublic.h>
using namespace Escargot;

namespace Starfish {
WebSocket::WebSocket(ExecutionContext* executionContext, String* url)
    : m_executionContext(executionContext)
{
}

WebSocket::WebSocket(ExecutionContext* executionContext, String* url,
                     String* protocols)
    : m_executionContext(executionContext)
{
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
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
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
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
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
