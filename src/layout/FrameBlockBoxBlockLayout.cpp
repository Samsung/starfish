#include "StarFishConfig.h"
#include "FrameBlockBox.h"
#include "FrameText.h"
#include "FrameInline.h"

namespace StarFish {

LayoutUnit FrameBlockBox::layoutBlock(LayoutContext& ctx)
{
    LayoutUnit top = paddingTop() + borderTop();
    LayoutUnit bottom = paddingBottom() + borderBottom();
    LayoutUnit normalFlowHeight = 0;
    MarginInfo marginInfo(top, bottom, isEstablishesBlockFormattingContext());
    LayoutUnit maxPositiveMarginTop, maxNegativeMarginTop;
    LayoutUnit lastMarginBottom;
    if (marginInfo.canCollapseTopWithChildren()) {
        if (marginTop() >= 0) {
            maxPositiveMarginTop = marginTop();
            ctx.setMaxPositiveMarginTop(std::max(ctx.maxPositiveMarginTop(), marginTop()));
            marginInfo.setPositiveMargin(ctx.maxPositiveMarginTop());
        } else {
            maxNegativeMarginTop = -marginTop();
            ctx.setMaxNegativeMarginTop(std::max(ctx.maxNegativeMarginTop(), maxNegativeMarginTop));
            marginInfo.setNegativeMargin(ctx.maxNegativeMarginTop());
        }
    } else {
        ctx.setMaxMarginTop(0, 0);
    }
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
            LayoutUnit mX = 0;
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

            LayoutUnit posTop, negTop;
            if (child->asFrameBox()->marginTop() >= 0)
                posTop = std::max(child->asFrameBox()->marginTop(), ctx.maxPositiveMarginTop());
            else
                negTop = std::max(-child->asFrameBox()->marginTop(), ctx.maxNegativeMarginTop());

            if (marginInfo.canCollapseWithMarginTop()) {
                maxPositiveMarginTop = std::max(maxPositiveMarginTop, posTop);
                maxNegativeMarginTop = std::max(maxNegativeMarginTop, posTop);
            }

            if (child->asFrameBox()->isSelfCollapsingBlock()) {
                if (child->asFrameBox()->marginBottom() >= 0)
                    posTop = std::max(posTop, child->asFrameBox()->marginBottom());
                else
                    negTop = std::max(negTop, -child->asFrameBox()->marginBottom());
            }
            posTop = std::max(marginInfo.positiveMargin(), posTop);
            negTop = std::max(marginInfo.negativeMargin(), negTop);
            if (child->asFrameBox()->isSelfCollapsingBlock()) {
                marginInfo.setMargin(posTop, negTop);
            } else {
                if (!marginInfo.atTopSideOfBlock() || !marginInfo.canCollapseWithMarginTop()) {
                    LayoutUnit logicalTop = std::max(marginInfo.positiveMargin(), posTop)
                        - std::max(marginInfo.negativeMargin(), negTop);
                    child->asFrameBox()->moveY(logicalTop);
                }
                marginInfo.setMargin(child->asFrameBox()->marginBottom());
            }
            ctx.setMaxMarginTop(marginInfo.positiveMargin(), marginInfo.negativeMargin());

            if (marginInfo.atTopSideOfBlock() && !child->asFrameBox()->isSelfCollapsingBlock()) {
                marginInfo.setAtTopSideOfBlock(false);
            }
        }

        lastMarginBottom = marginInfo.positiveMargin() - marginInfo.negativeMargin();
        if (child->isNormalFlow()) {
            normalFlowHeight = child->asFrameBox()->height() + child->asFrameBox()->y() - top;
        } else {
            ctx.registerAbsolutePositionedFrames(child);
            lastMarginBottom = 0;
        }

        child = child->next();
    }
    // TODO(june0cho) implement margin-collapse for bottom
    normalFlowHeight += lastMarginBottom;
    ctx.setMaxMarginTop(maxPositiveMarginTop, maxNegativeMarginTop);
    return normalFlowHeight;
}

}
