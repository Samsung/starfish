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

#ifdef STARFISH_ENABLE_WEBSOCKET

#include "StarfishConfig.h"
#include "Starfish.h"
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
#include "core/dom/MessageEvent.h"
#include "core/dom/EventTarget.h"
#include "core/modules/threading/AdaptedThread.h"
#include "core/modules/networking/LWSRunnable.h"
#include "core/fileapi/Blob.h"

namespace Starfish {

SocketLWS::Exception::Exception()
    : Socket::Exception::Exception(0)
{
}

#define CHECK_ALIVE() \
    if (!m_alive) {   \
        return;       \
    }

static int LWSSimpleCB(struct lws* wsi, enum lws_callback_reasons reason,
                       void* user, void* in, size_t len)
{
    SocketLWS* socket = (SocketLWS*)user;

    switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        socket->parent()->setReadyState(WebSocket::ReadyState::OPEN);
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
        break;

    case LWS_CALLBACK_CLIENT_WRITEABLE: {
        if (!socket->isActive()) {
            std::string reason = socket->closeReason();
            lws_close_status code = (lws_close_status)socket->closeCode();
            lws_close_reason(wsi, code, (unsigned char*)reason.c_str(),
                             reason.length());
            return -1;
        }

        std::vector<SocketLWSData*>* buffer = socket->txData();
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
            iter = buffer->erase(iter);

            delete data;
        }
        if (!buffer->empty()) {
            lws_callback_on_writable(wsi);
        }
        break;
    }

    case LWS_CALLBACK_CLOSED:
        socket->publishEvent(SocketLWS::LwsEvent::CLOSE);
        socket->parent()->setReadyState(WebSocket::ReadyState::CLOSED);
        socket->shutdown(0);
        break;

    default:
        break;
    }

    return 0;
}

static struct lws_protocols protocols[] = {
    { NULL, LWSSimpleCB, 0, 0, 0, NULL, 0 },
    { NULL, NULL, 0, 0, 0, NULL, 0 } /* terminator */
};

static const char* SocketLWSDefaultCertPath =
    "/etc/ssl/certs/ca-certificates.crt";

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
    : m_active(true)
    , m_alive(true)
    , m_parent(socket)
    , m_lwsContext(nullptr)
    , m_lwsClient(nullptr)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj, void* cd) { ((SocketLWS*)obj)->~SocketLWS(); },
        NULL, NULL, NULL);
    int useSSL = 0;
    const char* prot;
    m_url = parent()->url()->toUTF8NonGCString();
    m_protocol = parent()->protocol()->toUTF8NonGCString();
    char* param = (char*)m_url.c_str();
    if (lws_parse_uri(param, &prot, &m_lwsClientConnectInfo.address,
                      &m_lwsClientConnectInfo.port,
                      &m_lwsClientConnectInfo.path)) {
        // TODO
        return;
    }

    // add back the leading / on path
    if (m_lwsClientConnectInfo.path[0] != '/') {
        m_urlPath = UTF8StringDataNonGCStd(m_lwsClientConnectInfo.path);
        m_urlPath.insert(0, "/");
        m_lwsClientConnectInfo.path = m_urlPath.c_str();
    }

    if (!strcmp(prot, "https") || !strcmp(prot, "wss")) {
        useSSL = LCCSCF_USE_SSL;
    }

    m_lwsContextCreationInfo.port = CONTEXT_PORT_NO_LISTEN;
    m_lwsContextCreationInfo.protocols = protocols;
    m_lwsContextCreationInfo.gid = -1;
    m_lwsContextCreationInfo.uid = -1;
    m_lwsContextCreationInfo.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    // DO not use Proxy
    m_lwsContextCreationInfo.http_proxy_address = "";
    m_lwsContextCreationInfo.socks_proxy_address = "";

    if (useSSL) {
        m_lwsContextCreationInfo.client_ssl_ca_filepath =
            SocketLWSDefaultCertPath;
    }

    m_lwsContext = lws_create_context(&m_lwsContextCreationInfo);

    WebBase* webBase = parent()->executionContext()->webBase();
    m_thread = new AdaptedThread(webBase->threadPool());
    m_runnable = new LWSRunnable(webBase->messageLoop(), this);
    m_thread->start(m_runnable);

    // TODO
    protocols[0].name = m_protocol.data();

    m_lwsClientConnectInfo.context = m_lwsContext;
    m_lwsClientConnectInfo.host = m_lwsClientConnectInfo.address;

    // TODO Should handle origin property
    // m_lwsClientConnectInfo.origin = m_lwsClientConnectInfo.address;
    m_lwsClientConnectInfo.protocol = protocols[0].name;
    m_lwsClientConnectInfo.ssl_connection = useSSL;
    m_lwsClientConnectInfo.userdata = this;
    m_lwsClient = lws_client_connect_via_info(&m_lwsClientConnectInfo);
}
SocketLWS::~SocketLWS()
{
}

void SocketLWS::finalize()
{
    if (m_lwsContext != nullptr) {
        lws_context_destroy(m_lwsContext);
        m_lwsContext = nullptr;
    }
}

void SocketLWS::run()
{
    if (m_lwsContext != nullptr) {
        lws_service(m_lwsContext, 250);
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
    m_active = false;
    lws_callback_on_writable(m_lwsClient);
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
    m_thread->stop();
    return 0;
}

int SocketLWS::send(const void* buf, size_t len, int flags)
{
    SocketLWSData::SocketLWSDataType type =
        SocketLWSData::SocketLWSDataType::TEXT;
    if (flags != 0) {
        type = SocketLWSData::SocketLWSDataType::BINARY;
    }
    SocketLWSData* newData = new SocketLWSData((char*)buf, len, type);
    m_txBuffer.push_back(newData);
    lws_callback_on_writable(m_lwsClient);
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

void SocketLWS::publishEvent(LwsEvent eventType, bool isBinary)
{
    CHECK_ALIVE()

    WebBase* webBase = parent()->executionContext()->webBase();

    switch (eventType) {
    case LwsEvent::OPEN: {
        webBase->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
            nullptr,
            [](size_t handle, void* data) {
                SocketLWS* lws = (SocketLWS*)data;
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
        webBase->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
            nullptr,
            [](size_t handle, void* data) {
                SocketLWS* lws = (SocketLWS*)data;
                WebSocket* socket = lws->parent();
                String* eventName = socket->executionContext()
                                        ->starfish()
                                        ->staticStrings()
                                        ->m_error.localName();
                Event* e = new Event(socket->executionContext(), eventName);
                socket->EventTarget::dispatchEventByUA(socket, e);
            },
            this);
    } break;
    case LwsEvent::CLOSE: {
        webBase->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
            nullptr,
            [](size_t handle, void* data) {
                SocketLWS* lws = (SocketLWS*)data;
                WebSocket* socket = lws->parent();
                String* eventName = socket->executionContext()
                                        ->starfish()
                                        ->staticStrings()
                                        ->m_close.localName();
                Event* e = new Event(socket->executionContext(), eventName);
                socket->EventTarget::dispatchEventByUA(socket, e);
            },
            this);
    } break;
    case LwsEvent::ONMESSAGE: {
        if (isBinary) {
            struct Param {
                std::vector<char> data;
                SocketLWS* lws;
            };
            Param* p = new Param();
            p->lws = this;
            p->data = m_rxBuffer;

            // TODO
            m_rxBuffer.clear();
            m_rxBuffer.shrink_to_fit();

            webBase->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
                nullptr,
                [](size_t handle, void* data) {
                    struct Param {
                        std::vector<char> data;
                        SocketLWS* lws;
                    };
                    Param* p = (Param*)data;
                    SocketLWS* lws = p->lws;

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
                std::string msg;
                SocketLWS* lws;
            };
            Param* p = new Param();
            p->lws = this;
            p->msg = std::string(m_rxBuffer.data(), m_rxBuffer.size());

            m_rxBuffer.clear();
            m_rxBuffer.shrink_to_fit();

            webBase->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
                nullptr,
                [](size_t handle, void* data) {
                    struct Param {
                        std::string msg;
                        SocketLWS* lws;
                    };
                    Param* p = (Param*)data;
                    SocketLWS* lws = p->lws;

                    WebSocket* socket = lws->parent();
                    String* eventName = socket->executionContext()
                                            ->starfish()
                                            ->staticStrings()
                                            ->m_message.localName();
                    MessageEvent* e =
                        new MessageEvent(socket->executionContext(), eventName);
                    e->setData(createScriptValue(createScriptString(
                        String::fromUTF8(p->msg.c_str(), p->msg.length()))));
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
