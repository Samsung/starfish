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

#include "StarfishConfig.h"

#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/cast/BaseRunnable.h"

namespace Starfish {

#define CV_STATUS_TIMEOUT_MS 1

BaseRunnable::BaseRunnable(MessageLoop* messageLoop)
    : m_messageLoop(messageLoop)
    , m_isClientsUpdated(true)
    , m_isStopped(false)
{
    STARFISH_ASSERT(messageLoop != nullptr);
    GC_REGISTER_FINALIZER_NO_ORDER(this,
                                   [](void* obj, void* cd) {
                                       BaseRunnable* self =
                                           castTo<BaseRunnable*>(obj);
                                       self->~BaseRunnable();
                                   },
                                   NULL, NULL, NULL);
}

BaseRunnable::~BaseRunnable()
{
    // NOTE: In order to clean up std members such as `std::future`, this
    // destructor should be invoked when this class instance is no longer used.
}

void BaseRunnable::run()
{
    if (preRun() == false) {
        return;
    }

    auto timeout = std::chrono::milliseconds(CV_STATUS_TIMEOUT_MS);

    while (isStopRequested() == false) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_cv.wait_for(lock, timeout) == std::cv_status::timeout) {
            if (m_isClientsUpdated == true) {
                // TODO: provide an interface if needed
                m_isClientsUpdated = false;
            }
        }

        if (doRun() == false) {
            break;
        }
    };

    postRun();
}

bool BaseRunnable::preRun()
{
    return true;
}

bool BaseRunnable::doRun()
{
    return false;
}

void BaseRunnable::postRun()
{
    m_isStopped = true;

    for (const auto& client : m_clients) {
        auto notifier = [](size_t, void* data) {
            Client* self = castTo<Client*>(data);
            self->onStopped();
        };

        m_messageLoop->addIdlerWithNoGCRootingInOtherThread(nullptr, notifier,
                                                            client);
    }
}

void BaseRunnable::setStopper(std::future<void>&& stopper)
{
    m_stopper = std::move(stopper);
}

void BaseRunnable::stop()
{
    m_isStopped = true;
}

void BaseRunnable::addClient(Client* client)
{
    STARFISH_ASSERT(client != nullptr);
    std::unique_lock<std::mutex> lock(m_mutex);

    m_clients.push_back(client);
    m_isClientsUpdated = true;

    lock.unlock();
    m_cv.notify_one();
}

bool BaseRunnable::isStopRequested()
{
    if ((m_stopper.wait_for(std::chrono::milliseconds(0)) ==
         std::future_status::timeout) &&
        (m_isStopped == false)) {
        return false;
    }
    return true;
}

} // namespace Starfish
