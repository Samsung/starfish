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

#if defined(STARFISH_ENABLE_SERVICE_WORKER)
#ifndef __StarfishIORunnable__
#define __StarfishIORunnable__

#include "StarfishBase.h"
#include "core/modules/threading/IRunnable.h"

namespace Starfish {

class Socket;
class IRunnable;
class MessageLoop;
class IMessageLoop;
class Client;

class IORunnable : public IRunnable {
public:
    class Client : public gc {
    public:
        virtual ~Client()
        {
        }
        virtual void onReceived(Socket* socket, const char* data,
                                size_t len) = 0;
        virtual void onStopped() = 0;
        virtual Socket* socket() = 0;
        virtual IMessageLoop* messageLoop()
        {
            return nullptr;
        }
    };

    IORunnable(IMessageLoop* messageLoop, unsigned int timeout = 1000);
    virtual ~IORunnable();

    void run() override;
    void stop() override;
    void setStopper(std::future<void>&& stopper) override;
    void addClient(Client* connection);

private:
    bool stopRequested();
    void closeClients();

    IMessageLoop* m_messageLoop;
    GCVector<Client*> m_clients;

    std::atomic_bool m_isFdUpdateNeeded;
    std::atomic_bool m_isStopped;
    std::future<void> m_stopper;
    int m_rcvtimeout;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

} // namespace Starfish

#endif
#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
