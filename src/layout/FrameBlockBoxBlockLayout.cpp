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


LayoutUnit FrameBlockBox::layoutBlock(LayoutContext& ctx)
{
    LayoutUnit top = paddingTop() + borderTop();
    LayoutUnit bottom = paddingBottom() + borderBottom();
    LayoutUnit normalFlowHeight = 0, maxNormalFlowBottom = top, normalFlowPosition = top;
    Frame* child = firstChild();
    DirectionValue direction = style()->direction();

    while (child) {
        STARFISH_ASSERT(child->isNormalFlow());

        Length marginLeft = child->style()->marginLeft();
        Length marginRight = child->style()->marginRight();
        LayoutUnit mX = 0;
        if (direction == LtrDirectionValue) {
            mX = child->asFrameBox()->marginLeft();
            child->asFrameBox()->setX(paddingLeft() + borderLeft() + mX);
        } else {
            mX = child->asFrameBox()->marginRight();
            child->asFrameBox()->setX(width() - child->asFrameBox()->width() - mX - borderRight() - paddingRight());
        }

        child->asFrameBox()->setY(normalFlowHeight + top);
        child->asFrameBox()->moveY(child->asFrameBox()->marginCollapseResult().m_advanceY);

        if (child->isEstablishesBlockFormattingContext()) {
            bool widthIsAuto = child->style()->width().isAuto();
            bool floatAffected = false;
            LayoutLocation loc = absolutePoint(ctx.frameDocument());
            // TODO: Consider Rtl
            LayoutUnit leftBoundary = loc.x() + paddingLeft() + borderLeft() + child->asFrameBox()->marginLeft();
            LayoutUnit rightBoundary = leftBoundary + contentWidth() - child->asFrameBox()->marginRight();
            LayoutLocation selfLoc = child->asFrameBox()->absolutePoint(ctx.frameDocument());
            LayoutUnit originalY = selfLoc.y();
            positionEstablishedBlockFormatContextBox:
            std::pair<LayoutUnit, LayoutUnit> boundaries = ctx.floatingBoxBoundary(selfLoc.y(), leftBoundary, rightBoundary);
            floatAffected |= boundaries.first != leftBoundary || boundaries.second != rightBoundary;

            if (floatAffected) {
                LayoutUnit width = boundaries.second - boundaries.first;
                LayoutUnit yDiff = ctx.heightDueTofloatingBoxes(selfLoc.y());
                if ((widthIsAuto && child->asFrameBox()->contentWidth() + width - child->asFrameBox()->width() > 0)
                    || width > child->asFrameBox()->width() + child->asFrameBox()->marginWidth()
                    || yDiff == 0) {
                    child->asFrameBox()->moveX(boundaries.first - selfLoc.x());
                    child->asFrameBox()->moveY(selfLoc.y() - originalY);
                    if (widthIsAuto) {
                        child->asFrameBox()->setContentWidth(child->asFrameBox()->contentWidth() + width - child->asFrameBox()->width());
                    }
                } else {
                    selfLoc.setY(selfLoc.y() + yDiff);
                    goto positionEstablishedBlockFormatContextBox;
                }
            }
        }

        child->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);

        if (!child->asFrameBox()->isSelfCollapsingBlock(ctx)) {
            if (maxNormalFlowBottom < child->asFrameBox()->height() + child->asFrameBox()->y())
                maxNormalFlowBottom = child->asFrameBox()->height() + child->asFrameBox()->y();
            normalFlowHeight = child->asFrameBox()->height() + child->asFrameBox()->y() - top;
        }
        normalFlowPosition = child->asFrameBox()->y() + child->asFrameBox()->height() + child->asFrameBox()->marginBottom();

        child = child->next();
    }

    normalFlowHeight = maxNormalFlowBottom - top + m_marginCollapseResult.m_normalFlowHeightAdvance;
    return normalFlowHeight;
}

}
