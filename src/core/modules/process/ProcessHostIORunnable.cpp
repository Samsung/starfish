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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include "StarfishConfig.h"

#include <nn.hpp>
#include <nanomsg/pair.h>

#include "core/modules/threading/IRunnable.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/process/networking/Socket.h"
#include "ProcessHostIORunnable.h"

namespace Starfish {

#define RECV_TIMEOUT 1000

ProcessHostIORunnable::ProcessHostIORunnable(MessageLoop* messageLoop,
                                             Client* client)
    : m_messageLoop(messageLoop)
    , m_isStopped(false)
    , m_rcvtimeout(RECV_TIMEOUT)
    , m_client(client)
{
    STARFISH_ASSERT(m_messageLoop);
    STARFISH_ASSERT(client);
}

void ProcessHostIORunnable::run()
{
    /*
    NOTE: We many consider the followings if needed in the future
    1. Wrapping the 3rd party poller
    2. Creating networking stuffs including sockets through a factory
    */

    class Param {
    public:
        Param()
            : client(nullptr)
            , socket(nullptr)
            , buffer(nullptr)
        {
        }

        ~Param()
        {
            if (buffer) {
                nn::freemsg(buffer);
                buffer = nullptr;
            }
        }

        Client* client;
        Socket* socket;
        void* buffer;
    };

    std::unique_lock<std::mutex> lock(m_mutex);

    const int nSockets = m_sockets.size();
    struct nn_pollfd pfd[nSockets];

    for (int i = 0; i < nSockets; ++i) {
        pfd[i].fd = m_sockets[i]->getFd();
        pfd[i].events = m_sockets[i]->getEvents();
    }

    while (stopRequested() == false) {
        try {
            char* buffer = nullptr;

            int rc = nn_poll(pfd, nSockets, m_rcvtimeout);

            if (rc < 0) {
                throw Socket::Exception();
            } else if (rc == 0) {
                continue; // timeout
            } else {
                for (int i = 0; i < nSockets; i++) {
                    if (pfd[i].revents & NN_POLLIN) {
                        rc = m_sockets[i]->recv(&buffer, NN_MSG, NN_DONTWAIT);
                        /*
                        NOTE: addIdlerWithNoGCRootingInOtherThread guarantees
                        that the given idler should be invoked through doing
                        `m_shouldExecute = true` inside it.
                        */

                        Param* param = new Param();
                        param->client = m_client;
                        param->socket = m_sockets[i];
                        param->buffer = buffer;

                        m_messageLoop->addIdlerWithNoGCRootingInOtherThread(
                            nullptr,
                            [](size_t, void* data) {
                                Param* p = (Param*)data;
                                p->client->onReceived(p->socket->getFd(),
                                                      (const char*)p->buffer);
                                delete p;
                            },
                            param);
                    }
                }
            }

        } catch (const Socket::Exception& e) {
            if (e.num() != ETIMEDOUT) {
                STARFISH_LOG_INFO("Networking failed due to %s\n", e.what());
                break;
            }
        }
    };

    m_isStopped = true;
    auto notifier = [](size_t, void* data) {
        Client* client = (Client*)data;
        client->onStopped();
    };
    m_messageLoop->addIdlerWithNoGCRootingInOtherThread(nullptr, notifier,
                                                        m_client);
    closeSockets();
}

void ProcessHostIORunnable::setStopper(std::future<void>&& stopper)
{
    m_stopper = std::move(stopper);
}

void ProcessHostIORunnable::stop()
{
    m_isStopped = true;
};

bool ProcessHostIORunnable::addSocket(Socket* socket)
{
    std::unique_lock<std::mutex> lock(m_mutex, std::defer_lock);
    if (lock.try_lock()) {
        m_sockets.push_back(socket);
        return true;
    }
    return false;
}

void ProcessHostIORunnable::closeSockets()
{
    for (const auto& socket : m_sockets) {
        delete socket;
    }
}

bool ProcessHostIORunnable::stopRequested()
{
    if ((m_stopper.wait_for(std::chrono::milliseconds(0)) ==
         std::future_status::timeout) &&
        (m_isStopped == false))
        return false;
    return true;
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
