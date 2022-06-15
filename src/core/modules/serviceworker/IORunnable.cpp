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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include "StarfishConfig.h"

#include <nn.hpp>
#include <nanomsg/pair.h>

#include "core/modules/threading/IRunnable.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/serviceworker/SocketNN.h"
#include "core/modules/serviceworker/IORunnable.h"

namespace Starfish {

#define RECV_TIMEOUT 1000
#define MAX_LISTEN_SOCKET 50

IORunnable::IORunnable(IMessageLoop* messageLoop)
    : m_messageLoop(messageLoop)
    , m_isFdUpdateNeeded(true)
    , m_isStopped(false)
    , m_rcvtimeout(RECV_TIMEOUT)
{
    STARFISH_ASSERT(messageLoop != nullptr);
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            IORunnable* self = castTo<IORunnable*>(obj);
            self->~IORunnable();
        },
        NULL, NULL, NULL);
}

IORunnable::~IORunnable()
{
    // NOTE: In order to clean up std members such as `std::future`, this
    // destructor should be invoked when this class instance is no longer used.
}

void IORunnable::run()
{
    class Param {
    public:
        Param()
            : client(nullptr)
            , socket(nullptr)
            , buffer(nullptr)
            , len(0)
        {
        }

        ~Param()
        {
            if (buffer != nullptr) {
                nn::freemsg(buffer);
                buffer = nullptr;
            }
        }

        Client* client;
        Socket* socket;
        void* buffer;
        size_t len;
    };

    struct nn_pollfd pfd[MAX_LISTEN_SOCKET];

    int nSockets = 0;
    auto timeout = std::chrono::milliseconds(1);

    while (stopRequested() == false) {
        std::unique_lock<std::mutex> lock(m_mutex);

        try {
            if (m_cv.wait_for(lock, timeout) == std::cv_status::timeout) {
                if (m_isFdUpdateNeeded == true) {
                    nSockets = m_clients.size();

                    for (int i = 0; i < nSockets; ++i) {
                        pfd[i].fd = m_clients[i]->socket()->getFd();
                        pfd[i].events = m_clients[i]->socket()->getEvents();
                    }
                    m_isFdUpdateNeeded = false;
                }
            }

            char* buffer = nullptr;
            int rc = nn_poll(pfd, nSockets, m_rcvtimeout);

            if (rc < 0) {
                throw SocketNN::Exception();
            } else if (rc == 0) {
                // timeout
            } else {
                for (int i = 0; i < nSockets; i++) {
                    if (pfd[i].revents & NN_POLLIN) {
                        auto socket = m_clients[i]->socket();
                        rc = socket->recv(&buffer, NN_MSG, NN_DONTWAIT);
                        /*
                        NOTE: addIdlerWithNoGCRootingInOtherThread
                        guarantees that the given idler should be invoked
                        through doing `m_shouldExecute = true` inside it.
                        */
                        Param* param = new Param();
                        STARFISH_ASSERT(param != nullptr);

                        Client* client = m_clients[i];
                        param->client = client;
                        param->socket = socket;
                        param->buffer = buffer;
                        param->len = rc;

                        IMessageLoop* messageQueue = client->messageLoop()
                                                         ? client->messageLoop()
                                                         : m_messageLoop;

                        messageQueue->addIdlerWithNoGCRootingInOtherThread(
                            nullptr,
                            [](size_t, void* data) {
                                Param* p = castTo<Param*>(data);
                                p->client->onReceived(
                                    p->socket, (const char*)p->buffer, p->len);
                                delete p;
                            },
                            param);
                    }
                }
            }

        } catch (const Socket::Exception& e) {
            if (e.num() != ETIMEDOUT) {
                STARFISH_LOG_INFO("Networking failed due to %s", e.what());
                break;
            }
        }
    };

    m_isStopped = true;

    for (const auto& connection : m_clients) {
        auto notifier = [](size_t, void* data) {
            Client* client = castTo<Client*>(data);
            client->onStopped();
        };
        m_messageLoop->addIdlerWithNoGCRootingInOtherThread(nullptr, notifier,
                                                            connection);
    }

    closeClients();
}

void IORunnable::setStopper(std::future<void>&& stopper)
{
    m_stopper = std::move(stopper);
}

void IORunnable::stop()
{
    m_isStopped = true;
};

void IORunnable::addClient(Client* connection)
{
    STARFISH_ASSERT(connection != nullptr);
    std::unique_lock<std::mutex> lock(m_mutex);

    m_clients.push_back(connection);
    m_isFdUpdateNeeded = true;

    lock.unlock();
    m_cv.notify_one();
}

void IORunnable::closeClients()
{
    for (const auto& connection : m_clients) {
        connection->socket()->close();
    }
}

bool IORunnable::stopRequested()
{
    if ((m_stopper.wait_for(std::chrono::milliseconds(0)) ==
         std::future_status::timeout) &&
        (m_isStopped == false))
        return false;
    return true;
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
