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
#include "FrameTableCellBox.h"

#include "FrameTreeBuilder.h"
#include "FrameText.h"

namespace StarFish {

FrameTableCellBox::FrameTableCellBox(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
    STARFISH_ASSERT((node == nullptr && style != nullptr)
        || (node != nullptr && style == nullptr));
}

FrameTableCellBox* FrameTableCellBox::buildFrameTableCell(Node* current, FrameTreeBuilderContext& ctx, bool force)
{
    FrameBlockBox* parent = ctx.currentBlockContainer();
    FrameTableCellBox* tableCell;

    if (current->isTableCell()) {
        tableCell = new FrameTableCellBox(current, nullptr);
        current->setFrame(tableCell);
    } else {
        // current node is not tableRow then make anonymous Row or
        // reuse last anonymous Cell
        Frame* before = parent->lastChild();
        if (before && before->isAnonymous() && before->isFrameTableCellBox()) {
            tableCell = before->asFrameTableCellBox();
        } else {
            tableCell = FrameTableCellBox::createAnonymousWithParent(parent, current);
        }
    }
    ctx.setCurrentBlockContainer(tableCell);
    ctx.mergeTextDecorationData(tableCell->style());

    if (current->isTableCell()) {
        for (Node* c = current->firstChild(); c; c = c->nextSibling()) {
            FrameTreeBuilder::buildTree(c, ctx, force);
        }
    } else if (tableCell->isAnonymous()) {
        Frame* childFrame = FrameTreeBuilder::buildTree(current, ctx, force);
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }

    ctx.setCurrentBlockContainer(parent);

    return tableCell->parent() ? nullptr : tableCell;
}

void FrameTableCellBox::calCellWidth(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    FrameBlockBox::layout(ctx, Frame::LayoutWantToResolve::ResolveWidth);

    if (style()->width().isAuto()) {
        m_minCellWidth = calMinCellWidth(ctx);
        m_maxCellWidth = calMaxCellWidth(ctx);
    } else if (style()->width().isFixed()) {
        LayoutUnit width = LayoutUnit::fromPixel(style()->width().fixed());
        m_minCellWidth = std::max(width, calMinCellWidth(ctx));
        m_maxCellWidth = std::max(width, calMaxCellWidth(ctx));
    } else if (style()->width().isPercent()) {
        // TODO
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void FrameTableCellBox::layoutWidth(LayoutContext& ctx)
{
    // layout blockboxes to fit them into the width of this cell.
    // The width of cell has been calculated in calCellWidth()
    // in the first iteration
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameBlockBox()) {
            c->layout(ctx, Frame::LayoutWantToResolve::ResolveWidth);
        }
    }
}

void FrameTableCellBox::layoutHeight(LayoutContext& ctx)
{
    FrameBlockBox::layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
}

void FrameTableCellBox::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    // This method should not be called, as table uses its own layout algorithm
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

LayoutUnit FrameTableCellBox::calMinCellWidth(LayoutContext& ctx)
{
    LayoutUnit maxWidthSoFar = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        LayoutUnit width;
        if (c->isFrameText()) {
            width = c->asFrameText()->preferredMinWidth(ctx);
        } else if (c->isFrameBlockBox()) {
            LayoutUnit parentContentWidth = ctx.parentContentWidth(this);
            PreferredWidthContext p(ctx, 0, 0);
            c->computePreferredWidth(p);
            width = p.preferredMinWidth();
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        width += paddingLeft() + paddingRight();
        width += borderLeft() + borderRight();
        maxWidthSoFar = std::max(maxWidthSoFar, width);
    }

    return maxWidthSoFar;
}

LayoutUnit FrameTableCellBox::calMaxCellWidth(LayoutContext& ctx)
{
    return calPreferredFrameWidth(ctx, this);
}

LayoutUnit FrameTableCellBox::calPreferredFrameWidth(LayoutContext& ctx, FrameBlockBox* b)
{
    LayoutUnit maxWidthSoFar = 0;
    for (Frame* c = b->firstChild(); c; c = c->next()) {
        LayoutUnit width;
        if (c->isFrameText()) {
            width = c->asFrameText()->preferredWidth(ctx);
        } else if (c->isFrameBlockBox()) {
            width = calPreferredFrameWidth(ctx, c->asFrameBlockBox());
        }

        width += b->marginLeft() + b->marginRight();
        width += b->borderLeft() + b->borderRight();
        width += b->paddingLeft() + b->paddingRight();
        maxWidthSoFar = std::max(maxWidthSoFar, width);
    }

    return maxWidthSoFar;
}

int FrameTableCellBox::colspan()
{
    String* colspan = String::emptyString;
    if (isAnonymous()) {
        // FIX ME
        return 1;
    } else if (node()->asElement()->asHTMLElement()->isHTMLTDElement()) {
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
