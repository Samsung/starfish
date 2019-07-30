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

namespace Starfish {

SocketLWS::Exception::Exception()
    : Socket::Exception::Exception(0)
{
}

static int LWSSimpleCB(struct lws* wsi, enum lws_callback_reasons reason,
                       void* user, void* in, size_t len)
{
    SocketLWS* socket = (SocketLWS*)user;
    switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        socket->publishEvent(SocketLWS::LwsEvent::OPEN, NULL, 0);
        lws_callback_on_writable(wsi);
        break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
        socket->publishEvent(SocketLWS::LwsEvent::ONMESSAGE, (char*)in, len);
        break;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        // TODO
        socket->publishEvent(SocketLWS::LwsEvent::ERROR, NULL, 0);
        break;

    case LWS_CALLBACK_CLIENT_WRITEABLE: {
        std::vector<SocketLWSData*>* buffer = socket->data();
        if (!buffer->empty()) {
            auto iter = buffer->begin();
            SocketLWSData* data = (SocketLWSData*)*iter;
            lws_write(wsi, ((unsigned char*)data->data()) + LWS_PRE,
                      data->size(), LWS_WRITE_TEXT);
            iter = buffer->erase(iter);
            delete data;
        }
        if (!buffer->empty()) {
            lws_callback_on_writable(wsi);
        }
        break;
    }

    case LWS_CALLBACK_CLOSED:
        socket->publishEvent(SocketLWS::LwsEvent::CLOSE, NULL, 0);
        break;

    default:
        break;
    }

    return 0;
}

static struct lws_protocols protocols[] = {
    { "SocketLWS-defalut-protocol", LWSSimpleCB, 0, 0, 0, NULL, 0 },
    { NULL, NULL, 0, 0, 0, NULL, 0 } /* terminator */
};

const char* SocketLWS::Exception::what() const throw()
{
    return nullptr;
}

SocketLWSData::SocketLWSData(const char* buf, size_t size)
{
    m_size = size;
    m_buffer = malloc(LWS_PRE + size);
    memcpy(((char*)m_buffer) + LWS_PRE, buf, size);
}

SocketLWS::SocketLWS(WebSocket* socket)
    : m_active(false)
    , m_parent(socket)
    , m_lwsContext(nullptr)
    , m_lwsClient(nullptr)
{
    m_lwsContextCreationInfo.port = CONTEXT_PORT_NO_LISTEN;
    m_lwsContextCreationInfo.protocols = protocols;
    m_lwsContextCreationInfo.gid = -1;
    m_lwsContextCreationInfo.uid = -1;
    m_lwsContext = lws_create_context(&m_lwsContextCreationInfo);

    m_active = true;
    WebBase* webBase = parent()->executionContext()->webBase();
    m_thread = new Thread(webBase->threadPool());
    m_thread->run(webBase->messageLoop(),
                  [](void* data) -> void* {
                      SocketLWS* socket = (SocketLWS*)data;
                      socket->run();
                      return nullptr;
                  },
                  this);
}

SocketLWS::~SocketLWS()
{
    close();
}

void SocketLWS::run()
{
    if (m_lwsContext != nullptr) {
        while (m_active || !m_buffer.empty()) {
            lws_service(m_lwsContext, 1000);
        }
        lws_context_destroy(m_lwsContext);
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

int SocketLWS::close()
{
    m_active = false;
    return 0;
}

int SocketLWS::connect(String* url)
{
    m_url = url->toUTF8NonGCString();

    int use_ssl = 0;
    const char* prot;
    char* param;
    param = (char*)m_url.c_str();
    if (lws_parse_uri(param, &prot, &m_lwsClientConnectInfo.address,
                      &m_lwsClientConnectInfo.port,
                      &m_lwsClientConnectInfo.path)) {
        return 0;
    }
    if (!strcmp(prot, "https") || !strcmp(prot, "wss")) {
        use_ssl = LCCSCF_USE_SSL;
    }
    m_lwsClientConnectInfo.context = m_lwsContext;
    m_lwsClientConnectInfo.host = m_lwsClientConnectInfo.address;
    m_lwsClientConnectInfo.origin = m_lwsClientConnectInfo.address;
    m_lwsClientConnectInfo.protocol = protocols[0].name;
    m_lwsClientConnectInfo.ssl_connection = use_ssl;
    m_lwsClientConnectInfo.userdata = this;
    m_lwsClient = lws_client_connect_via_info(&m_lwsClientConnectInfo);

    return 0;
}

int SocketLWS::connect(const char* addr)
{
    STARFISH_ASSERT_NOT_REACHED();
    return 0;
}

int SocketLWS::shutdown(int howto)
{
    STARFISH_ASSERT_NOT_REACHED();
    return 0;
}

int SocketLWS::send(const void* buf, size_t len, int flags)
{
    SocketLWSData* newData = new SocketLWSData((char*)buf, len);
    m_buffer.push_back(newData);
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

void SocketLWS::publishEvent(LwsEvent eventType, char* param, size_t size)
{
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
        webBase->messageLoop()->addIdlerWithNoGCRootingInOtherThread(
            nullptr,
            [](size_t handle, void* data) {
                SocketLWS* lws = (SocketLWS*)data;
                WebSocket* socket = lws->parent();
                String* eventName = socket->executionContext()
                                        ->starfish()
                                        ->staticStrings()
                                        ->m_message.localName();
                Event* e =
                    new MessageEvent(socket->executionContext(), eventName);
                socket->EventTarget::dispatchEventByUA(socket, e);
            },
            this);
    } break;
    }
}

} // namespace Starfish

#endif
