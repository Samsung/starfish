/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "StarFishConfig.h"
#include "core/layout/FrameGridBox.h"

namespace StarFish {

GridFormattingContext::GridFormattingContext(LayoutContext& ctx,
                                             FrameGridBox* container,
                                             LayoutUnit availableWidth)
    : m_layoutContext(ctx)
    , m_container(container)
    , m_availableWidth(availableWidth)
{
}

void GridFormattingContext::computeColumnsAndRows()
{
    std::vector<FrameBox*> orderedGridItems;
    Frame* child = m_container->firstChild();

    while (child) {
        if (child->isGridItem()) {
            orderedGridItems.push_back(child->asFrameBox());
        }
        child = child->next();
    }

    std::stable_sort(orderedGridItems.begin(), orderedGridItems.end(),
                     [](FrameBox* a, FrameBox* b) {
                         return a->style()->order() < b->style()->order();
                     });

    auto item = orderedGridItems.begin();

    // Apply 'y' coordinate, but we have to make grid properties.
    LayoutUnit y(0);
    while (item != orderedGridItems.end()) {
        FrameBox* gridItem = (*item);
        gridItem->layout(m_layoutContext,
                         Frame::LayoutWantToResolve::ResolveAll);

        gridItem->setY(y);
        y += gridItem->height();
        item++;
    }
}

bool GridFormattingContext::doesParticipateInGridFormattingContext(
    Frame* gridItem)
{
    if (gridItem->isFrameBlockBox() && gridItem->isAnonymous()) {
        FrameBlockBox* blockBox = gridItem->asFrameBlockBox();
        Frame* child = blockBox->firstChild();
        while (child) {
            // If the entire sequence of child text runs contains only white
            // space,
            // it is instead not rendered.
            if (!(child->isFrameText() &&
                  child->asFrameText()->text()->containsOnlyWhitespace())) {
                return true;
            }

            child = child->next();
        }

        return false;
    }

    return true;
}

FrameGridBox::FrameGridBox(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
}

void FrameGridBox::layoutGrid(LayoutContext& ctx)
{
    GridFormattingContext gridFormattingContext(ctx, this, contentWidth());
    gridFormattingContext.computeColumnsAndRows();
}
}
