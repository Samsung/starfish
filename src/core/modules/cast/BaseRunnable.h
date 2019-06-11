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

#ifndef __StarfishBaseRunnable__
#define __StarfishBaseRunnable__

#include "core/modules/threading/IRunnable.h"

namespace Starfish {

class MessageLoop;

class BaseRunnable : public IRunnable {
public:
    class Client : public gc {
    public:
        virtual ~Client()
        {
        }
        virtual void onStopped() = 0;
    };

    BaseRunnable(MessageLoop* messageLoop);
    virtual ~BaseRunnable();

    void run() override;
    void stop() override;
    void setStopper(std::future<void>&& stopper) override;

    void addClient(Client* client);

protected:
    virtual bool doRun();
    virtual void postRun();

private:
    bool isStopRequested();

    MessageLoop* m_messageLoop;
    GCVector<Client*> m_clients;

    std::atomic_bool m_isClientsUpdated;
    std::atomic_bool m_isStopped;
    std::future<void> m_stopper;

    std::mutex m_mutex;
    std::condition_variable m_cv;
};

} // namespace Starfish

#endif
