/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishWebSocket__
#define __StarfishWebSocket__

#ifdef STARFISH_ENABLE_WEBSOCKET

#include "core/dom/EventTarget.h"
#include "core/modules/networking/BinaryType.h"

namespace Starfish {
class Blob;
class SocketLWS;

class WebSocket : public EventTarget {
public:
    enum ReadyState { CONNECTING, OPEN, CLOSING, CLOSED };
    WebSocket(ExecutionContext* executionContext, String* url);
    WebSocket(ExecutionContext* executionContext, String* url,
              String* protocols);

    String* url();

    // ready state
    uint16_t readyState()
    {
        return m_readyState;
    }
    void setReadyState(ReadyState state)
    {
        m_readyState = state;
    }
    uint64_t bufferedAmount();

    // networking
    String* extensions()
    {
        return m_extensions;
    }
    void setExtensions(String* extensions)
    {
        m_extensions = extensions;
    }
    String* protocol()
    {
        return m_protocol;
    }
    void setProtocol(String* protocol)
    {
        m_protocol = protocol;
    }

    void close();
    void close(uint16_t code);
    void close(String* reason);
    void close(uint16_t code, String* reason);

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(open);
    DECLARE_EVENT_LISTENER(error);
    DECLARE_EVENT_LISTENER(close);
    // messaging
    DECLARE_EVENT_LISTENER(message);
#undef VIRTUAL
#undef OVERRIDE

    String* binaryType();
    void setBinaryType(String* value);
    void send(String* data);
    void send(Blob* data);
    void send(ScriptArrayBuffer data);
    void send(ScriptArrayBufferView data);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isWebSocket() const;
    virtual ExecutionContext* executionContext() const override
    {
        return m_executionContext;
    }

private:
    void init(String* url, String* protocol);
    void send(const void* buf, size_t len, int flags);
    ReadyState m_readyState;
    BinaryType m_binaryType;
    String* m_url;
    String* m_extensions;
    String* m_protocol;

    ExecutionContext* m_executionContext;
    SocketLWS* m_socketLWS;
};
}
#endif
#endif
