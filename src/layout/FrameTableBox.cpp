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
#include "FrameTableBox.h"

#include "FrameTreeBuilder.h"
#include "FrameTableCaptionBox.h"
#include "FrameTableSectionBox.h"

namespace StarFish {

class TableFormattingContextBlock {
public:
    TableFormattingContextBlock(Frame* frm, LayoutContext& ctx)
        : m_ctx(ctx)
        , m_needs(false)
    {
        if (frm->isEstablishesBlockFormattingContext()) {
            m_needs = true;
            m_ctx.establishBlockFormattingContext(frm->isNormalFlow());
        }
    }

    ~TableFormattingContextBlock()
    {
        if (m_needs) {
            m_ctx.removeBlockFormattingContext();
        }
    }

    LayoutContext& m_ctx;
    bool m_needs;
};

FrameTableBox::FrameTableBox(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
    , m_tableRect(0, 0, 0, 0)
{
    STARFISH_ASSERT((node == nullptr && style != nullptr)
        || (node != nullptr && style == nullptr));
}

FrameTableBox* FrameTableBox::buildFrameTable(Node* current, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTableBox* tableWrapper;
    FrameBlockBox* parent = ctx.currentBlockContainer();
    if (current->isTable()) {
        // if current node is table then make wrapper and current node owns this wrapper
        tableWrapper = new FrameTableBox(current, nullptr);
        current->setFrame(tableWrapper);
    } else {
        // if current node is not table wrapper node then make anonymous wrapper or
        // reuse before anonymous wrapper
        FrameBlockBox* parent = ctx.currentBlockContainer();
        Frame* before = parent->lastChild();

        // NEED TO DISCUSSION : StarFish generate anonymous block box which has only wihtespace,
        // below code treat above situation
        while (before->isAnonymous() && before->firstChild()->isFrameText()
            && before->firstChild()->asFrameText()->text()->containsOnlyWhitespace()) {
            before = before->previous();
        }

        if (before && before->isAnonymous() && before->isFrameTableBox()) {
            tableWrapper = before->asFrameTableBox();
        } else {
            tableWrapper = FrameTableBox::createAnonymousWithParent(parent, current);
        }

    }

    // Table establishes a new block context
    ctx.setCurrentBlockContainer(tableWrapper);
    ctx.mergeTextDecorationData(tableWrapper->style());

    if (current->isTable()) {
        for (Node* c = current->firstChild(); c; c = c->nextSibling()) {
            tableWrapper->addChild(c, ctx, force);
        }
    } else if (tableWrapper->isAnonymous()) {
        tableWrapper->addChild(current, ctx, force);
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }

    ctx.setCurrentBlockContainer(parent);
    return tableWrapper->parent()? nullptr : tableWrapper;
}

FrameTableBox* FrameTableBox::createAnonymousWithParent(FrameBlockBox* parent, Node* node)
{
    ComputedStyle* style = new ComputedStyle(parent->style());
    style->setDisplay(DisplayValue::TableRowDisplayValue);
    style->loadResources(node);
    style->arrangeStyleValues(parent->style(), node);

    return new FrameTableBox(nullptr, style);
}

void FrameTableBox::addChild(Node* child, FrameTreeBuilderContext& ctx, bool force)
{
    bool wrapInAnnoymousSection = false;
    Frame* childFrame;

    if (child->isTableCaption()) {
        childFrame = FrameTableCaptionBox::buildFrameTableCaptionBox(child, ctx, force);
        m_captions.push_back(childFrame->asFrameTableCaptionBox());
    } else if (child->isTableCol()) {
        // TODO
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    } else if (child->isTableSection()) {
        // Only [TableHeaderGroup | TableFooterGroup | TableRowGroup] should appear
        switch (child->style()->display()) {
        case DisplayValue::TableHeaderGroupDisplayValue:
        case DisplayValue::TableFooterGroupDisplayValue:
        case DisplayValue::TableRowGroupDisplayValue:
            childFrame = FrameTableSectionBox::buildFrameTableSectionBox(child, ctx, force);
            break;
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else {
        wrapInAnnoymousSection = true;
    }

    if (!wrapInAnnoymousSection) {
        FrameTreeBuilder::frameBlockBoxChildInserter(ctx.currentBlockContainer(), childFrame, child, ctx);
        STARFISH_ASSERT(childFrame->parent());
        return;
    }

    if (child->isCharacterData() || child->isComment()) {
        // TODO
        return;
    } else if (wrapInAnnoymousSection) {
        // return nullptr, if buildFrameTableSection reuse before anonymouse section
        childFrame = FrameTableSectionBox::buildFrameTableSectionBox(child, ctx, force);
        if (childFrame != nullptr) {
            ctx.currentBlockContainer()->appendChild(childFrame);
            STARFISH_ASSERT(childFrame->parent());
        }
        return;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void FrameTableBox::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    // This method is called by FrameBlockBox::layout() to do table layout.
    // Table starts its own layout algorithm that has minimum interaction with
    // the existing layout algorithm.
    //
    // In brief,
    // after establishes a table context, we calculate the width of the table,
    // and place cells in rows and columns. To do so, we calculate x positions
    // of cells first, and then calculate the y positions of cells.
    TableFormattingContextBlock context(this, ctx);

    if (resolveWhat & Frame::LayoutWantToResolve::ResolveWidth) {
        calCellWidth(ctx);
        layoutWidth(ctx);
    }
    if (resolveWhat & Frame::LayoutWantToResolve::ResolveHeight) {
        layoutHeight(ctx);
    }
}

void FrameTableBox::calCellWidth(LayoutContext& ctx)
{
    // 1. We traverse the table to calculate min/max cell widths of the table
    //    before we perform table layout.
    m_columnWidths.clear();
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox()) {
            c->asFrameTableSectionBox()->calCellWidth(ctx);
            collectColumnWidths(m_columnWidths, c->asFrameTableSectionBox()->columnWidths());
        }
    }

    // 2. Determine the column size of the table
    //    If the parent's width is smaller than the max table width,
    //    the with of the table is adjusted to somewhere between min/max table
    //    width.
    //    If the parent's width >= the max table width,
    //    max table width is used

    // 2.1 calculate the table width
    LayoutUnit borderSpacing = LayoutUnit::fromPixel(style()->borderSpacing().fixed());
    LayoutUnit tableWidth = 0;
    tableWidth += marginLeft() + marginRight();
    tableWidth += borderLeft() + borderRight();
    tableWidth += paddingLeft() + paddingRight();
    tableWidth += borderSpacing;
    for (auto& colSize : m_columnWidths) {
        tableWidth += colSize.maxCellWidth + borderSpacing;
    }

    LayoutUnit parentContentWidth = ctx.parentContentWidth(this);
    if (tableWidth > parentContentWidth) {
        LayoutUnit availableWidth = parentContentWidth;
        availableWidth -= marginLeft() + marginRight();
        availableWidth -= borderLeft() + borderRight();
        availableWidth -= paddingLeft() + paddingRight();
        availableWidth -= borderSpacing + (borderSpacing * m_columnWidths.size());

        // 2.2 Calculate the ratio of which each column is to be reduced.
        //     The cell width is:
        //     minCellWidth <= cellWidth <= maxCellWidth
        LayoutUnit totalCellWidths = 0;
        for (auto& col : m_columnWidths) {
            totalCellWidths += col.maxCellWidth;
        }

        for (auto& col : m_columnWidths) {
            // FIXME: LayoutUnit has a rounding error bug when division is
            // performed. To workaround, we convert LayoutUnit to double,
            // do calculation, and convert back to LayoutUnit.
            // LayoutUnit newCellWidth = availableWidth * (col.maxCellWidth / totalCellWidths);
            LayoutUnit newCellWidth(availableWidth.toDouble() *
                (col.maxCellWidth.toDouble() / totalCellWidths.toDouble()));

            if (newCellWidth.round() < col.minCellWidth) {
                col.cellWidth = col.minCellWidth;
                availableWidth -= col.minCellWidth;
                totalCellWidths -= col.maxCellWidth;
            }
        }

        for (auto& col : m_columnWidths) {
            if (col.cellWidth != col.minCellWidth) {
                // To workaround the rounding error in LayoutUnit
                // LayoutUnit newCellWidth = availableWidth * (col.maxCellWidth / totalCellWidths);
                LayoutUnit newCellWidth(availableWidth.toDouble() *
                    (col.maxCellWidth.toDouble() / totalCellWidths.toDouble()));
                col.cellWidth = std::max(col.minCellWidth.toInt(), newCellWidth.round());
            }
        }
    }
}

void FrameTableBox::layoutWidth(LayoutContext& ctx)
{
    // The width of the caption is limited by the max width of the
    // FrameTableSection. Hence, captions can only be placed after calculating
    // the width of the table, which has already been done by calContentWidth()
    LayoutUnit maxWidth = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox()) {
            c->asFrameTableSectionBox()->layoutWidth(ctx);
            maxWidth = std::max(maxWidth, c->asFrameBox()->width());
        } else if (c->isFrameTableCaptionBox()) {
            c->asFrameTableCaptionBox()->layout(ctx, Frame::LayoutWantToResolve::ResolveWidth);
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    // The width of all table sections should be the same,
    // so getting the max width should be the same as the width of
    // any table sections.
    setWidth(maxWidth);
    computeBorderMarginPadding(width());
}

void FrameTableBox::layoutHeight(LayoutContext& ctx)
{
    // Table is placed in the following order:
    // 1. Captions that has property "caption-side: top"
    //    If there are multiple captions, place them in document order
    // 2. Table sections in document order
    // 3. Captions that has property "caption-side: bottom"
    //    If there are multiple captions, place them in document order

    LayoutUnit ySoFar = marginTop();

    // 1. place captions with caption-side: top
    for (auto& caption : m_captions) {
        if (caption->style()->captionSide() == CaptionSideValue::TopCaptionSideValue) {
            caption->setWidth(paddingLeft() + width() + paddingRight());
            caption->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
            caption->setY(ySoFar);
            ySoFar += caption->height();
        }
    }

    // 2. place table sections
    m_tableRect.setX(0);
    m_tableRect.setY(ySoFar);
    ySoFar += borderTop();
    ySoFar += paddingTop();
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSectionBox()) {
            c->asFrameBox()->setX(paddingLeft());
            c->asFrameTableSectionBox()->layoutHeight(ctx);
            c->asFrameBox()->setY(ySoFar);
            ySoFar += c->asFrameBox()->height();
        }
    }
    ySoFar += paddingBottom();
    ySoFar += borderBottom();
    m_tableRect.setWidth(paddingLeft() + width() + paddingRight());
    m_tableRect.setHeight(ySoFar - m_tableRect.y());

    // 3. place captions with caption-side: bottom
    for (auto& caption : m_captions) {
        if (caption->style()->captionSide() == CaptionSideValue::BottomCaptionSideValue) {
            caption->setWidth(paddingLeft() + width() + paddingRight());
            caption->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
            caption->setY(ySoFar);
            ySoFar += caption->height();
        }
    }
    ySoFar += marginBottom();

    setHeight(ySoFar);
}

void FrameTableBox::collectColumnWidths(GCVector<ColSizeStruct>& columnWidthsSoFar, GCVector<ColSizeStruct>& columnWidths)
{
    // FIXME: absolute at this stage
    // Need to consider absolute and logical columns
    if (columnWidthsSoFar.empty()) {
        columnWidthsSoFar = columnWidths;
    } else {
        // FIXME: Update to support colspans
        for (unsigned i = 0; i < columnWidths.size(); i++) {
            ColSizeStruct& col = columnWidths[i];
            if (columnWidthsSoFar.size() == i) {
                columnWidthsSoFar.push_back(ColSizeStruct());
            }
            ColSizeStruct& colSoFar = columnWidthsSoFar[i];
            colSoFar.maxCellWidth = std::max(colSoFar.maxCellWidth, col.maxCellWidth);
            colSoFar.minCellWidth = std::max(colSoFar.minCellWidth, col.minCellWidth);
            colSoFar.cellWidth = colSoFar.maxCellWidth;
        }
    }
}

// Table draws the border around the TableFrameSections
void FrameTableBox::paintBackgroundAndBorders(Canvas* canvas)
{
    // Fill in the table with background color
    LayoutRect bgRect(m_tableRect.x() + borderLeft(), m_tableRect.y() + borderTop(),
        m_tableRect.width() - borderWidth(), m_tableRect.height() - borderHeight());
    paintBackground(canvas, style(), bgRect, m_tableRect, false);

    canvas->save();

    if (style()->hasBorderImageData()) {
        // Draw image borders at the four corners as shown below.
        //   ______________
        //  |_|          |_|
        //  |              |
        //  |              |
        //  |_            _|
        //  |_|__________|_|
        //

        double bWidth = style()->surround()->border.top().width().specifiedValue(height());
        double bImgWidth = style()->surround()->border.image().widths().top().specifiedValue(bWidth);
        double bImgSlice = style()->surround()->border.image().slices().top().specifiedValue(height());

        size_t imgWidth = style()->surround()->border.image().imageData()->width();
        size_t imgHeight = style()->surround()->border.image().imageData()->height();

        size_t lSlice = style()->surround()->border.image().slices().left().specifiedValue(width());
        size_t tSlice = style()->surround()->border.image().slices().top().specifiedValue(height());
        size_t rSlice = style()->surround()->border.image().slices().right().specifiedValue(width());
        size_t bSlice = style()->surround()->border.image().slices().bottom().specifiedValue(height());

        ImageData* imgData = style()->surround()->border.image().imageData();

        if (bImgSlice > imgWidth || bImgSlice > imgHeight)
            bImgSlice = std::min(imgWidth, imgHeight);

        double value = std::min((float)width() / (bImgWidth*2), (float)height() / (bImgWidth*2));
        if (value < 1)
            bImgWidth *= value;

        double scale = bImgWidth / bImgSlice;
        bool isFill = false;

        if ((lSlice + rSlice > imgWidth) || (tSlice + bSlice > imgHeight)) {
            float drawRect = std::min((float)width(), (float)height()) / 2.0;

            if (drawRect > bImgWidth)
                drawRect = bImgWidth;

            // left-top
            canvas->drawBorderImage(imgData,
                Rect(m_tableRect.x(), m_tableRect.y(), drawRect, drawRect), lSlice, tSlice, 0, 0, scale, isFill);
            // right-top
            canvas->drawBorderImage(imgData,
                Rect((float)m_tableRect.width() - drawRect, m_tableRect.y(), drawRect, drawRect), 0, tSlice, rSlice, 0, scale, isFill);
            // right-bottom
            canvas->drawBorderImage(imgData,
                Rect((float)m_tableRect.width() - drawRect, (float)(m_tableRect.y() + m_tableRect.height()) - drawRect, drawRect, drawRect), 0, 0, rSlice, bSlice, scale, isFill);
            // left-bottom
            canvas->drawBorderImage(imgData,
                Rect(m_tableRect.x(), (float)(m_tableRect.y() + m_tableRect.height()) - drawRect, drawRect, drawRect), lSlice, 0, 0, bSlice, scale, isFill);
        } else {
            isFill = style()->surround()->border.image().sliceFill();
            canvas->drawBorderImage(imgData,
                Rect(m_tableRect.x(), m_tableRect.y(),
                    m_tableRect.width(), m_tableRect.height()), lSlice, tSlice, rSlice, bSlice, scale, isFill);
        }
    } else if (style()->hasBorderStyle()) {
        // Draw trapezium-like borders around FrameTableSections
        // The area for FrameTableSections is obtained from m_tableRect,
        // which has been already calculated in layoutHeight();
        //    _______________
        //   |\_____________/|
        //   ||             ||
        //   ||             ||
        //   ||             ||
        //   ||_____________||
        //   |/_____________\|
        //

        // top
        canvas->setColor(style()->borderTopColor());
        canvas->drawRect(
            LayoutLocation(m_tableRect.x(), m_tableRect.y()),
            LayoutLocation(m_tableRect.x() + m_tableRect.width(), m_tableRect.y()),
            LayoutLocation(m_tableRect.x() + m_tableRect.width() - borderRight(), m_tableRect.y() + borderTop()),
            LayoutLocation(m_tableRect.x() + borderLeft(), m_tableRect.y() + borderTop())
        );

        // right
        canvas->setColor(style()->borderRightColor());
        canvas->drawRect(
            LayoutLocation(m_tableRect.x() + m_tableRect.width() - borderRight(), m_tableRect.y() + borderTop()),
            LayoutLocation(m_tableRect.x() + m_tableRect.width(), m_tableRect.y()),
            LayoutLocation(m_tableRect.x() + m_tableRect.width(), m_tableRect.y() + m_tableRect.height()),
            LayoutLocation(m_tableRect.x() + m_tableRect.width() - borderRight(), m_tableRect.y() + m_tableRect.height() - borderBottom())
        );

        // bottom
        canvas->setColor(style()->borderBottomColor());
        canvas->drawRect(
            LayoutLocation(m_tableRect.x() + borderLeft(), m_tableRect.y() + m_tableRect.height() - borderBottom()),
            LayoutLocation(m_tableRect.x() + m_tableRect.width() - borderRight(), m_tableRect.y() + m_tableRect.height() - borderBottom()),
            LayoutLocation(m_tableRect.x() + m_tableRect.width(), m_tableRect.y() + m_tableRect.height()),
            LayoutLocation(m_tableRect.x(), m_tableRect.y() + m_tableRect.height())
        );

        // left
        canvas->setColor(style()->borderLeftColor());
        canvas->drawRect(
            LayoutLocation(m_tableRect.x(), m_tableRect.y()),
            LayoutLocation(m_tableRect.x() + borderLeft(), m_tableRect.y() + borderTop()),
            LayoutLocation(m_tableRect.x() + borderLeft(), m_tableRect.y() + m_tableRect.height() - borderBottom()),
            LayoutLocation(m_tableRect.x(), m_tableRect.y() + m_tableRect.height())
        );
    }

    canvas->restore();
}

}
