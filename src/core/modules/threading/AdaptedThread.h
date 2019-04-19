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

#ifndef __StarfishAdaptedThread__
#define __StarfishAdaptedThread__

namespace Starfish {

class Thread;
class ThreadPool;
class IRunnable;

class IThread : public gc {
public:
    virtual ~IThread()
    {
    }
    virtual void start(IRunnable* runnable) = 0;
    virtual void stop() = 0;
    virtual void join() = 0;
};

class AdaptedThread : public IThread {
public:
    AdaptedThread(ThreadPool* threadPool);
    virtual ~AdaptedThread();

    void start(IRunnable* runnable) override;
    void stop() override;
    void join() override;

private:
    void run();

    Thread* m_threadImp;
    IRunnable* m_runnable;
    std::atomic_bool m_isAlive;
    ThreadPool* m_threadPool;
};

} // namespace Starfish

#endif
