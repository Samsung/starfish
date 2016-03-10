#include "StarFishConfig.h"
#include "FrameBox.h"

#include "FrameBlockBox.h"

namespace StarFish {

void FrameBox::paintStackingContextContent(Canvas* canvas)
{
    // the in-flow, non-inline-level, non-positioned descendants.
    paintChildrenWith(canvas, PaintingNormalFlowBlock);

    // TODO the non-positioned floats.
    // paintChildrenWith(canvas, ctx, PaintingNonPositionedFloats);

    // the in-flow, inline-level, non-positioned descendants, including inline tables and inline blocks.
    paintChildrenWith(canvas, PaintingNormalFlowInline);

    // the child stacking contexts with stack level 0 and the positioned descendants with stack level 0.
    paintChildrenWith(canvas, PaintingPositionedElements);
}

}
