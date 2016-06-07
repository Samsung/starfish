#ifndef __StarFishSemaphore__
#define __StarFishSemaphore__

namespace StarFish {

class Semaphore : public gc {
public:
    Semaphore(size_t cnt);
    ~Semaphore() { }
    void lock();
    void unlock();

protected:
    sem_t* m_semaphore;
};

}

#endif
