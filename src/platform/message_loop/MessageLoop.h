#ifndef __StarFishMessageLoop__
#define __StarFishMessageLoop__


namespace StarFish {

class MessageLoop : public gc {
    friend class StarFish;
    friend class Window;
public:
    MessageLoop(StarFish* sf)
        : m_starFish(sf)
    {

    }

    void* addIdler(void (*fn)(void*), void* data);
    void removeIdler(void* handle);
protected:
    StarFish* m_starFish;
    void run();
};

}

#endif
