/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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
/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef STARFISH_ENABLE_WEBSOCKET
#include <EscargotPublic.h>
#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Event.h"
#include "core/page/WebBase.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptWrappable.h"
#include "core/dom/DOMException.h"
#include "core/modules/networking/WebSocket.h"
#include "core/fileapi/Blob.h"
#include "core/modules/networking/SocketLWS.h"
#include "core/dom/ExecutionContext.h"
#include "platform/loader/ResourceURL.h"
#include "core/dom/CloseEvent.h"

using namespace Escargot;

namespace Starfish {

// From blink source code :renderer/modules/websockets/dom_websocket.cc:181
// -->
static inline bool IsValidSubprotocolCharacter(char32_t character)
{
    const char32_t kMinimumProtocolCharacter = '!'; // U+0021.
    const char32_t kMaximumProtocolCharacter = '~'; // U+007E.
    // Set to true if character does not matches "separators" ABNF defined in
    // RFC2616. SP and HT are excluded since the range check excludes them.
    bool is_not_separator =
        character != '"' && character != '(' && character != ')' &&
        character != ',' && character != '/' &&
        !(character >= ':' &&
          character <=
              '@') // U+003A - U+0040 (':', ';', '<', '=', '>', '?', '@').
        && !(character >= '[' &&
             character <= ']') // U+005B - U+005D ('[', '\\', ']').
        && character != '{' && character != '}';
    return character >= kMinimumProtocolCharacter &&
           character <= kMaximumProtocolCharacter && is_not_separator;
}

static bool IsValidSubprotocolString(String* protocol)
{
    if (protocol->isEmpty())
        return false;
    for (size_t i = 0; i < protocol->length(); ++i) {
        if (!IsValidSubprotocolCharacter(protocol->charAt(i)))
            return false;
    }
    return true;
}
// <--

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
    , m_extensions(String::emptyString)
    , m_protocol(String::emptyString)
    , m_hasProtocol(false)
    , m_executionContext(executionContext)
{
    init(url, String::emptyString);
}

WebSocket::WebSocket(ExecutionContext* executionContext, String* url,
                     String* protocols)
    : EventTarget()
    , m_readyState(ReadyState::CONNECTING)
    , m_binaryType(BinaryType::Blob)
    , m_extensions(String::emptyString)
    , m_protocol(String::emptyString)
    , m_hasProtocol(true)
    , m_executionContext(executionContext)
{
    init(url, protocols);
}

void WebSocket::init(String* url, String* protocol)
{
    // https://html.spec.whatwg.org/multipage/web-sockets.html#dom-websocket

    // These failures happen before any socket exists, so they only throw: the
    // object never reaches script, and firing error/close events here would
    // both be unobservable and re-enter close() with a null m_socketLWS.
    m_url = new ResourceURL(url);
    // Let urlRecord be the result of applying the URL parser to url.
    // If urlRecord is failure, then throw a "SyntaxError" DOMException.
    if (!m_url->isValid()) {
        throw new DOMException(executionContext(), DOMException::SYNTAX_ERR,
                               "url's is not valid");
    }

    // If urlRecord's scheme is not "ws" or "wss", then throw a "SyntaxError"
    // DOMException.
    if (!m_url->isWSURL() && !m_url->isWSSURL()) {
        throw new DOMException(executionContext(), DOMException::SYNTAX_ERR,
                               "url's scheme is not \"ws\" or \"wss\"");
    }

    // If urlRecord's fragment is non-null, then throw a "SyntaxError"
    // DOMException.
    if (!m_url->hash()->equals(String::emptyString)) {
        throw new DOMException(executionContext(), DOMException::SYNTAX_ERR,
                               "url's fragment is non-null");
    }

    // If protocols is a string, set protocols to a sequence consisting of just
    // that string.
    // If any of the values in protocols occur more than once or otherwise fail
    // to match the requirements for elements that comprise the value of
    // Sec-WebSocket-Protocol fields as defined by The WebSocket protocol, then
    // throw a "SyntaxError" DOMException. [WSP]
    if (m_hasProtocol && !IsValidSubprotocolString(protocol)) {
        throw new DOMException(executionContext(), DOMException::SYNTAX_ERR,
                               "protocol has invalid value");
    }

    setProtocol(protocol);
    m_socketLWS = new SocketLWS(this);
    executionContext()->addActiveWebSockets(this);
}

String* WebSocket::url()
{
    return m_url->serialize();
}

ResourceURL* WebSocket::urlObject()
{
    return m_url;
}

uint64_t WebSocket::bufferedAmount()
{
    if (m_socketLWS) {
        return m_socketLWS->txBufferSize();
    }
    return 0;
}

void WebSocket::setReadyState(ReadyState state)
{
    m_readyState = state;
    if (state == ReadyState::CLOSED) {
        executionContext()->removeActiveWebSockets(this);
    }
}

void WebSocket::close()
{
    close(CloseCode::NormalClosure, String::emptyString);
}

void WebSocket::close(uint16_t code)
{
    close(code, String::emptyString);
}

void WebSocket::close(String* reason)
{
    close(CloseCode::NormalClosure, reason);
}

void WebSocket::close(uint16_t code, String* reason)
{
    // https://html.spec.whatwg.org/multipage/web-sockets.html#dom-websocket-close

    // If code is present, but is neither an integer equal to 1000 nor an
    // integer in the range 3000 to 4999, inclusive, throw an
    // "InvalidAccessError" DOMException.
    if (code != CloseCode::NormalClosure && (code < 3000 || code > 4999)) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_ACCESS_ERR,
                               "close code is not 1000 or in 3000-4999");
    }

    UTF8StringDataNonGCStd reasonString = reason->toUTF8NonGCString();
    // If reason is present, then its UTF-8 encoding must not be longer than
    // 123 bytes, otherwise throw a "SyntaxError" DOMException.
    if (reasonString.length() > 123) {
        throw new DOMException(executionContext(), DOMException::SYNTAX_ERR,
                               "close reason is longer than 123 bytes");
    }

    // If this's ready state is CLOSING or CLOSED, do nothing.
    if (m_readyState == ReadyState::CLOSING ||
        m_readyState == ReadyState::CLOSED) {
        return;
    }

    setReadyState(WebSocket::ReadyState::CLOSING);
    m_socketLWS->close(reasonString.c_str(), reasonString.length(), code);
}

DEFINE_EVENT_LISTENER(WebSocket, open);
DEFINE_EVENT_LISTENER(WebSocket, error);
DEFINE_EVENT_LISTENER(WebSocket, close);
DEFINE_EVENT_LISTENER(WebSocket, message);

void WebSocket::dispose()
{
    setReadyState(WebSocket::ReadyState::CLOSED);
    if (m_socketLWS) {
        m_socketLWS->close("", 0, CloseCode::NormalClosure);
        m_socketLWS->waitForWorkerEnd();
    }
}

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

    // If the readyState attribute is CLOSING or CLOSED, the data is discarded.
    // Queueing it instead would grow m_txBuffer without bound, because the
    // WRITEABLE callback that drains it never runs again.
    if (m_readyState == ReadyState::CLOSING ||
        m_readyState == ReadyState::CLOSED) {
        return;
    }

    m_socketLWS->send(buf, len, type);
}

void WebSocket::send(String* data)
{
    data->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            WebSocket* self = (WebSocket*)data;
            self->send(buf, len, 0);
            return 0;
        },
        this);
}

void WebSocket::send(Blob* data)
{
    send(data->data(), data->size(), 1);
}

void WebSocket::send(ScriptArrayBuffer data)
{
    if (!data->isDetachedBuffer()) {
        send(data->rawBuffer(), data->byteLength(), 1);
    }
}

void WebSocket::send(ScriptArrayBufferView data)
{
    if (data->buffer()->isArrayBufferObject() &&
        data->buffer()->asArrayBufferObject()->isDetachedBuffer()) {
        return;
    }
    send(data->rawBuffer(), data->byteLength(), 1);
}
} // namespace Starfish
#endif
