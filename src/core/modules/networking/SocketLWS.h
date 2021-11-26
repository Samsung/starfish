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

#ifndef __StarfishSocketLWS__
#define __StarfishSocketLWS__

#include "core/modules/networking/Socket.h"
#include <libwebsockets.h>

namespace Starfish {

class IThread;
class IRunnable;
class WebBase;
class LWSRunnable;
class Mutex;

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
    friend class LWSRunnable;
    enum LwsEvent { OPEN, ERROR, CLOSE, ONMESSAGE };
    class Exception : public Socket::Exception {
    public:
        Exception();
        const char* what() const throw() override;
    };
    SocketLWS(WebSocket* parent);
    virtual ~SocketLWS();
    void finalize();

    int bind(const char* addr) override;
    int connect(const char* addr) override;
    int send(const void* buf, size_t len, int flags) override;
    int recv(void* buf, size_t len, int flags) override;
    int close() override;
    void close(const char* ptr, size_t len, size_t code);
    int getFd() override;
    int shutdown(int howto) override;
    void setsockopt(int level, int option, const void* optval,
                    size_t optvallen) override;
    void getsockopt(int level, int option, void* optval,
                    size_t* optvallen) override;
    short getEvents() override;

    void run();
    void publishEvent(LwsEvent eventType, bool isBinary = false);
    void addToRxBuffer(char* param, size_t size);
    void updateState(WebSocket::ReadyState state);

    WebSocket* parent()
    {
        return m_parent;
    }

    bool needsToClose()
    {
        return m_needsToClose;
    }

    bool isConnected()
    {
        return m_isReady;
    }

    std::string closeReason()
    {
        return m_closeReasonStr;
    }

    size_t closeCode()
    {
        return m_closeReasonCode;
    }

    uint64_t txBufferSize();

    static int lwsEventCallback(struct lws* wsi,
                                enum lws_callback_reasons reason, void* user,
                                void* in, size_t len);

private:
    bool m_needsToClose;
    bool m_workerStarted;
    bool m_alive;
    bool m_isReady;
    IThread* m_thread{ nullptr };
    LWSRunnable* m_runnable{ nullptr };
    WebSocket* m_parent;
    UTF8StringDataNonGCStd m_url;
    UTF8StringDataNonGCStd m_urlPath;
    UTF8StringDataNonGCStd m_protocol;

    lws_client_connect_info m_lwsClientConnectInfo;
    lws_context* m_lwsContext;
    lws* m_lwsClient;

    Mutex* m_txMutex;

    std::vector<SocketLWSData*> m_txBuffer;
    std::vector<char> m_rxBuffer;
    std::string m_closeReasonStr;
    size_t m_closeReasonCode;
    uint64_t m_txBufferSize;
};

} // namespace Starfish

#endif
