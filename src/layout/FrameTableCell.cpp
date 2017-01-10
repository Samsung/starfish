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
#include "FrameTableCell.h"

#include "FrameTreeBuilder.h"
#include "FrameText.h"

namespace StarFish {

FrameTableCell::FrameTableCell(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
    STARFISH_ASSERT((node == nullptr && style != nullptr)
        || (node != nullptr && style == nullptr));
}

FrameTableCell* FrameTableCell::buildFrameTableCell(Node* cellNode, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTableCell* tableCell = new FrameTableCell(cellNode, nullptr);
    cellNode->setFrame(tableCell);

    FrameBlockBox* lastContext = ctx.currentBlockContainer();
    ctx.setCurrentBlockContainer(tableCell);

    for (Node* c = cellNode->firstChild(); c; c = c->nextSibling()) {
        FrameTreeBuilder::buildTree(c, ctx, force);
    }

    ctx.setCurrentBlockContainer(lastContext);

    return tableCell;
}

void FrameTableCell::calContentWidth(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    FrameBlockBox::layout(ctx, Frame::LayoutWantToResolve::ResolveWidth);

    if (style()->width().isAuto()) {
        m_minContentWidth = minimumCellWidth(ctx);
        m_maxContentWidth = maximumCellWidth(ctx);
    } else if (style()->width().isFixed()) {
        LayoutUnit width = LayoutUnit::fromPixel(style()->width().fixed());
        m_minContentWidth = std::max(width, minimumCellWidth(ctx));
        m_maxContentWidth = std::max(width, maximumCellWidth(ctx));
    } else if (style()->width().isPercent()) {
        // TODO
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void FrameTableCell::layoutWidth(LayoutContext& ctx)
{

}

void FrameTableCell::layoutHeight(LayoutContext& ctx)
{
    FrameBlockBox::layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
}

void FrameTableCell::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    // This method should not be called, as table uses its own layout algorithm
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

LayoutUnit FrameTableCell::minimumCellWidth(LayoutContext& ctx)
{
    LayoutUnit maxWidthSoFar = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        LayoutUnit width;
        if (c->isFrameText()) {
            width = c->asFrameText()->minimumContentWidth(ctx);
        } else {
            // TODO: measure width for other frameboxes
        }
        width += paddingLeft() + paddingRight();
        maxWidthSoFar = std::max(maxWidthSoFar, width);
    }

    return maxWidthSoFar;
}

LayoutUnit FrameTableCell::maximumCellWidth(LayoutContext& ctx)
{
    LayoutUnit maxWidthSoFar = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        LayoutUnit width;
        if (c->isFrameText()) {
            width = c->asFrameText()->maximumContentWidth(ctx);
        } else {
            // TODO
        }
        width += paddingLeft() + paddingRight();
        maxWidthSoFar = std::max(maxWidthSoFar, width);
    }

    return maxWidthSoFar;
}

int FrameTableCell::colspan()
{
    String* colspan = String::emptyString;
    if (node()->asElement()->asHTMLElement()->isHTMLTDElement()) {
        colspan = node()->asElement()->asHTMLElement()->asHTMLTDElement()->colspan();
    } else if (node()->asElement()->asHTMLElement()->isHTMLTHElement()) {
        colspan = node()->asElement()->asHTMLElement()->asHTMLTHElement()->colspan();
    } else {
        // colspan is only accepted when HTML element is either <td> or <th>,
        // hence it is not applied when used in other elements.
        // e.g., <div style="display: table-cell" colspan="2">
        // In this case, we ignore the colspan value
    }

    int num = String::parseInt(colspan);
    // If colspan is not defined, use 1 as the default value
    return num == 0? 1: num;
}

}
