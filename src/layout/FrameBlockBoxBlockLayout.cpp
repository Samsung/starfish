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
    LayoutUnit normalFlowHeight = 0, maxNormalFlowBottom = top, normalFlowPosition = top;
    LayoutRect visibleRect(0, 0, 0, 0);
    Frame* child = firstChild();
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

        // Place the child.
        if (child->isNormalFlow()) {
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
        }

        child->asFrameBox()->setY(normalFlowHeight + top);

        if (child->isNormalFlow()) {
            // Lay out the child
            child->asFrameBox()->moveY(child->asFrameBox()->marginCollapseResult().m_advanceY);
            child->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);

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

    normalFlowHeight = maxNormalFlowBottom - top + m_marginCollapseResult.m_normalFlowHeightAdvance;
    return std::make_pair(normalFlowHeight, visibleRect);
}

}
