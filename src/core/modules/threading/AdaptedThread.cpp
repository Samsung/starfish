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

#include "StarfishConfig.h"

#include "Thread.h"
#include "ThreadPool.h"
#include "IRunnable.h"
#include "AdaptedThread.h"

namespace Starfish {

AdaptedThread::AdaptedThread(ThreadPool* threadPool)
    : m_threadImp(nullptr)
    , m_runnable(nullptr)
    , m_isAlive(false)
    , m_threadPool(threadPool)
{
}

AdaptedThread::~AdaptedThread()
{
    m_runnable = nullptr;
    m_isAlive = false;

    m_threadImp->joinIfNeeds();
}

void AdaptedThread::start(IRunnable* runnable)
{
    m_runnable = runnable;

    m_threadImp = new Thread(m_threadPool);

    auto worker = [](void* data, std::future<void>&& future) -> void* {
        auto self = static_cast<AdaptedThread*>(data);

        if (self) {
            self->m_runnable->setStopper(std::move(future));
            self->run();
        }
        return nullptr;
    };

    m_isAlive = true;

    m_threadImp->run(m_threadPool->messageLoop(), worker, this);
}

void AdaptedThread::run()
{
    if (m_runnable && m_isAlive) {
        m_runnable->run();
    }

    m_isAlive = false;
}

void AdaptedThread::join()
{
    m_threadImp->joinIfNeeds();
}

void AdaptedThread::stop()
{
    m_runnable->stop();
}

} // namespace Starfish
