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

    size_t addIdler(void (*fn)(size_t handle, void*), void* data);
    size_t addIdler(void (*fn)(size_t handle, void*, void*), void* data, void* data1);
    size_t addIdler(void (*fn)(size_t handle, void*, void*, void*), void* data, void* data1, void* data2);
    size_t addIdlerWithNoGCRootingInOtherThread(void (*fn)(size_t handle, void*), void* data);

    void removeIdler(size_t handle);
    void removeIdlerWithNoGCRooting(size_t handle);
protected:
    StarFish* m_starFish;
    void run();
};

}

#endif
