#ifndef __StarFishFrameDocument__
#define __StarFishFrameDocument__

#include "layout/FrameBlockBox.h"

namespace StarFish {

class FrameDocument : public FrameBlockBox {
public:
    FrameDocument(Node* node, ComputedStyle* style)
        : FrameBlockBox(node, style)
    {

    }

    virtual const char* name()
    {
        return "FrameDocument";
    }

    virtual bool isFrameDocument()
    {
        return true;
    }

    virtual void layout(LayoutContext& ctx);
    virtual void paint(Canvas* canvas, PaintingStage stage)
    {
        STARFISH_ASSERT(stage == PaintingStackingContext);
        STARFISH_ASSERT(firstChild() == lastChild());
        if (!firstChild())
            return;
        firstChild()->paint(canvas, stage);
    }
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage)
    {
        STARFISH_ASSERT(stage == HitTestStackingContext);
        STARFISH_ASSERT(firstChild() == lastChild());
        if (!firstChild())
            return nullptr;
        Frame* result = firstChild()->hitTest(x, y, stage);
        if (result)
            return result;
        return this;
    }
};

}

#endif
