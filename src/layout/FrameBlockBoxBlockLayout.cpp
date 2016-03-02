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
            maxNegativeMarginTop = marginTop();
            ctx.setMaxNegativeMarginTop(std::max(ctx.maxNegativeMarginTop(), marginTop()));
            marginInfo.setNegativeMargin(ctx.maxNegativeMarginTop());
        }
    } else {
        ctx.setMaxPositiveMarginTop(0);
        ctx.setMaxNegativeMarginTop(0);
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

            // FIXME(june0cho): consider negative margin
            LayoutUnit posTop = std::max(child->asFrameBox()->marginTop(), ctx.maxPositiveMarginTop());

            if (marginInfo.canCollapseWithMarginTop()) {
                maxPositiveMarginTop = std::max(maxPositiveMarginTop, posTop);
            }

            if (child->asFrameBox()->isSelfCollapsingBlock()) {
                posTop = std::max(posTop, child->asFrameBox()->marginBottom());
            }
            LayoutUnit logicalTop = std::max(marginInfo.positiveMargin(), posTop);
            if (child->asFrameBox()->isSelfCollapsingBlock()) {
                marginInfo.setPositiveMargin(logicalTop);
            } else {
                if (!marginInfo.atTopSideOfBlock() || !marginInfo.canCollapseWithMarginTop()) {
                    logicalTop = std::max(marginInfo.positiveMargin(), posTop);
                    child->asFrameBox()->moveY(logicalTop);
                }
                marginInfo.setPositiveMargin(child->asFrameBox()->marginBottom());
            }
            ctx.setMaxPositiveMarginTop(marginInfo.positiveMargin());

            if (marginInfo.atTopSideOfBlock() && !child->asFrameBox()->isSelfCollapsingBlock()) {
                marginInfo.setAtTopSideOfBlock(false);
            }
        }

        lastMarginBottom = marginInfo.positiveMargin();
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
    ctx.setMaxPositiveMarginTop(maxPositiveMarginTop);
    return normalFlowHeight;
}

}
