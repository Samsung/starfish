/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishThread__
#define __StarFishThread__

#include "binding/StarFishHoldable.h"
namespace StarFish {

class MessageLoop;
class Mutex;
class Thread;

typedef void* (*ThreadWorker)(void*);

void registerMainThread();
bool isMainThread();

struct ThreadData {
    ThreadData(Thread* t, MessageLoop* m, ThreadWorker w, void* d)
        : m_thread(t)
        , m_messageLoop(m)
        , m_fn(w)
        , m_data(d)
        , m_joinHandle(SIZE_MAX)
    {
    }
    Thread* m_thread;
    MessageLoop* m_messageLoop;
    ThreadWorker m_fn;
    void* m_data;
    pthread_t m_tid;
    size_t m_joinHandle;
};

class Thread : public gc, public StarFishHoldable {
public:
    Thread(StarFish* starFish);
    ~Thread()
    {
    }

    void run(MessageLoop* msgLoop, ThreadWorker fn, void* data);
    void joinIfNeeds();
    bool isAlive()
    {
        return m_alive;
    }
    void finishUnjoined();
    void stop();

private:
    static void cleanupHandler(void* data);

protected:
    volatile bool m_alive;
    Mutex* m_mutex;
    ThreadData* m_currentUnjoined;
};
}

#endif
