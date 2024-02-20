/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#ifndef __ThreadedCallHelper__
#define __ThreadedCallHelper__

#include <functional>
#include <memory>

namespace Starfish {
class MessageLoop;
}

namespace LWEDelegate {

class Caller {
public:
    virtual ~Caller() = default;

    virtual void SyncCall(const std::function<void()>& functor) = 0;
    virtual void AsyncCall(Starfish::MessageLoop* messageLoop,
                           const std::function<void()>& functor) = 0;
};

class ThreadedCallHelper {
public:
    static ThreadedCallHelper* Instance();

    ThreadedCallHelper(const ThreadedCallHelper&) = delete;
    ThreadedCallHelper& operator=(const ThreadedCallHelper&) = delete;
    ThreadedCallHelper(ThreadedCallHelper&&) = delete;

    void Initialize(bool isThreadMode);

    void PostTaskToLWEMainThreadSync(const std::function<void()>& functor);
    void PostTaskToLWEMainThreadAsync(Starfish::MessageLoop* messageLoop,
                                      const std::function<void()>& functor);

private:
    static ThreadedCallHelper* m_instance;

    ThreadedCallHelper();
    void CreateLWEMainThread();

    bool m_isLWEThreadStarted = false;
    pthread_mutex_t m_mainThreadInitLocker;

    std::unique_ptr<Caller> m_caller;
};

} // namespace LWEDelegate

#endif
