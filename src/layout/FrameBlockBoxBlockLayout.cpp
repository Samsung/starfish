#include "StarFishConfig.h"
#include "FrameBlockBox.h"
#include "FrameText.h"
#include "FrameInline.h"

namespace StarFish {

float FrameBlockBox::layoutBlock(LayoutContext& ctx)
{
    float top = paddingTop() + borderTop();
    float normalFlowHeight = 0;
    Frame* child = firstChild();
    while (child) {
        // Place the child.
        child->asFrameBox()->setX(paddingLeft() + borderLeft());
        child->asFrameBox()->setY(normalFlowHeight + top);

        // Lay out the child
        if (child->isNormalFlow()) {
            child->layout(ctx);
            Length marginLeft = child->style()->marginLeft();
            Length marginRight = child->style()->marginRight();
            float mX = 0;
            if (!marginLeft.isAuto() && !marginRight.isAuto()) {
                // FIXME what "direction value" should we watch? self? child?
                if (style()->direction() == LtrDirectionValue) {
                    mX = child->asFrameBox()->marginLeft();
                } else {
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
            } else if (marginLeft.isAuto() && !marginRight.isAuto()) {
                mX = contentWidth() - child->asFrameBox()->width();
                mX -= child->asFrameBox()->marginRight();
            } else if (!marginLeft.isAuto() && marginRight.isAuto()) {
                mX = child->asFrameBox()->marginLeft();
            } else {
                // auto-auto
                mX = child->asFrameBox()->marginLeft();
            }
            child->asFrameBox()->moveX(mX);
            // TODO implement margin-collapse
            child->asFrameBox()->moveY(child->asFrameBox()->marginTop());
        }

        if (child->isNormalFlow()) {
            // TODO implement margin-collapse
            normalFlowHeight = child->asFrameBox()->height() + child->asFrameBox()->y() + child->asFrameBox()->marginBottom() - top;
        } else
            ctx.registerAbsolutePositionedFrames(child);

        child = child->next();
    }
    return normalFlowHeight;
}

}
