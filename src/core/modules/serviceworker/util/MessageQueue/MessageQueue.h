/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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

#pragma once

#include "Queue.h"
#include <atomic> // atomic_bool

namespace Starfish {

class MessageQueue {
private:
    class Task {
    public:
        explicit Task(std::function<void()>&& f)
            : m_callable(std::move(f))
        {
        }

        void run()
        {
            m_callable();
        }

    private:
        std::function<void()> m_callable;
    };

    using MessageType = Task;

public:
    MessageQueue()
        : m_isStopped(false)
    {
    }

    template <typename Callable, typename... Args>
    void add(Callable&& f, Args&&... args)
    {
        std::shared_ptr<Task> t = std::make_shared<Task>(
            std::bind(std::forward<Callable>(f), std::forward<Args>(args)...));
        m_queue.push(t);
    }

    void run(unsigned int timeout = 1000,
             std::function<bool()> postJobFn = nullptr)
    {
        std::shared_ptr<MessageType> task;

        while (m_isStopped == false) {
            if (m_queue.pop(task, timeout)) {
                task->run();
                task = nullptr;
            }
            if (postJobFn != nullptr) {
                if (postJobFn() == false) {
                    m_isStopped = true;
                }
            }
        }
    }

    void stop()
    {
        m_isStopped = true;
    }

private:
    std::atomic_bool m_isStopped{ false };
    Queue<std::shared_ptr<MessageType>> m_queue;
};

} // namespace Starfish
