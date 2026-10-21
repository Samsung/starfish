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
#include <pthread.h>
#include <mutex>

namespace Starfish {
class MessageLoop;
}

namespace LWEDelegate {

class ThreadedCallHelper {
public:
    static ThreadedCallHelper* Instance();

    ThreadedCallHelper(const ThreadedCallHelper&) = delete;
    ThreadedCallHelper& operator=(const ThreadedCallHelper&) = delete;
    ThreadedCallHelper(ThreadedCallHelper&&) = delete;

    void Initialize(bool isThreadMode);
    bool ShutdownForUnload();

    void PostTaskToLWEMainThreadSync(const std::function<void()>& functor);
    void PostTaskToLWEMainThreadAsync(Starfish::MessageLoop* messageLoop,
                                      const std::function<void()>& functor);

    bool isThreadMode() const;

private:
    ThreadedCallHelper();
    void CreateLWEMainThread();

    bool m_isLWEThreadStarted;
    bool m_isThreadMode;
    bool m_isMessageLoopInitialized;
    bool m_didShutdown;
    pthread_t m_mainThread;
    std::mutex m_shutdownMutex;
};

} // namespace LWEDelegate

#endif
