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
#include <atomic>

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
    // Returns false when the peer exceeded the per-message receive limit; the
    // caller must then fail the connection instead of growing the buffer.
    bool addToRxBuffer(char* param, size_t size);
    void updateState(WebSocket::ReadyState state);

    // lws_cancel_service() is the only lws entry point that may be called from
    // a thread other than the one running lws_service(). Everything else has
    // to be deferred to LWS_CALLBACK_EVENT_WAIT_CANCELLED on the service
    // thread. See third_party/libwebsockets/READMEs/README.coding.md.
    void wakeService();

    WebSocket* parent()
    {
        return m_parent;
    }

    bool needsToClose()
    {
        return m_needsToClose.load(std::memory_order_acquire);
    }

    bool isConnected()
    {
        return m_isReady.load(std::memory_order_acquire);
    }

    std::string closeReason();

    size_t closeCode();

    void waitForWorkerEnd();

    uint64_t txBufferSize();

    static int lwsEventCallback(struct lws* wsi,
                                enum lws_callback_reasons reason, void* user,
                                void* in, size_t len);

    void ref()
    {
        m_refCount++;
    }

    void deref();

    // Upper bound on a single incoming message. Without it a peer that never
    // sets FIN can grow m_rxBuffer until the process is killed.
    static const size_t kMaxRxBufferSize = 16 * 1024 * 1024;
    // Upper bound on data queued by send() but not yet handed to lws.
    static const uint64_t kMaxTxBufferSize = 16 * 1024 * 1024;

private:
    static SocketLWS* fromContext(struct lws* wsi);
    void serviceTxQueueLocked(struct lws* wsi, bool* failed);

    // Written by the main thread, read by the lws service thread.
    std::atomic<bool> m_needsToClose;
    // Main thread only (constructor / waitForWorkerEnd).
    bool m_workerStarted;
    std::atomic<bool> m_alive;
    std::atomic<bool> m_isReady;

    std::atomic<unsigned> m_refCount;
    // Set once the service loop is running, so wakeService() can tell whether
    // it is already on that thread.
    std::atomic<unsigned long> m_serviceThreadId;

    IThread* m_thread{ nullptr };
    LWSRunnable* m_runnable{ nullptr };
    WebSocket* m_parent;
    std::string m_url;
    std::string m_urlPath;
    std::string m_protocol;
    std::string m_origin;

    lws_client_connect_info m_lwsClientConnectInfo;
    // Guarded by m_contextMutex: the service thread destroys the context in
    // finalize() while the main thread may be waking it in wakeService().
    lws_context* m_lwsContext;
    lws_protocols* m_lwsProtocols;
    lws_context_creation_info* m_lwsContextCreationInfo;

    // Service thread only. lws owns the wsi and frees it during context
    // teardown, so no other thread may hold or dereference it.
    lws* m_lwsClient;

    Mutex* m_txMutex;
    Mutex* m_contextMutex;
    Mutex* m_closeMutex;

    // Guarded by m_txMutex.
    std::vector<SocketLWSData*> m_txBuffer;
    uint64_t m_txBufferSize;
    // Service thread only.
    std::vector<char> m_rxBuffer;
    // Guarded by m_closeMutex.
    std::string m_closeReasonStr;
    size_t m_closeReasonCode;
};

} // namespace Starfish

#endif
