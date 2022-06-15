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

#include <condition_variable>
#include <mutex>
#include <queue>

namespace Starfish {

template <typename T>
class Queue {
public:
    Queue() = default;
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;
    Queue(const Queue&&) = delete;
    Queue& operator=(const Queue&&) = delete;

    void push(const T& item)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push(item);
        lock.unlock();
        m_cv.notify_one();
    }

    T pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_queue.empty()) {
            m_cv.wait(lock);
        }
        T item = m_queue.front();
        m_queue.pop();
        return item;
    }

    bool pop(T& item, const unsigned int millisecondWaitingPeriod)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_queue.empty()) {
            auto timeout = std::chrono::milliseconds(millisecondWaitingPeriod);
            if (m_cv.wait_for(lock, timeout) == std::cv_status::timeout) {
                return false;
            }
        }

        item = m_queue.front();
        m_queue.pop();
        return true;
    }

private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

} // namespace Starfish
