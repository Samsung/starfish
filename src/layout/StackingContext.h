#ifndef __StarFishStackingContext__
#define __StarFishStackingContext__

namespace StarFish {
class Node;
class FrameBox;
class StackingContext;
class Canvas;
class Frame;

class StackingContextChild : public std::vector<StackingContext*, gc_allocator<StackingContext*> > , public gc {
};

class StackingContext : public gc {
public:
    StackingContext(FrameBox* owner, StackingContext* parent);
    void clearChildContexts()
    {
        m_childContexts.clear();
    }

    const std::map<int32_t, StackingContextChild*, std::less<int32_t>, gc_allocator<std::pair<uint32_t, StackingContextChild*> > >& childContexts()
    {
        return m_childContexts;
    }

    FrameBox* owner()
    {
        return m_owner;
    }

    bool needsOwnBuffer()
    {
        return m_needsOwnBuffer;
    }
    bool computeStackingContextProperties(bool forceNeedsBuffer = false);

    void paintStackingContext(Canvas* canvas);
    Frame* hitTestStackingContext(LayoutUnit x, LayoutUnit y);

protected:
    bool m_needsOwnBuffer;
    FrameBox* m_owner;
    StackingContext* m_parent;
    ImageData* m_buffer;

    SkMatrix m_matrix;

    std::map<int32_t, StackingContextChild*, std::less<int32_t>, gc_allocator<std::pair<uint32_t, StackingContextChild*> > > m_childContexts;
};
}

#endif
