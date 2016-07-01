/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "Thread.h"

namespace StarFish {

Thread::Thread()
{
    m_alive = false;
}

void Thread::run(ThreadWorker fn, void* data)
{
    STARFISH_ASSERT(!m_alive);
    m_alive = true;

    struct ThreadData {
        Thread* thread;
        ThreadWorker fn;
        void* data;
    };

    ThreadData* d = new(NoGC) ThreadData;
    d->thread = this;
    d->fn = fn;
    d->data = data;

    pthread_t thread;
    pthread_create(&thread, NULL, [](void* data) -> void* {
        ThreadData* d = (ThreadData*)data;
        auto ret = d->fn(d->data);
        d->thread->m_alive = false;
        GC_FREE(d);
        return ret;
    }, d);
}


}
