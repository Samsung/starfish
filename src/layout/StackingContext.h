#ifndef __StarFishStackingContext__
#define __StarFishStackingContext__

namespace StarFish
{
class Node;
class FrameBox;

class StackingContext : public gc {
public:
    StackingContext(FrameBox* owner)
    {
        m_owner = owner;
        m_parent = nullptr;
    }

    StackingContext(FrameBox* owner, StackingContext* parent)
    {
        m_owner = owner;
        m_parent = parent;
    }

protected:
    FrameBox* m_owner;
    StackingContext* m_parent;
    std::map<int32_t, StackingContext*, std::less<int32_t>, gc_allocator<std::pair<uint32_t, StackingContext>>> m_childContexts;
};


}

#endif
