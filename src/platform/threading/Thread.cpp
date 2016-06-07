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
