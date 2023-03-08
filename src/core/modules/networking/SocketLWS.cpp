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

#ifdef STARFISH_ENABLE_WEBSOCKET

#include "StarfishConfig.h"
#include "Starfish.h"
#include "PlatformIntegrationData.h"
#include <EscargotPublic.h>
#include "core/modules/networking/WebSocket.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/networking/SocketLWS.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/threading/Mutex.h"
#include "core/modules/threading/Locker.h"
#include "core/page/WebBase.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/dom/Event.h"
#include "core/dom/CloseEvent.h"
#include "core/dom/MessageEvent.h"
#include "core/dom/EventTarget.h"
#include "core/dom/Document.h"
#include "core/modules/threading/AdaptedThread.h"
#include "core/modules/networking/LWSRunnable.h"
#include "core/fileapi/Blob.h"

namespace Starfish {

SocketLWS::Exception::Exception()
    : Socket::Exception::Exception(0)
{
}

#if defined(STARFISH_TIZEN)
static const char* SocketLWSDefaultCertPath =
    "/opt/share/cert-svc/ca-certificate.crt";
#else
static const char* SocketLWSDefaultCertPath =
    "/etc/ssl/certs/ca-certificates.crt";
#endif

#define CHECK_ALIVE() \
    if (!m_alive) {   \
        return;       \
    }

int SocketLWS::lwsEventCallback(struct lws* wsi,
                                enum lws_callback_reasons reason, void* user,
                                void* in, size_t len)
{
    SocketLWS* socket = (SocketLWS*)user;

    switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        socket->updateState(WebSocket::ReadyState::OPEN);
        socket->publishEvent(SocketLWS::LwsEvent::OPEN);
        lws_callback_on_writable(wsi);
        break;

    case LWS_CALLBACK_CLIENT_RECEIVE:

        socket->addToRxBuffer((char*)in, len);
        if (lws_is_final_fragment(wsi)) {
            socket->publishEvent(SocketLWS::LwsEvent::ONMESSAGE,
                                 lws_frame_is_binary(wsi));
        }
        break;

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        // TODO
        socket->publishEvent(SocketLWS::LwsEvent::ERROR);
        // If connection is not establised, LWS doesn't call close
        // LWS_CALLBACK_CLOSED.
        if (!socket->isConnected()) {
            socket->close(nullptr, 0, WebSocket::CloseCode::AbnormalClosure);
            socket->updateState(WebSocket::ReadyState::CLOSED);
            socket->publishEvent(SocketLWS::LwsEvent::CLOSE);
            socket->shutdown(0);
        }
        break;

    case LWS_CALLBACK_CLIENT_WRITEABLE: {
        if (socket->needsToClose()) {
            std::string reason = socket->closeReason();
            lws_close_status code = (lws_close_status)socket->closeCode();
            lws_close_reason(wsi, code, (unsigned char*)reason.c_str(),
                             reason.length());
            return -1;
        }

        {
            Locker<Mutex> l(*socket->m_txMutex);
            std::vector<SocketLWSData*>* buffer = &socket->m_txBuffer;
            if (!buffer->empty()) {
                auto iter = buffer->begin();
                SocketLWSData* data = (SocketLWSData*)*iter;
                if (data->type() == SocketLWSData::SocketLWSDataType::TEXT) {
                    lws_write(wsi, ((unsigned char*)data->data()) + LWS_PRE,
                              data->size(), LWS_WRITE_TEXT);
                } else {
                    lws_write(wsi, ((unsigned char*)data->data()) + LWS_PRE,
                              data->size(), LWS_WRITE_BINARY);
                }
                size_t siz = data->size();
                iter = buffer->erase(iter);
                delete data;

                WebBase* webBase =
                    socket->parent()->executionContext()->webBase();
                socket->ref();
                webBase->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
                    nullptr,
                    [](size_t handle, void* data, void* data1) {
                        SocketLWS* socket = (SocketLWS*)data;
                        socket->deref();
                        size_t siz = (size_t)data1;
                        socket->m_txBufferSize -= siz;
                    },
                    socket, (void*)siz);
            }
            if (!buffer->empty()) {
                lws_callback_on_writable(wsi);
            }
        }
        break;
    }

    case LWS_CALLBACK_CLIENT_CLOSED:
    case LWS_CALLBACK_CLOSED:
        socket->updateState(WebSocket::ReadyState::CLOSED);
        socket->publishEvent(SocketLWS::LwsEvent::CLOSE);
        socket->shutdown(0);
        break;

    default:
        break;
    }

    return 0;
}

const char* SocketLWS::Exception::what() const throw()
{
    STARFISH_ASSERT_NOT_REACHED();
    return nullptr;
}

SocketLWSData::SocketLWSData(const char* buf, size_t size,
                             SocketLWSDataType type)
{
    m_dataType = type;
    m_size = size;
    m_buffer = malloc(LWS_PRE + size);
    memcpy(((char*)m_buffer) + LWS_PRE, buf, size);
}

SocketLWS::SocketLWS(WebSocket* socket)
    : m_needsToClose(false)
    , m_workerStarted(false)
    , m_alive(true)
    , m_isReady(false)
    , m_refCount(0)
    , m_parent(socket)
    , m_lwsContext(nullptr)
    , m_lwsProtocols(nullptr)
    , m_lwsContextCreationInfo(nullptr)
    , m_lwsClient(nullptr)
    , m_txMutex(new Mutex())
    , m_closeReasonStr(std::string())
    , m_closeReasonCode(WebSocket::CloseCode::NoStatusReceived)
    , m_txBufferSize(0)
{
    int logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO |
               LLL_PARSER | LLL_HEADER | LLL_EXT | LLL_CLIENT | LLL_LATENCY |
               LLL_DEBUG | LLL_THREAD;

    // lws_set_log_level(logs, NULL);

    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj, void* cd) { ((SocketLWS*)obj)->~SocketLWS(); },
        NULL, NULL, NULL);

    int useSSL = 0;
    const char* prot;
    m_url = parent()->url()->toUTF8NonGCString();
    m_protocol = parent()->protocol()->toUTF8NonGCString();
    m_origin = parent()->urlObject()->origin()->toUTF8NonGCString();
    char* param = (char*)m_url.c_str();
    if (lws_parse_uri(param, &prot, &m_lwsClientConnectInfo.address,
                      &m_lwsClientConnectInfo.port,
                      &m_lwsClientConnectInfo.path)) {
        // TODO
        m_alive = false;
        return;
    }

    // add back the leading / on path
    if (m_lwsClientConnectInfo.path[0] != '/') {
        m_urlPath = UTF8StringDataNonGCStd(m_lwsClientConnectInfo.path);
        m_urlPath.insert(0, "/");
        m_lwsClientConnectInfo.path = m_urlPath.c_str();
    }

    WebBase* webBase = parent()->executionContext()->webBase();
    m_thread = new AdaptedThread(webBase->threadPool());
    m_runnable = new LWSRunnable(webBase->messageLoop(), this);

    if (!strcmp(prot, "https") || !strcmp(prot, "wss")) {
        useSSL = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED |
                 LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
    }

    m_lwsProtocols = new lws_protocols[2];
    m_lwsProtocols[0] = {
        m_protocol.data(), SocketLWS::lwsEventCallback, 0, 0, 0, NULL, 0
    };
    m_lwsProtocols[1] = { nullptr, nullptr, 0, 0, 0, NULL, 0 };

    m_lwsContextCreationInfo = new lws_context_creation_info();
    m_lwsContextCreationInfo->port = CONTEXT_PORT_NO_LISTEN;
    m_lwsContextCreationInfo->protocols = m_lwsProtocols;
    m_lwsContextCreationInfo->options = 0;

    static bool sslInited = false;
    if (!sslInited && useSSL) {
        m_lwsContextCreationInfo->options |=
            LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        m_lwsContextCreationInfo->options |= LWS_SERVER_OPTION_UNIX_SOCK;
        sslInited = true;
    }

    if (useSSL) {
        m_lwsContextCreationInfo->client_ssl_ca_filepath =
            SocketLWSDefaultCertPath;
    }
    if (webBase->getWebSecurityMode() == LWE::WebSecurityMode::Disable) {
        useSSL = useSSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_ALLOW_INSECURE |
                 LCCSCF_PIPELINE;
    }

    m_lwsContext = lws_create_context(m_lwsContextCreationInfo);

    m_lwsClientConnectInfo.context = m_lwsContext;
    m_lwsClientConnectInfo.host = m_lwsClientConnectInfo.address;
    m_lwsClientConnectInfo.origin = m_origin.c_str();

    if (m_protocol.size() != 0) {
        m_lwsClientConnectInfo.protocol = m_protocol.data();
    }

    m_lwsClientConnectInfo.ssl_connection = useSSL;
    m_lwsClientConnectInfo.userdata = this;
    m_lwsClientConnectInfo.pwsi = &m_lwsClient;

    m_workerStarted = true;
    m_thread->start(m_runnable);

    ref();
    parent()->executionContext()->webBase()->starfish()->addPointerInRootSet(
        this);

    STARFISH_LOG_INFO("%p SocketLWS::SocketLWS called", this);
}

SocketLWS::~SocketLWS()
{
    waitForWorkerEnd();
    delete[] m_lwsProtocols;
    delete m_lwsContextCreationInfo;

    STARFISH_LOG_INFO("%p SocketLWS::~SocketLWS called", this);
}

void SocketLWS::deref()
{
    STARFISH_RELEASE_ASSERT(m_refCount >= 1);
    if (m_refCount.fetch_sub(1) == 1) {
        parent()
            ->executionContext()
            ->webBase()
            ->starfish()
            ->removePointerFromRootSet(this);
    }
}

void SocketLWS::finalize()
{
    m_alive = false;
    if (m_lwsContext != nullptr) {
        lws_context_destroy(m_lwsContext);
        m_lwsContext = nullptr;
        m_lwsClient = nullptr;
    }
}

void SocketLWS::run()
{
    if (m_lwsContext) {
        lws_service(m_lwsContext, 0);
    }
}

void SocketLWS::setsockopt(int level, int option, const void* optval,
                           size_t optvallen)
{
    STARFISH_ASSERT_NOT_REACHED();
}

void SocketLWS::getsockopt(int level, int option, void* optval,
                           size_t* optvallen)
{
    STARFISH_ASSERT_NOT_REACHED();
}

int SocketLWS::bind(const char* addr)
{
    STARFISH_ASSERT_NOT_REACHED();
    return 0;
}

void SocketLWS::close(const char* ptr, size_t len, size_t code)
{
    m_closeReasonStr = std::string(ptr, len);
    m_closeReasonCode = code;
    if (!m_needsToClose) {
        m_needsToClose = true;
        if (m_lwsClient && m_isReady) {
            lws_callback_on_writable(m_lwsClient);
        }
    }
}

int SocketLWS::close()
{
    STARFISH_ASSERT_NOT_REACHED();
    return 0;
}

int SocketLWS::connect(const char* addr)
{
    STARFISH_ASSERT_NOT_REACHED();
    return 0;
}

int SocketLWS::shutdown(int howto)
{
    m_alive = false;
    // if there was error on lws_client_connect_via_info, there is no started
    // runner
    if (m_workerStarted) {
        m_thread->stop();
    }
    return 0;
}

int SocketLWS::send(const void* buf, size_t len, int flags)
{
    SocketLWSData::SocketLWSDataType type =
        SocketLWSData::SocketLWSDataType::TEXT;
    if (flags != 0) {
        type = SocketLWSData::SocketLWSDataType::BINARY;
    }

    {
        Locker<Mutex> l(*m_txMutex);
        SocketLWSData* newData = new SocketLWSData((char*)buf, len, type);
        m_txBuffer.push_back(newData);
        m_txBufferSize += len;
    }

    if (m_lwsClient) {
        lws_callback_on_writable(m_lwsClient);
    }
    return 0;
}

int SocketLWS::recv(void* buf, size_t len, int flags)
{
    STARFISH_ASSERT_NOT_REACHED();
    return 0;
}

int SocketLWS::getFd()
{
    STARFISH_ASSERT_NOT_REACHED();
    return 0;
};

short SocketLWS::getEvents()
{
    STARFISH_ASSERT_NOT_REACHED();
    return 0;
}

void SocketLWS::addToRxBuffer(char* param, size_t size)
{
    m_rxBuffer.insert(m_rxBuffer.end(), param, param + size);
}

void SocketLWS::waitForWorkerEnd()
{
    if (m_workerStarted) {
        m_thread->join();
        m_workerStarted = false;
    }
}

uint64_t SocketLWS::txBufferSize()
{
    Locker<Mutex> l(*m_txMutex);
    return m_txBufferSize;
}

void SocketLWS::updateState(WebSocket::ReadyState state)
{
    CHECK_ALIVE()

    WebBase* webBase = parent()->executionContext()->webBase();

    switch (state) {
    case WebSocket::ReadyState::OPEN: {
        m_isReady = true;
        ref();
        webBase->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
            nullptr,
            [](size_t handle, void* data) {
                SocketLWS* socket = (SocketLWS*)data;
                socket->deref();
                socket->parent()->setReadyState(WebSocket::ReadyState::OPEN);
            },
            this);
        break;
    }
    case WebSocket::ReadyState::CLOSED: {
        m_isReady = false;
        // if there was error, the callback is fired by main thread
        auto fn = [](size_t handle, void* data) {
            SocketLWS* socket = (SocketLWS*)data;
            socket->deref();
            socket->parent()->setReadyState(WebSocket::ReadyState::CLOSED);
        };
        ref();
        if (isMainThread()) {
            webBase->messageLoop()->addIdler(nullptr, fn, this);
        } else {
            webBase->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
                nullptr, fn, this);
        }
        break;
    }
    default: {
        break;
    }
    }
}

void SocketLWS::publishEvent(LwsEvent eventType, bool isBinary)
{
    CHECK_ALIVE()

    WebBase* webBase = parent()->executionContext()->webBase();

    switch (eventType) {
    case LwsEvent::OPEN: {
        ref();
        webBase->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
            nullptr,
            [](size_t handle, void* data) {
                SocketLWS* lws = (SocketLWS*)data;
                lws->deref();
                WebSocket* socket = lws->parent();
                String* eventName = socket->executionContext()
                                        ->starfish()
                                        ->staticStrings()
                                        ->m_open.localName();
                Event* e = new Event(socket->executionContext(), eventName);
                socket->EventTarget::dispatchEventByUA(socket, e);
            },
            this);
    } break;
    case LwsEvent::ERROR: {
        // if address is wrong, the callback is fired by main thread
        auto fn = [](size_t handle, void* data) {
            SocketLWS* lws = (SocketLWS*)data;
            lws->deref();
            WebSocket* socket = lws->parent();
            String* eventName = socket->executionContext()
                                    ->starfish()
                                    ->staticStrings()
                                    ->m_error.localName();
            Event* e = new Event(socket->executionContext(), eventName);
            socket->EventTarget::dispatchEventByUA(socket, e);
        };
        ref();
        if (isMainThread()) {
            webBase->messageLoop()->addIdler(nullptr, fn, this);
        } else {
            webBase->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
                nullptr, fn, this);
        }
    } break;
    case LwsEvent::CLOSE: {
        // if there was error, the callback is fired by main thread
        auto fn = [](size_t handle, void* data) {
            SocketLWS* lws = (SocketLWS*)data;
            lws->deref();
            WebSocket* socket = lws->parent();
            String* eventName = socket->executionContext()
                                    ->starfish()
                                    ->staticStrings()
                                    ->m_close.localName();
            CloseEvent* e =
                new CloseEvent(socket->executionContext(), eventName);
            if (lws->closeCode() == WebSocket::CloseCode::NormalClosure) {
                e->setWasClean(true);
            }
            e->setCode(lws->closeCode());
            e->setReason(String::createASCIIString(
                lws->closeReason().c_str(), lws->closeReason().length()));
            socket->EventTarget::dispatchEventByUA(socket, e);
        };
        ref();
        if (isMainThread()) {
            webBase->messageLoop()->addIdler(nullptr, fn, this);
        } else {
            webBase->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
                nullptr, fn, this);
        }
    } break;
    case LwsEvent::ONMESSAGE: {
        ref();
        if (isBinary) {
            struct Param {
                std::vector<char> data;
                SocketLWS* lws;
            };
            Param* p = new Param();
            p->lws = this;
            p->data = std::move(m_rxBuffer);

            webBase->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
                nullptr,
                [](size_t handle, void* data) {
                    Param* p = (Param*)data;
                    SocketLWS* lws = p->lws;
                    lws->deref();
                    WebSocket* socket = lws->parent();
                    String* eventName = socket->executionContext()
                                            ->starfish()
                                            ->staticStrings()
                                            ->m_message.localName();
                    MessageEvent* e =
                        new MessageEvent(socket->executionContext(), eventName);

                    size_t dataSize = p->data.size();
                    if (socket->isBlobBinaryType()) {
                        void* buffer = malloc(dataSize);
                        memcpy(buffer, p->data.data(), dataSize);
                        // TODO : SHOULD support mime type
                        Blob* blob =
                            new Blob(socket->executionContext(), dataSize,
                                     String::createASCIIString("mime/type"),
                                     buffer, false, false, true);
                        e->setData(createScriptValue(blob->scriptObject()));
                    } else {
                        auto scriptArrayBuffer = createScriptArrayBuffer(
                            socket->executionContext()->scriptBindingInstance(),
                            dataSize);
                        memcpy(scriptArrayBuffer->rawBuffer(), p->data.data(),
                               dataSize);
                        e->setData(createScriptValue(scriptArrayBuffer));
                    }
                    socket->EventTarget::dispatchEventByUA(socket, e);
                    delete p;
                },
                p);
        } else {
            // Text
            struct Param {
                std::vector<char> msg;
                SocketLWS* lws;
                bool isAllASCII;
            };
            Param* p = new Param();
            p->lws = this;
            p->isAllASCII = isAllASCII(m_rxBuffer.data(), m_rxBuffer.size());
            p->msg = std::move(m_rxBuffer);

            webBase->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
                nullptr,
                [](size_t handle, void* data) {
                    Param* p = (Param*)data;
                    SocketLWS* lws = p->lws;
                    lws->deref();
                    WebSocket* socket = lws->parent();
                    String* eventName = socket->executionContext()
                                            ->starfish()
                                            ->staticStrings()
                                            ->m_message.localName();
                    MessageEvent* e =
                        new MessageEvent(socket->executionContext(), eventName);
                    if (p->isAllASCII) {
                        e->setData(createScriptValue(createScriptASCIIString(
                            p->msg.data(), p->msg.size())));
                    } else {
                        e->setData(createScriptValue(
                            createScriptString(p->msg.data(), p->msg.size())));
                    }
                    socket->EventTarget::dispatchEventByUA(socket, e);
                    delete p;
                },
                p);
        }
    } break;
    }
}

} // namespace Starfish

#endif
