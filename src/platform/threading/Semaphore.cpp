#include "StarFishConfig.h"
#include "Semaphore.h"

namespace StarFish {

Semaphore::Semaphore(size_t cnt)
{
    m_semaphore = new sem_t;
    sem_init(m_semaphore, 0, cnt);
    GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
        sem_t* m = (sem_t*)cd;
        sem_destroy(m);
        delete m;
    }, m_semaphore, NULL, NULL);
}

void Semaphore::lock()
{
    sem_wait(m_semaphore);
}

void Semaphore::unlock()
{
    sem_post(m_semaphore);
}


}
