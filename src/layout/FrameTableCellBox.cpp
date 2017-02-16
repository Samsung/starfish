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

#include "FrameTableBox.h"
#include "FrameTableRowBox.h"
#include "FrameTableSectionBox.h"
#include "FrameTreeBuilder.h"
#include "FrameText.h"

namespace StarFish {

FrameTableCellBox::FrameTableCellBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
{

}

FrameTableCellBox* FrameTableCellBox::buildFrameTableCell(Node* current, FrameTreeBuilderContext& ctx, bool force)
{
    FrameBlockBox* parent = ctx.currentBlockContainer();
    FrameTableCellBox* tableCell;
    bool isTableCell = current->style()->display() == DisplayValue::TableCellDisplayValue;

    if (isTableCell) {
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

    if (isTableCell) {
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

void FrameTableCellBox::calCellWidth(LayoutContext& ctx, unsigned pos, Frame::LayoutWantToResolve resolveWhat)
{
    FrameBlockBox::layout(ctx, Frame::LayoutWantToResolve::ResolveWidth);

    // If "table-layout: auto", calculate min/max cell widths.
    // If "table-layout: fixed", and the top cell in the first row has
    // values other than "width: auto", we use the fixed value from the cell.
    FrameTableBox* table = rowBox()->sectionBox()->tableBox();
    if (table->style()->tableLayout() == TableLayoutValue::AutoTableLayoutValue) {
        if (style()->width().isAuto()) {
            m_minCellWidth = calMinCellWidth(ctx);
            m_maxCellWidth = calMaxCellWidth(ctx);
        } else if (style()->width().isFixed()) {
            LayoutUnit width =
                LayoutUnit::fromPixel(style()->width().fixed());
            m_minCellWidth = std::max(width, calMinCellWidth(ctx));
            m_maxCellWidth = std::max(width, calMaxCellWidth(ctx));
        } else if (style()->width().isPercent()) {
            // TODO
        } else {
            // Should not be here
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else if (table->style()->tableLayout() == TableLayoutValue::FixedTableLayoutValue) {
        std::vector<FrameTableCellBox*>& cellsInTheFirstRow =
            rowBox()->sectionBox()->tableBox()->cellsInTheFirstRow();

        // This row may contain more columns than the first row in the table.
        // In this case, the spec,
        // https://www.w3.org/TR/CSS21/tables.html#fixed-table-layout,
        // says we can stop rendering additional cells. But, we continue to
        // layout these additional cells, following blink's behaviour.
        FrameTableCellBox* matchingCellInTheFirstRow = nullptr;
        if (pos < cellsInTheFirstRow.size()) {
            matchingCellInTheFirstRow = cellsInTheFirstRow[pos];
        }

        if (!matchingCellInTheFirstRow || matchingCellInTheFirstRow->style()->width().isAuto()) {
            m_minCellWidth = calMinCellWidth(ctx);
            m_maxCellWidth = calMaxCellWidth(ctx);
        } else {
            if (matchingCellInTheFirstRow->style()->width().isFixed()) {
                LayoutUnit width =
                    LayoutUnit::fromPixel(matchingCellInTheFirstRow->style()->width().fixed());
                m_minCellWidth = std::max(width, calMinCellWidth(ctx));
                m_maxCellWidth = width;
            } else if (matchingCellInTheFirstRow->style()->width().isPercent()) {
                // TODO
            } else {
                // Should not be here
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }
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

void FrameTableCellBox::applyVerticalAlign()
{
    // 1. Cal the content height of all child boxes
    //    Also calculate the ascender of the first line. It is used
    //    in "vertical-align: baseline"
    LayoutUnit childContentHeight = 0;
    LayoutUnit ascenderOfTheFirstLineBox = 0;
    if (hasBlockFlow() && firstChild()) {
        STARFISH_ASSERT(firstChild()->isFrameBlockBox());
        STARFISH_ASSERT(lastChild()->isFrameBlockBox());
        FrameBlockBox* firstBox = firstChild()->asFrameBlockBox();
        FrameBlockBox* lastBox = lastChild()->asFrameBlockBox();
        LayoutUnit yStart = firstBox->y() - firstBox->marginTop();
        LayoutUnit yEnd = lastBox->y() + lastBox->height() + lastBox->marginBottom();
        childContentHeight = yEnd - yStart;
        if (!firstBox->lineBoxes().empty()) {
            ascenderOfTheFirstLineBox = firstBox->lineBoxes()[0]->ascender();
        }
    } else {
        if (!m_lineBoxes.empty()) {
            LineBox* firstBox = m_lineBoxes[0];
            LineBox* lastBox = m_lineBoxes[m_lineBoxes.size()-1];
            LayoutUnit yStart = firstBox->y();
            LayoutUnit yEnd = lastBox->y() + lastBox->height();
            childContentHeight = yEnd - yStart;
            ascenderOfTheFirstLineBox = firstBox->ascender();
        }
    }

    // 2. Cal y pos where the first child box will be positioned
    LayoutUnit yPosOffset = 0;
    switch (style()->verticalAlign()) {
    case VerticalAlignValue::TopVAlignValue:
        yPosOffset = 0;
        break;
    case VerticalAlignValue::BottomVAlignValue:
        yPosOffset = contentHeight() - childContentHeight;
        break;
    case VerticalAlignValue::MiddleVAlignValue: {
        LayoutUnit halfCellContentHeight = LayoutUnit(contentHeight().toDouble() / 2);
        yPosOffset = halfCellContentHeight.toDouble() - (childContentHeight.toDouble() / 2);
        break;
    }
    case VerticalAlignValue::BaselineVAlignValue:
        yPosOffset = rowBox()->baseline() - ascenderOfTheFirstLineBox;
        yPosOffset -= borderTop() + paddingTop();
        break;
    default:
        break;
    }

    // 3. Move all child boxes by yPosOffset
    if (hasBlockFlow()) {
        for (Frame* c = firstChild(); c; c = c->next()) {
            if (c->isFrameBlockBox()) {
                c->asFrameBox()->moveY(yPosOffset.round());
            }
        }
    } else {
        for (auto& b : m_lineBoxes) {
            b->moveY(yPosOffset.round());
        }
    }
}

LayoutUnit FrameTableCellBox::calBaseline()
{
    // baseline is only calculated when this cell has "vertical-align: baseline"
    if (style()->verticalAlign() != VerticalAlignValue::BaselineVAlignValue) {
        return 0;
    }

    LineBox* firstLineBox = nullptr;
    if (hasBlockFlow() && firstChild()) {
        STARFISH_ASSERT(firstChild()->isFrameBlockBox());
        FrameBlockBox* box = firstChild()->asFrameBlockBox();
        if (!box->lineBoxes().empty()) {
            firstLineBox = box->lineBoxes()[0];
        }
    } else {
        if (!m_lineBoxes.empty()) {
            firstLineBox = m_lineBoxes[0];
        }
    }

    if (firstLineBox == nullptr) {
        return 0;
    }

    return firstLineBox->y() + firstLineBox->ascender();
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
