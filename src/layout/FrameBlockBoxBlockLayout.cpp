#include "StarFishConfig.h"
#include "FrameBlockBox.h"
#include "FrameText.h"
#include "FrameInline.h"

namespace StarFish {

std::pair<LayoutUnit, LayoutRect> FrameBlockBox::layoutBlock(LayoutContext& ctx)
{
    LayoutUnit top = paddingTop() + borderTop();
    LayoutUnit bottom = paddingBottom() + borderBottom();
    LayoutUnit normalFlowHeight = 0, maxNormalFlowBottom = top;
    LayoutRect visibleRect(0, 0, 0, 0);
    MarginInfo marginInfo(top, bottom, isEstablishesBlockFormattingContext(), style()->height());
    LayoutUnit maxPositiveMarginTop, maxNegativeMarginTop;
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
    if (!child)
        marginInfo.setMargin(0, 0);
    LayoutUnit marginForAbsolute;
    bool hasOnlySelfCollapsing = true;
    while (child) {
        // Place the child.
        child->layout(ctx, Frame::LayoutWantToResolve::ResolveWidth);

        if (style()->direction() == LtrDirectionValue) {
            child->asFrameBox()->setX(paddingLeft() + borderLeft());
        } else {
            child->asFrameBox()->setX(contentWidth() - paddingRight() - borderRight() - child->asFrameBox()->width());
        }

        child->asFrameBox()->setY(normalFlowHeight + top);

        Length marginLeft = child->style()->marginLeft();
        Length marginRight = child->style()->marginRight();
        LayoutUnit mX = 0;
        if (!marginLeft.isAuto() && !marginRight.isAuto()) {
            if (style()->direction() == LtrDirectionValue) {
                mX = child->asFrameBox()->marginLeft();
            } else {
                mX = -child->asFrameBox()->marginRight();
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

        if (child->isNormalFlow()) {
            // Lay out the child
            child->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
            LayoutUnit posTop = marginInfo.positiveMargin(), negTop = marginInfo.negativeMargin();
            if (child->asFrameBox()->marginTop() >= 0)
                posTop = std::max(child->asFrameBox()->marginTop(), posTop);
            else
                negTop = std::max(-child->asFrameBox()->marginTop(), negTop);

            LayoutUnit topPosition = posTop - negTop;

            posTop = std::max(posTop, ctx.maxPositiveMarginTop());
            negTop = std::max(negTop, ctx.maxNegativeMarginTop());
            if (marginInfo.canCollapseWithMarginTop() && marginInfo.atTopSideOfBlock()) {
                // NOTE: max[Positive/Negative]MarginTop has the collapsed margin between ancestors and first child
                maxPositiveMarginTop = std::max(maxPositiveMarginTop, posTop);
                maxNegativeMarginTop = std::max(maxNegativeMarginTop, negTop);
            } else if (child->asFrameBox()->isSelfCollapsingBlock(ctx)) {
                child->asFrameBox()->moveY(topPosition);
            }

            if (child->asFrameBox()->isSelfCollapsingBlock(ctx)) {
                if (child->asFrameBox()->marginBottom() >= 0)
                    posTop = std::max(posTop, child->asFrameBox()->marginBottom());
                else
                    negTop = std::max(negTop, -child->asFrameBox()->marginBottom());
            }
            LayoutUnit logicalTop = posTop - negTop;
            if (child->asFrameBox()->isSelfCollapsingBlock(ctx)) {
                marginInfo.setMargin(posTop, negTop);
            } else {
                hasOnlySelfCollapsing = false;
                if (!marginInfo.atTopSideOfBlock() || !marginInfo.canCollapseWithMarginTop()) {
                    child->asFrameBox()->moveY(logicalTop);
                }
                marginInfo.setMargin(child->asFrameBox()->marginBottom());
                marginForAbsolute = marginInfo.positiveMargin() - marginInfo.negativeMargin();
                marginInfo.setPositiveMargin(std::max(marginInfo.positiveMargin(), ctx.maxPositiveMarginBottom()));
                marginInfo.setNegativeMargin(std::max(marginInfo.negativeMargin(), ctx.maxNegativeMarginBottom()));
            }
            ctx.setMaxMarginTop(marginInfo.positiveMargin(), marginInfo.negativeMargin());

            if (marginInfo.atTopSideOfBlock() && !child->asFrameBox()->isSelfCollapsingBlock(ctx)) {
                marginInfo.setAtTopSideOfBlock(false);
            }
        }

        if (child->isNormalFlow()) {
            if (!child->asFrameBox()->isSelfCollapsingBlock(ctx)) {
                if (maxNormalFlowBottom < child->asFrameBox()->height() + child->asFrameBox()->y())
                    maxNormalFlowBottom = child->asFrameBox()->height() + child->asFrameBox()->y();
                normalFlowHeight = child->asFrameBox()->height() + child->asFrameBox()->y() - top;
            }
            visibleRect.unite(child->asFrameBox()->visibleRect());
        } else {
            child->asFrameBox()->moveY(marginForAbsolute);
            ctx.registerAbsolutePositionedFrames(child);
        }

        child = child->next();
    }

    // NOTE: At this point, ctx.max[P/N]MarginTop has the collapsed margin
    // between collapsible ancestors and first descendants.
    ctx.setMaxMarginTop(maxPositiveMarginTop, maxNegativeMarginTop);
    // NOTE: At this point, ctx.max[P/N]MarginBottom has the collapsed margin
    // between this block and last descendants.
    ctx.setMaxMarginBottom(std::max(ctx.maxPositiveMarginBottom(), marginInfo.positiveMargin()),
        std::max(ctx.maxNegativeMarginBottom(), marginInfo.negativeMargin()));
    normalFlowHeight = maxNormalFlowBottom - top;
    if (!marginInfo.canCollapseWithMarginBottom()) {
        normalFlowHeight += ctx.maxPositiveMarginBottom() - ctx.maxNegativeMarginBottom();
        ctx.setMaxMarginBottom(0, 0);
    }

    return std::make_pair(normalFlowHeight, visibleRect);
}

}
