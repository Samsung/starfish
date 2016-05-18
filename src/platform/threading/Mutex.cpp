#include "StarFishConfig.h"
#include "Mutex.h"

namespace StarFish {

Mutex::Mutex()
{
    pthread_mutex_init(&m_mutex, NULL);
}

Mutex::~Mutex()
{
    auto check = pthread_mutex_destroy(&m_mutex);
    STARFISH_ASSERT(check == 0);
}

void Mutex::lock()
{
    pthread_mutex_lock(&m_mutex);
}

void Mutex::unlock()
{
    pthread_mutex_unlock(&m_mutex);
}


}
