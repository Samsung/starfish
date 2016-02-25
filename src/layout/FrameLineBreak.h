#ifndef __StarFishFrameLineBreak__
#define __StarFishFrameLineBreak__

#include "layout/Frame.h"

namespace StarFish {

class FrameLineBreak : public Frame {
public:
    FrameLineBreak(Node* node)
        : Frame(node, nullptr)
    {

    }

    virtual bool isFrameLineBreak()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameLineBreak";
    }

    virtual void layout(LayoutContext& ctx)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void computePreferredWidth(ComputePreferredWidthContext& ctx)
    {
    }

    virtual void paint(Canvas* canvas, PaintingStage = PaintingStackingContext)
    {
    }

    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage = HitTestStackingContext)
    {
        return nullptr;
    }

    virtual void dump(int depth)
    {
        Frame::dump(depth);
    }

protected:
};

}

#endif
