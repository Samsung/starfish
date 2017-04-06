/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "FrameBlockBoxInlineLayout.h"

namespace StarFish {

FrameTableCellBox::FrameTableCellBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
    , m_minCellWidth(0)
    , m_maxCellWidth(0)
    , m_actualContentHeight(0)
{
}

FrameTableCellBox* FrameTableCellBox::buildFrameTableCell(
    Node* current, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTableCellBox* currentFrame = nullptr;
    FrameBlockBox* parent = ctx.currentBlockContainer();
    bool isTableCell =
        current->style()->display() == DisplayValue::TableCellDisplayValue;

    if (isTableCell) {
        currentFrame = new FrameTableCellBox(current, nullptr);
        current->setFrame(currentFrame);
        ctx.setCurrentBlockContainer(currentFrame);
        ctx.mergeTextDecorationData(currentFrame->style());
        for (Node* c = current->firstChild(); c; c = c->nextSibling()) {
            FrameTreeBuilder::buildTree(c, ctx, force);
        }
    } else {
        // please read comment in FrameTableBox::buildFrameTable
        Frame* before = parent->lastChild();
        if (before && before->isAnonymous() && before->isFrameTableCellBox()) {
            currentFrame = before->asFrameTableCellBox();
        } else {
            currentFrame =
                FrameTableCellBox::createAnonymousWithParent(parent, current);
        }
        ctx.setCurrentBlockContainer(currentFrame);
        ctx.mergeTextDecorationData(currentFrame->style());
        FrameTreeBuilder::buildTree(current, ctx, force);
    }

    ctx.setCurrentBlockContainer(parent);

    FrameTreeBuilder::createPseudoElementIfNeeded(
        current, StyleResolver::PseudoElementType::PseudoElementFirstLetter,
        ctx);

    STARFISH_ASSERT(currentFrame);
    return currentFrame;
}

FrameTableCellBox* FrameTableCellBox::createAnonymousWithParent(
    FrameBlockBox* parent, Node* parentNode)
{
    ComputedStyle* style = new ComputedStyle(parent->style());
    style->setDisplay(DisplayValue::TableCellDisplayValue);
    style->loadResources(parentNode);
    style->arrangeStyleValues(parent->style(), parentNode);
    return new FrameTableCellBox(nullptr, style);
}

// We follow the CSS definition of width in the implementation.
// The width of a cell refers to the content width, which excludes the border
// and padding, e.g., <td style="width: 100px">
//
//  +--------------------------+
//  |          border          |
//  |  +--------------------+  |
//  |  |  Width of the cell |  |
//  |  |        100px       |  |
//  |  |<------------------>|  |
//  |  |                    |  |
//  |  +--------------------+  |
//  |                          |
//  +--------------------------+
//
void FrameTableCellBox::calCellWidth(LayoutContext& ctx,
                                     Frame::LayoutWantToResolve resolveWhat)
{
    FrameBlockBox::layout(ctx, Frame::LayoutWantToResolve::ResolveWidth);

    if (style()->width().isAuto() || style()->width().isFixed() ||
        style()->width().isPercent()) {
        PreferredWidthContext p(ctx, LayoutUnit::max());
        computePreferredWidth(p);
        p.finishLine(false);
        m_minCellWidth = p.preferredMinWidth() + borderWidth() + paddingWidth();
        m_maxCellWidth = p.preferredWidth() + borderWidth() + paddingWidth();
    } else {
        // Should not be here
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

void FrameTableCellBox::layout(LayoutContext& ctx,
                               Frame::LayoutWantToResolve resolveWhat)
{
    // This method should not be called, as table uses its own layout algorithm
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

// Table draws the border around the TableFrameSections
void FrameTableCellBox::paintBackgroundAndBorders(Canvas* canvas)
{
    // Do not print borders and background if the cell width is 0
    if (width() > 0) {
        Color bgColor;
        if (bgColorFromAttribute(&bgColor)) {
            style()->setBackgroundColor(bgColor);
        }

        FrameBox::paintBackgroundAndBorders(canvas);
    }
}

void FrameTableCellBox::applyVerticalAlign()
{
    // 1. Cal the content height of all child boxes
    //    Also calculate the ascender of the first line. It is used
    //    in "vertical-align: baseline"
    // 2. Cal y pos where the first child box will be positioned
    LayoutUnit yPosOffset = 0;
    switch (style()->verticalAlign()) {
    case VerticalAlignValue::TopVAlignValue:
        yPosOffset = 0;
        break;
    case VerticalAlignValue::BottomVAlignValue:
        yPosOffset = contentHeight() - m_actualContentHeight;
        break;
    case VerticalAlignValue::MiddleVAlignValue: {
        LayoutUnit halfCellContentHeight =
            LayoutUnit(contentHeight().toDouble() / 2);
        yPosOffset = halfCellContentHeight.toDouble() -
                     (m_actualContentHeight.toDouble() / 2);
        break;
    }
    case VerticalAlignValue::BaselineVAlignValue:
    case VerticalAlignValue::SubVAlignValue:
    case VerticalAlignValue::SuperVAlignValue:
    case VerticalAlignValue::TextTopVAlignValue:
    case VerticalAlignValue::TextBottomVAlignValue:
    case VerticalAlignValue::NumericVAlignValue:
        yPosOffset = rowBox()->baseline() - calBaseline();
        break;
    default:
        break;
    }

    // 3. Move all child boxes by yPosOffset
    if (hasBlockFlow()) {
        for (Frame* c = firstChild(); c; c = c->next()) {
            if (c->isFrameBlockBox()) {
                c->asFrameBox()->moveY(yPosOffset);
            }
        }
    } else {
        for (auto& b : m_lineBoxes) {
            b->moveY(yPosOffset);
        }
    }
}

LayoutUnit FrameTableCellBox::calBaseline()
{
    // To reduce unneeded computation, baseline is only calculated when
    // this cell has "vertical-align: baseline" or equivalent
    switch (style()->verticalAlign()) {
    case VerticalAlignValue::BaselineVAlignValue:
    case VerticalAlignValue::SubVAlignValue:
    case VerticalAlignValue::SuperVAlignValue:
    case VerticalAlignValue::TextTopVAlignValue:
    case VerticalAlignValue::TextBottomVAlignValue:
    case VerticalAlignValue::NumericVAlignValue: {
        LineBox* flb = firstLineBox();
        if (flb) {
            return flb->absolutePoint(this).y() + flb->ascender();
        }
        break;
    }
    default:
        break;
    }
    return 0;
}

unsigned FrameTableCellBox::colspan()
{
    String* colspan = String::emptyString;
    if (isAnonymous()) {
        // FIX ME
        return 1;
    } else if (node()->asElement()->asHTMLElement()->isHTMLTDElement()) {
        colspan =
            node()->asElement()->asHTMLElement()->asHTMLTDElement()->colspan();
    } else if (node()->asElement()->asHTMLElement()->isHTMLTHElement()) {
        colspan =
            node()->asElement()->asHTMLElement()->asHTMLTHElement()->colspan();
    } else {
        // colspan is only accepted when HTML element is either <td> or <th>,
        // hence it is not applied when used in other elements.
        // e.g., <div style="display: table-cell" colspan="2">
        // In this case, we ignore the colspan value
    }

    int num = String::parseInt(colspan);
    // If colspan is not defined, use 1 as the default value
    // Negative colspan value is ignored
    return num <= 0 ? 1 : num;
}
}
