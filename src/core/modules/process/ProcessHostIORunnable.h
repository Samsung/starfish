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

#ifndef __StarfishProcessIORunnable__
#define __StarfishProcessIORunnable__

namespace Starfish {

class Sockect;
class IRunnable;
class MessageLoop;

class ProcessHostIORunnable : public IRunnable, public gc {
public:
    class Client {
    public:
        virtual ~Client()
        {
        }
        virtual void onReceived(int socketfd, const char* data) = 0;
        virtual void onStopped() = 0;
    };

    ProcessHostIORunnable(MessageLoop* messageLoop, Client* client);

    void run() override;
    void stop() override;
    void setStopper(std::future<void>&& stopper) override;
    bool addSocket(Socket* socket);

private:
    bool stopRequested();
    void closeSockets();

    MessageLoop* m_messageLoop;
    GCVector<Socket*> m_sockets;

    std::atomic_bool m_isStopped;
    std::future<void> m_stopper;
    std::mutex m_mutex;
    int m_rcvtimeout;
    Client* m_client;
};

} // namespace Starfish

#endif
