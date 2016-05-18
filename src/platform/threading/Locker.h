#ifndef __StarFishLocker__
#define __StarFishLocker__

namespace StarFish {

template<typename T>
class Locker {
    STARFISH_MAKE_STACK_ALLOCATED();
public:
    Locker(T& lock)
        : m_lock(lock)
    {
    }

    void lock()
    {
        m_lock.lock();
    }

    void unlock()
    {
        m_lock.unlock();
    }
protected:
    T& m_lock;
};

}

#endif
