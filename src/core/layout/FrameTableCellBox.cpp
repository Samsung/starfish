/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "core/dom/Node.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/HTMLTDElement.h"
#include "core/dom/HTMLTHElement.h"
#include "core/layout/FrameBlockBoxInlineLayout.h"
#include "core/layout/FrameTableBox.h"
#include "core/layout/FrameTableCellBox.h"
#include "core/layout/FrameTableRowBox.h"
#include "core/layout/FrameTableSectionBox.h"
#include "core/layout/FrameText.h"
#include "core/layout/FrameTreeBuilder.h"

namespace StarFish {

FrameTableCellBox::FrameTableCellBox(Node* node, ComputedStyle* style)
    : FrameTableObjectBox(node, style)
    , m_minCellWidth(0)
    , m_maxCellWidth(0)
    , m_actualContentHeight(0)
{
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

    PreferredWidthContext p(ctx, LayoutUnit::max());
    computePreferredWidth(p);
    m_minCellWidth = p.preferredMinWidth() + borderWidth() + paddingWidth();
    m_maxCellWidth = p.preferredWidth() + borderWidth() + paddingWidth();
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

bool FrameTableCellBox::emptyContent()
{
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameText()) {
            if (c->asFrameText()->text()->trim()->equals(String::emptyString)) {
                if ((style()->whiteSpace() ==
                     WhiteSpaceValue::NormalWhiteSpaceValue) ||
                    (style()->whiteSpace() ==
                     WhiteSpaceValue::NoWrapWhiteSpaceValue)) {
                    continue;
                }
            }
        }
        return false;
    }
    return true;
}

// Table draws the border around the TableFrameSections
void FrameTableCellBox::paintBackgroundAndBorders(Canvas* canvas)
{
    // Do not print borders and background if "emptyCells: hide", and
    // there's no content
    if (style()->emptyCells() == EmptyCellsValue::HideEmptyCellsValue &&
        emptyContent()) {
        return;
    }

    // Print borders and background if the cell width is > 0
    if (width() > 0) {
        Unit::Color bgColor;
        if (bgColorFromAttribute(&bgColor)) {
            style()->setBackgroundColor(bgColor);
        }

        FrameBox::paintBackgroundAndBorders(canvas);
    }
}

void FrameTableCellBox::applyVerticalAlign(LayoutContext& ctx)
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
        yPosOffset = rowBox()->baseline() - calBaseline(ctx);
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

LayoutUnit FrameTableCellBox::calBaseline(LayoutContext& ctx)
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
            return flb->absolutePoint(this).y() + ctx.firstLineAscender(flb);
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
    if (!(node() && node()->isHTMLElement())) {
        return 1;
    }

    HTMLElement* e = node()->asHTMLElement();
    // Use 1 as the default value
    uint32_t colspan = 1;
    if (e->isHTMLTableCellElement()) {
        colspan = e->asHTMLTableCellElement()->colSpan();
    } else {
        // colspan is only accepted when HTML element is either <td> or <th>,
        // hence it is not applied when used in other elements.
        // e.g., <div style="display: table-cell" colspan="2">
        // In this case, we ignore the colspan value
    }
    return colspan;
}

void* FrameTableCellBox::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FrameTableCellBox)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableCellBox, m_node));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableCellBox, m_layoutParent));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableCellBox, m_treeItemModel.m_parent));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableCellBox,
                                              m_treeItemModel.m_previous));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameTableCellBox, m_treeItemModel.m_next));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableCellBox,
                                              m_treeItemModel.m_firstChild));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableCellBox,
                                              m_treeItemModel.m_lastChild));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameTableCellBox, m_lineBoxes));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameTableCellBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}
}
