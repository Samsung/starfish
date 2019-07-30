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

#ifndef __StarfishSocketLWS__
#define __StarfishSocketLWS__

#include "core/modules/networking/Socket.h"
#include <libwebsockets.h>

namespace Starfish {

class Thread;
class WebBase;
class SocketLWSData {
public:
    enum SocketLWSDataType { TEXT, BINARY };
    ~SocketLWSData()
    {
        free(m_buffer);
    }
    SocketLWSData(const char* buf, size_t size, SocketLWSDataType type);
    void* data()
    {
        return m_buffer;
    }
    size_t size()
    {
        return m_size;
    }
    SocketLWSDataType type()
    {
        return m_dataType;
    }

private:
    SocketLWSDataType m_dataType;
    size_t m_size;
    void* m_buffer;
};

class SocketLWS : public Socket {
public:
    enum LwsEvent { OPEN, ERROR, CLOSE, ONMESSAGE };
    class Exception : public Socket::Exception {
    public:
        Exception();
        const char* what() const throw() override;
    };
    SocketLWS(WebSocket* parent);
    virtual ~SocketLWS();

    int bind(const char* addr) override;
    int connect(const char* addr) override;
    int connect(String* url);
    int send(const void* buf, size_t len, int flags) override;
    int recv(void* buf, size_t len, int flags) override;
    int close() override;
    int getFd() override;
    int shutdown(int howto) override;
    void setsockopt(int level, int option, const void* optval,
                    size_t optvallen) override;
    void getsockopt(int level, int option, void* optval,
                    size_t* optvallen) override;
    short getEvents() override;

    void run();
    void publishEvent(LwsEvent eventType, char* param, size_t size);

    std::vector<SocketLWSData*>* data()
    {
        return &m_buffer;
    }
    WebSocket* parent()
    {
        return m_parent;
    }

private:
    bool m_active;
    Thread* m_thread;
    WebSocket* m_parent;
    UTF8StringDataNonGCStd m_url;

    lws_context_creation_info m_lwsContextCreationInfo;
    lws_client_connect_info m_lwsClientConnectInfo;
    lws_context* m_lwsContext;
    lws* m_lwsClient;

    std::vector<SocketLWSData*> m_buffer;
};

} // namespace Starfish

#endif
