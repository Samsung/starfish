#ifndef __StarFishMutex__
#define __StarFishMutex__

namespace StarFish {

class Mutex {
    STARFISH_MAKE_STACK_ALLOCATED();
public:
    Mutex();
    ~Mutex();
    void lock();
    void unlock();
protected:
    pthread_mutex_t m_mutex;
};

}

#endif
