#include "StarFishConfig.h"
#include "Mutex.h"

namespace StarFish {

Mutex::Mutex()
{
    m_mutex = new pthread_mutex_t;
    pthread_mutex_init(m_mutex, NULL);

    GC_REGISTER_FINALIZER_NO_ORDER(this, [] (void* obj, void* cd) {
        // STARFISH_LOG_INFO("Mutex::~Mutex\n");
        pthread_mutex_t* m = (pthread_mutex_t*)cd;
        auto check = pthread_mutex_destroy(m);
        delete m;
        STARFISH_ASSERT(check == 0);
    }, m_mutex, NULL, NULL);
}

void Mutex::lock()
{
    pthread_mutex_lock(m_mutex);
}

void Mutex::unlock()
{
    pthread_mutex_unlock(m_mutex);
}


}
