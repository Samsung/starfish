#ifndef __StarFishFrameDocument__
#define __StarFishFrameDocument__

#include "layout/FrameBlockBox.h"

namespace StarFish {

class FrameDocument : public FrameBlockBox {
public:
    FrameDocument(Node* node)
        : FrameBlockBox(node, nullptr)
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
    virtual void paint(Canvas* canvas, PaintingContext& ctx, PaintingStage stage)
    {
        STARFISH_ASSERT(stage == PaintingStackingContext);
        STARFISH_ASSERT(firstChild() == lastChild());
        if (!firstChild())
            return;
        canvas->save();
        canvas->translate(firstChild()->asFrameBox()->x(), firstChild()->asFrameBox()->y());
        firstChild()->asFrameBox()->paintStackingContext(canvas, ctx);
        canvas->restore();
    }

    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestContext& ctx, HitTestStage stage)
    {
        STARFISH_ASSERT(stage == HitTestStackingContext);
        STARFISH_ASSERT(firstChild() == lastChild());
        if (!firstChild())
            return nullptr;

        Frame* result = firstChild()->asFrameBox()->hitTestStackingContext(x, y, ctx);
        if (result)
            return result;
        return this;
    }
};

}

#endif
