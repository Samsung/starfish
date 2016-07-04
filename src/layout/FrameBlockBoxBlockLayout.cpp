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
#include "FrameBlockBox.h"
#include "FrameText.h"
#include "FrameInline.h"

namespace StarFish {


std::pair<LayoutUnit, LayoutRect> FrameBlockBox::layoutBlock(LayoutContext& ctx)
{
    LayoutUnit top = paddingTop() + borderTop();
    LayoutUnit bottom = paddingBottom() + borderBottom();
    // NOTE: maxNormalFlowBottom : Maximum position after finishing layout for all children
    // normalFlowPosition : Current Position on normal flow
    LayoutUnit normalFlowHeight = 0, maxNormalFlowBottom = top, normalFlowPosition = top;
    LayoutRect visibleRect(0, 0, 0, 0);
    MarginInfo marginInfo(top, bottom, isEstablishesBlockFormattingContext() || isFrameDocument(), style()->height());
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
    bool hasOnlySelfCollapsing = true;
    DirectionValue direction = style()->direction();
    while (child) {
        // Set initial position for resolve child width when position: absolute
        if (!child->isNormalFlow()) {
            if (direction == LtrDirectionValue) {
                child->asFrameBox()->setX(paddingLeft() + borderLeft());
            } else {
                child->asFrameBox()->setX(width() - borderRight() - paddingRight());
            }
        }

        child->layout(ctx, Frame::LayoutWantToResolve::ResolveWidth);

        // Place the child.
        if (child->isNormalFlow()) {
            Length marginLeft = child->style()->marginLeft();
            Length marginRight = child->style()->marginRight();
            LayoutUnit mX = 0;
            if (direction == LtrDirectionValue) {
                if (!marginLeft.isAuto() && !marginRight.isAuto()) {
                    mX = child->asFrameBox()->marginLeft();
                } else if (marginLeft.isAuto() && !marginRight.isAuto()) {
                } else if (!marginLeft.isAuto() && marginRight.isAuto()) {
                    mX = child->asFrameBox()->marginLeft();
                } else {
                    // auto-auto
                    mX = child->asFrameBox()->marginLeft();
                }
            } else {
                if (!marginLeft.isAuto() && !marginRight.isAuto()) {
                    mX = child->asFrameBox()->marginRight();
                } else if (marginLeft.isAuto() && !marginRight.isAuto()) {
                    mX = child->asFrameBox()->marginRight();
                } else if (!marginLeft.isAuto() && marginRight.isAuto()) {
                } else {
                    // auto-auto
                    mX = child->asFrameBox()->marginRight();
                }
            }

            if (direction == LtrDirectionValue) {
                child->asFrameBox()->setX(paddingLeft() + borderLeft() + mX);
            } else {
                child->asFrameBox()->setX(width() - child->asFrameBox()->width() - mX - borderRight() - paddingRight());
            }
        }

        child->asFrameBox()->setY(normalFlowHeight + top);

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
            normalFlowPosition = child->asFrameBox()->y() + child->asFrameBox()->height() + child->asFrameBox()->marginBottom();
            visibleRect.unite(child->asFrameBox()->visibleRect());
        } else {
            child->asFrameBox()->setY(normalFlowPosition);
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
