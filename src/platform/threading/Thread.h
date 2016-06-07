#ifndef __StarFishThread__
#define __StarFishThread__

namespace StarFish {

typedef void* (*ThreadWorker)(void*);

class Thread : public gc {
public:
    Thread();
    ~Thread() { }

    void run(ThreadWorker fn, void* data);
    bool isAlive()
    {
        return m_alive;
    }
protected:
    volatile bool m_alive;
};

}

#endif
