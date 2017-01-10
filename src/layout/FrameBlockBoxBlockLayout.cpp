/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "FrameDocument.h"
#include "FrameBlockBox.h"
#include "FrameText.h"
#include "FrameInline.h"

namespace StarFish {

static std::pair<LayoutUnit, LayoutUnit> estimateLogicalPosition(Frame* f, LayoutContext& ctx, MarginInfo& marginInfo,
    bool isSelfCollapsing, bool ignore)
{
    if (ignore) {
        return std::make_pair(0, 0);
    }

    f->asFrameBox()->setMarginCollapseResult(MarginCollapseResult());
    LayoutUnit posTop = marginInfo.positiveMargin(), negTop = marginInfo.negativeMargin();

    if (f->asFrameBox()->marginTop() >= 0)
        posTop = std::max(f->asFrameBox()->marginTop(), posTop);
    else
        negTop = std::max(-f->asFrameBox()->marginTop(), negTop);

    if (isSelfCollapsing) {
        if (f->asFrameBox()->marginBottom() >= 0)
            posTop = std::max(posTop, f->asFrameBox()->marginBottom());
        else
            negTop = std::max(negTop, -f->asFrameBox()->marginBottom());
    }

    posTop = std::max(posTop, ctx.maxPositiveMarginTop());
    negTop = std::max(negTop, ctx.maxNegativeMarginTop());

    LayoutUnit topPosition = posTop - negTop;

    if (marginInfo.canCollapseWithMarginTop()) {
        marginInfo.setMaxPositiveMarginTop(posTop);
        marginInfo.setMaxNegativeMarginTop(negTop);
    } else {
        MarginCollapseResult r = f->asFrameBox()->marginCollapseResult();
        r.m_advanceY += topPosition;
        f->asFrameBox()->setMarginCollapseResult(r);
    }

    return std::make_pair(posTop, negTop);
}

static void marginCollapse(Frame* f, LayoutContext& ctx, MarginInfo& marginInfo, bool isSelfCollapsing, LayoutUnit posTop, LayoutUnit negTop)
{
    ctx.setMaxMarginTop(0, 0);

    if (isSelfCollapsing) {
        marginInfo.setMargin(posTop, negTop);
    } else {
        marginInfo.setMargin(f->asFrameBox()->marginBottom());
        marginInfo.setPositiveMargin(std::max(marginInfo.positiveMargin(), ctx.maxPositiveMarginBottom()));
        marginInfo.setNegativeMargin(std::max(marginInfo.negativeMargin(), ctx.maxNegativeMarginBottom()));
        ctx.setMaxMarginBottom(0, 0);
    }

    if (marginInfo.atTopSideOfBlock() && !isSelfCollapsing) {
        marginInfo.setAtTopSideOfBlock(false);
    }
}

static bool shouldAvoidFlaots(Frame* f)
{
    return f->isFrameReplaced() || f->isEstablishesBlockFormattingContext();
}

static bool shouldStretchWidth(Frame* f)
{
    STARFISH_ASSERT(shouldAvoidFlaots(f));
    return f->style()->width().isAuto() && !f->isFrameReplaced();
}

static void setXForDirection(FrameBlockBox* parent, FrameBox* child, DirectionValue direction)
{
    LayoutUnit mX = 0;
    if (direction == LtrDirectionValue) {
        mX = child->marginLeft();
        child->setX(parent->paddingLeft() + parent->borderLeft() + mX);
    } else {
        mX = child->marginRight();
        child->setX(parent->width() - child->width() - mX - parent->borderRight() - parent->paddingRight());
    }
}

LayoutUnit FrameBlockBox::layoutBlock(LayoutContext& ctx)
{
    LayoutLocation loc = absolutePoint(ctx.frameDocument());
    LayoutUnit yAbsPosition;
    LayoutUnit top = paddingTop() + borderTop();
    LayoutUnit bottom = paddingBottom() + borderBottom();
    LayoutUnit normalFlowHeight = 0, maxNormalFlowBottom = top;
    LayoutUnit marginTopOfClearedSelfCollapsingBlock = 0, maxMarginOfSelfCollapsingBlocks = 0, yPositionOfClearedSelfCollapsingBlock = 0;
    Frame* child = firstChild();
    DirectionValue direction = style()->direction();
    bool wasClearedSelfCollapsingBlock = false;
    bool clearAffected = false;
    bool floatAffected = false;

    MarginInfo marginInfo(top, bottom, isEstablishesBlockFormattingContext() || isFrameDocument(), style()->height());
    setMarginInfo(&marginInfo);

    if (!marginInfo.canCollapseTopWithChildren())
        ctx.setMaxMarginTop(0, 0);

    if (!child)
        marginInfo.setMargin(0, 0);

    while (child) {
        STARFISH_ASSERT(child->isNormalFlow());
        yAbsPosition = loc.y() + normalFlowHeight + top;
        child->layout(ctx, Frame::LayoutWantToResolve::ResolveWidth);
        bool isSelfCollapsing = child->asFrameBox()->isSelfCollapsingBlock(ctx);
        LayoutUnit oldMaxPositiveMarginTop = marginInfo.maxPositiveMarginTop();
        LayoutUnit oldMaxNegativeMarginTop = marginInfo.maxNegativeMarginTop();

        estimateLogicalPosition(child, ctx, marginInfo, isSelfCollapsing, false);
        LayoutUnit advanceY = child->asFrameBox()->marginCollapseResult().m_advanceY;

        Length marginLeft = child->style()->marginLeft();
        Length marginRight = child->style()->marginRight();
        floatAffected = false;
        clearAffected = false;

        setXForDirection(this, child->asFrameBox(), direction);

        reLayoutFrameBox:
        size_t floatIdx;
        LayoutUnit clearedDistanceToFloatBottom = ctx.clearedDistanceToFloatBottom(yAbsPosition, child->style()->clear(), &floatIdx);

        if (clearedDistanceToFloatBottom > 0) {
            // TODO : Study more cases which can set clearAffected to true.
            if (clearedDistanceToFloatBottom > child->asFrameBox()->marginTop() || !isSelfCollapsing) {
                clearAffected = true;
                if (marginInfo.canCollapseWithMarginTop()) {
                    marginInfo.setMaxPositiveMarginTop(oldMaxPositiveMarginTop);
                    marginInfo.setMaxNegativeMarginTop(oldMaxNegativeMarginTop);
                }
                MarginCollapseResult r = child->asFrameBox()->marginCollapseResult();
                r.m_advanceY = clearedDistanceToFloatBottom;
                child->asFrameBox()->setMarginCollapseResult(r);
                advanceY = child->asFrameBox()->marginCollapseResult().m_advanceY;
            }
        }

        child->asFrameBox()->setY(normalFlowHeight + top + advanceY);

        size_t floatSize = ctx.floatBoxesSize();
        clearedDistanceToFloatBottom = ctx.clearedDistanceToFloatBottom(yAbsPosition + advanceY, BothClearValue);
        // TODO : Study more cases which can make margin collapse ignored.
        bool ignoreMarginCollapse = (clearAffected && marginInfo.canCollapseWithMarginTop() && ctx.canFloatCollapseWithMarginTop(floatIdx))
            || (isSelfCollapsing && (wasClearedSelfCollapsingBlock || clearAffected));
        bool reLayoutNeeded = clearedDistanceToFloatBottom > 0;

        child->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
        // Basically, isSelfCollapsing isn't changed. But for the case that this margin collapse of child box is ignored, and the margin parts
        // is included to height, even though it was originally self collapsing, but it shouldn't behave like that.
        isSelfCollapsing = child->asFrameBox()->isSelfCollapsingBlock(ctx);

        std::pair<LayoutUnit, LayoutUnit> tops = estimateLogicalPosition(child, ctx, marginInfo, isSelfCollapsing, ignoreMarginCollapse);
        if (advanceY != child->asFrameBox()->marginCollapseResult().m_advanceY) {
            if (clearAffected && !ignoreMarginCollapse) {
                // Already advanceY is calculated considering clearance, if the real margin collapsed y position is less than advanceY,
                // this position repeatedly is pulled down to the advanceY. Therefore, in this case, relayout should not happen.
                if (advanceY < child->asFrameBox()->marginCollapseResult().m_advanceY) {
                    reLayoutNeeded |= clearedDistanceToFloatBottom > 0;
                    advanceY = child->asFrameBox()->marginCollapseResult().m_advanceY;
                } else {
                    reLayoutNeeded = false;
                }
            } else {
                reLayoutNeeded |= clearedDistanceToFloatBottom > 0;
                advanceY = child->asFrameBox()->marginCollapseResult().m_advanceY;
            }

            if (reLayoutNeeded) {
                ctx.unregisterFloatingBoxes(floatSize);
                goto reLayoutFrameBox;
            } else {
                child->asFrameBox()->setY(normalFlowHeight + top + advanceY);
                // if the y position of box has any changes, then the cached position of float boxes which it is going to be used next time
                // should also be recached.
                ctx.reCacheFloatBoxes(floatSize);
            }
        }

        if (shouldAvoidFlaots(child)) {
            bool hasToStretchWidth = shouldStretchWidth(child);
            reLayoutNeeded = false;
            floatAffected = false;
            LayoutLocation loc = absolutePoint(ctx.frameDocument());
            // TODO: Consider Rtl
            LayoutUnit leftBoundary = loc.x();
            LayoutUnit rightBoundary = leftBoundary + contentWidth();
            LayoutLocation selfLoc = child->asFrameBox()->absolutePoint(ctx.frameDocument());
            LayoutUnit originalY = selfLoc.y();
            rePositionFloatAvoidingFrameBox:
            std::pair<LayoutUnit, LayoutUnit> boundaries = ctx.floatingBoxBoundary(selfLoc.y(),
                child->asFrameBox()->height(), leftBoundary, rightBoundary);
            if ((boundaries.first != leftBoundary && selfLoc.x() < boundaries.first)
                || (boundaries.second != rightBoundary && selfLoc.x() + child->asFrameBox()->width() > boundaries.second)) {
                floatAffected |= true;
            }

            if (floatAffected) {
                LayoutUnit width = boundaries.second - boundaries.first;
                LayoutUnit nextDistanceToFloatBottom;
                if ((hasToStretchWidth && child->asFrameBox()->contentWidth() + width - child->asFrameBox()->width() > 0)
                    || (width >= child->asFrameBox()->width())
                    || ((nextDistanceToFloatBottom = ctx.nextDistanceToFloatBottom(selfLoc.y(), child->asFrameBox()->height())) == 0)) {
                    child->asFrameBox()->moveY(selfLoc.y() - originalY);
                    if (hasToStretchWidth) {
                        child->asFrameBox()->moveX(boundaries.first - selfLoc.x());
                        reLayoutNeeded = width != child->asFrameBox()->width();
                        child->asFrameBox()->setContentWidth(child->asFrameBox()->contentWidth() + width - child->asFrameBox()->width());
                    } else {
                        if (boundaries.first != leftBoundary && selfLoc.x() < boundaries.first) {
                            child->asFrameBox()->moveX(boundaries.first - selfLoc.x());
                        }

                        if (boundaries.second != rightBoundary && selfLoc.x() + child->asFrameBox()->width() > boundaries.second) {
                            child->asFrameBox()->moveX(boundaries.second - selfLoc.x() - child->asFrameBox()->width());
                        }
                    }

                    if (reLayoutNeeded) {
                        // In this case, the float box recache isn't needed.
                        // The only possible case there has floating box is the child box is FrameBlockBox, and this box
                        // established BlockFormattingContext. This means when the layout is finished, the floating box cached vector
                        // is already gone, because the vector belongs to block formatting context.
                        goto reLayoutFrameBox;
                    }
                } else {
                    selfLoc.setY(selfLoc.y() + nextDistanceToFloatBottom);
                    goto rePositionFloatAvoidingFrameBox;
                }
            }
        }

        marginCollapse(child, ctx, marginInfo, isSelfCollapsing, tops.first, tops.second);

        if (isSelfCollapsing) {
            // ------------- <- containing block (CB)
            // | --------- |
            // | | float | |
            // | --------- |
            // | --------- |
            // | | clear | | <- self collapsing clear affected Block (SCC)
            // | --------- |
            // | --------- |
            // | | block | | <- self collapsing blocks (SCB1, SCB2, ...)
            // | --------- |
            // |    ...    |
            // -------------
            //
            // The height of CB is determined by this expression.
            // SCBn.maxMargin = max(SCBn.marginTop, SCBn.marginBottom)
            // height = max(max(max(SCB1.maxMargin, SCB2.maxMargin, ..., SCBn.maxMargin), SCC.matginTop), 0)
            //
            if (clearAffected) {
                yPositionOfClearedSelfCollapsingBlock = child->asFrameBox()->y();
                marginTopOfClearedSelfCollapsingBlock = child->asFrameBox()->marginTop();
                maxMarginOfSelfCollapsingBlocks = child->asFrameBox()->marginBottom();
                if (marginTopOfClearedSelfCollapsingBlock < 0)
                    marginTopOfClearedSelfCollapsingBlock = 0;
                if (maxMarginOfSelfCollapsingBlocks < 0)
                    maxMarginOfSelfCollapsingBlocks = 0;
                wasClearedSelfCollapsingBlock = true;
                markHeightComputed(true);
            } else if (wasClearedSelfCollapsingBlock) {
                maxMarginOfSelfCollapsingBlocks = std::max(maxMarginOfSelfCollapsingBlocks, child->asFrameBox()->marginTop());
                maxMarginOfSelfCollapsingBlocks = std::max(maxMarginOfSelfCollapsingBlocks, child->asFrameBox()->marginBottom());
            }

            if (wasClearedSelfCollapsingBlock) {
                LayoutUnit height = maxMarginOfSelfCollapsingBlocks - marginTopOfClearedSelfCollapsingBlock;
                if (height < 0)
                    height = 0;
                if (maxNormalFlowBottom < height + yPositionOfClearedSelfCollapsingBlock)
                    maxNormalFlowBottom = height + yPositionOfClearedSelfCollapsingBlock;
                normalFlowHeight = height + yPositionOfClearedSelfCollapsingBlock - top;
            }
        } else {
            wasClearedSelfCollapsingBlock = false;
            if (maxNormalFlowBottom < child->asFrameBox()->height() + child->asFrameBox()->y())
                maxNormalFlowBottom = child->asFrameBox()->height() + child->asFrameBox()->y();
            normalFlowHeight = child->asFrameBox()->height() + child->asFrameBox()->y() - top;
        }

        child = child->next();
    }

    // NOTE: At this point, ctx.max[P/N]MarginTop has the collapsed margin
    // between collapsible ancestors and first descendants.
    ctx.setMaxMarginTop(marginInfo.maxPositiveMarginTop(), marginInfo.maxNegativeMarginTop());
    // NOTE: At this point, ctx.max[P/N]MarginBottom has the collapsed margin
    // between this block and last descendants.
    ctx.setMaxMarginBottom(marginInfo.positiveMargin(), marginInfo.negativeMargin());
    if (!marginInfo.canCollapseWithMarginBottom()) {
        m_marginCollapseResult.m_normalFlowHeightAdvance = ctx.maxPositiveMarginBottom() - ctx.maxNegativeMarginBottom();
        ctx.setMaxMarginBottom(0, 0);
    }

    normalFlowHeight = maxNormalFlowBottom - top + m_marginCollapseResult.m_normalFlowHeightAdvance;
    return normalFlowHeight;
}

}
