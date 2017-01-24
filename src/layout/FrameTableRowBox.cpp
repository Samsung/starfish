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
#include "FrameTableRowBox.h"

#include "FrameTreeBuilder.h"
#include "FrameTableBox.h"
#include "FrameTableCellBox.h"
#include "FrameTableSectionBox.h"

namespace StarFish {

FrameTableRowBox::FrameTableRowBox(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
    STARFISH_ASSERT((node == nullptr && style != nullptr)
        || (node != nullptr && style == nullptr));
}

FrameTableRowBox* FrameTableRowBox::buildFrameTableRow(Node* current,
    FrameTreeBuilderContext& ctx, bool force)
{
    FrameBlockBox* parent = ctx.currentBlockContainer();
    FrameTableRowBox* tableRow;

    if (current->isTableRow()) {
        tableRow = new FrameTableRowBox(current, nullptr);
        current->setFrame(tableRow);
    } else {
        // current node is not tableRow then make anonymous Row or
        // reuse last anonymous Row
        Frame* before = parent->lastChild();
        if (before && before->isAnonymous() && before->isFrameTableRowBox()) {
            tableRow = before->asFrameTableRowBox();
        } else {
            tableRow = FrameTableRowBox::createAnonymousWithParent(parent, current);
        }
    }

    ctx.setCurrentBlockContainer(tableRow);
    ctx.mergeTextDecorationData(tableRow->style());

    FrameTableCellBox* tableCell;
    if (current->isTableRow()) {
        unsigned i = 0;
        for (Node* c = current->firstChild(); c; c = c->nextSibling()) {
            tableCell = tableRow->addChild(c, ctx, force);
            // TODO: need to calculate absoluteColumnIndex
            // After implementing anonymous boxes, replace the null check with assert()
            if (tableCell) {
                tableCell->setAbsoluteColumnIndex(i);
                i += tableCell->colspan();
            }
        }
    } else if (tableRow->isAnonymous()) {
        // set cell index value if reuse last anonymous table row
        tableCell = tableRow->addChild(current, ctx, force);
        if (tableCell != nullptr) {
            unsigned cellIndex = 0;
            for (Frame* c = tableRow->firstChild(); c; c = c->next()) {
                if (c->isFrameTableCellBox())
                    cellIndex += c->asFrameTableCellBox()->colspan();
            }
            tableCell->setAbsoluteColumnIndex(cellIndex);
            STARFISH_ASSERT(tableCell->parent());
        }
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }

    ctx.setCurrentBlockContainer(parent);
    if (tableRow->parent()) {
        parent->asFrameTableSectionBox()->grid()[tableRow->rowIndex()].cells.push_back(CellStruct(tableCell->asFrameTableCellBox()));
        return nullptr;
    }
    return tableRow;
}

FrameTableRowBox* FrameTableRowBox::createAnonymousWithParent(FrameBlockBox* parent, Node* node)
{
    ComputedStyle* style = new ComputedStyle(parent->style());
    style->setDisplay(DisplayValue::TableRowDisplayValue);
    style->loadResources(node);
    style->arrangeStyleValues(parent->style(), node);

    return new FrameTableRowBox(nullptr, style);
}

FrameTableCellBox* FrameTableRowBox::addChild(Node* child, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTableCellBox* childFrame;
    if (!child->isTableCell()) {
        // TODO
        if (child->isCharacterData() || child->isComment()) {
            return nullptr;
        } else {
            // return nullptr, if buildFrameTableCell resuse before anonymous cell
            childFrame = FrameTableCellBox::buildFrameTableCell(child, ctx, force);
            if (childFrame != nullptr) {
                ctx.currentBlockContainer()->appendChild(childFrame);
            }
            return childFrame;
        }
    }

    childFrame = FrameTableCellBox::buildFrameTableCell(child, ctx, force);
    FrameTreeBuilder::frameBlockBoxChildInserter(ctx.currentBlockContainer(), childFrame, child, ctx);
    STARFISH_ASSERT(childFrame->parent());
    return childFrame;
}

void FrameTableRowBox::calCellWidth(LayoutContext& ctx)
{
    // We traverse the cells first to calculate min/max cell width
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCellBox()) {
            c->asFrameTableCellBox()->calCellWidth(ctx, Frame::LayoutWantToResolve::ResolveWidth);
        } else {
            // Only FrameTableCell should appear
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }
}

void FrameTableRowBox::layoutWidth(LayoutContext& ctx)
{
    LayoutUnit xSoFar = 0;
    LayoutUnit borderSpacing =
        LayoutUnit::fromPixel(tableSectionBox()->tableBox()->style()->borderSpacing().fixed());

    if (firstChild()) {
        xSoFar += borderSpacing;
    }

    int i = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCellBox()) {
            FrameBox* cell = c->asFrameTableCellBox();
            cell->setX(xSoFar);
            LayoutUnit cellWidth = tableSectionBox()->tableBox()-> columnWidths()[i].cellWidth;
            cell->setWidth(cellWidth);
            cell->asFrameTableCellBox()->layoutWidth(ctx);
            xSoFar += cellWidth;
            xSoFar += borderSpacing;
            i++;
        } else {
            // Only FrameTableCell should appear
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    setWidth(xSoFar);
}

void FrameTableRowBox::layoutHeight(LayoutContext& ctx)
{
    LayoutUnit maxHeightSoFar = 0;
    // 1. We make the second iteration of cells to layout cells
    //    and calculate the width of each cell
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCellBox()) {
            FrameTableCellBox* cell = c->asFrameTableCellBox();
            cell->layoutHeight(ctx);
            LayoutUnit cellHeight = cell->height();
            maxHeightSoFar = std::max(maxHeightSoFar, cellHeight);
        } else {
            // Only FrameTableCell should appear
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    // 2. The height of each cell is set to the max height of the cells
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCellBox()) {
            FrameTableCellBox* cell = c->asFrameTableCellBox();
            cell->setHeight(maxHeightSoFar);
        }
    }

    setHeight(maxHeightSoFar);

    // 3. Place the contents of each cell according to the vertical-align of
    //    each cell
    // TODO: 3.1 find baseline
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableCellBox()) {
            c->asFrameTableCellBox()->applyVerticalAlign();
        } else {
            // Only FrameTableCell should appear
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }
}

void FrameTableRowBox::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    // This method should not be called, as table uses its own layout algorithm
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

void FrameTableRowBox::paintBackgroundAndBorders(Canvas* canvas)
{
    Frame* child = firstChild();
    while (child) {
        if (child->isFrameTableCellBox()) {
            FrameTableCellBox* cell = child->asFrameTableCellBox();

            LayoutRect rect(
                cell->x(),
                cell->y(),
                cell->frameRect().width(),
                cell->frameRect().height()
            );
            paintBackground(canvas, style(), rect, rect, false);
        }
        child = child->next();
    }

    canvas->save();

    // below codes were copied from framebox.h
    // It is redundant code and will be removed when refactoring.

    // draw border-image
    if (style()->hasBorderImageData()) {
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

        if (bImgSlice > imgWidth || bImgSlice > imgHeight) {
            bImgSlice = std::min(imgWidth, imgHeight);
        }
        double value = std::min((float)width() / (bImgWidth*2), (float)height() / (bImgWidth*2));
        if (value < 1) {
            bImgWidth *= value;
        }
        double scale = bImgWidth / bImgSlice;
        bool isFill = false;

        if ((lSlice + rSlice > imgWidth) || (tSlice + bSlice > imgHeight)) {
            float drawRect = std::min((float)width(), (float)height()) / 2.0;

            if (drawRect > bImgWidth) {
                drawRect = bImgWidth;
            }
            // left-top
            canvas->drawBorderImage(imgData, Rect(0, 0, drawRect, drawRect), lSlice, tSlice, 0, 0, scale, isFill);
            // right-top
            canvas->drawBorderImage(imgData, Rect((float)width() - drawRect, 0, drawRect, drawRect), 0, tSlice, rSlice, 0, scale, isFill);
            // right-bottom
            canvas->drawBorderImage(imgData, Rect((float)width() - drawRect, (float)height() - drawRect, drawRect, drawRect), 0, 0, rSlice, bSlice, scale, isFill);
            // left-bottom
            canvas->drawBorderImage(imgData, Rect(0, (float)height() - drawRect, drawRect, drawRect), lSlice, 0, 0, bSlice, scale, isFill);
        } else {
            isFill = style()->surround()->border.image().sliceFill();
            canvas->drawBorderImage(imgData, Rect(0, 0, width(), height()), lSlice, tSlice, rSlice, bSlice, scale, isFill);
        }
    } else if (style()->hasBorderStyle()) {
        // draw border
        // TODO border-join

        if ((style()->borderTopColor() == style()->borderRightColor())
            && (style()->borderRightColor() == style()->borderBottomColor())
            && (style()->borderBottomColor() == style()->borderLeftColor())) {
            // if 4-colors are same.

            // top
            canvas->setColor(style()->borderTopColor());
            canvas->drawRect(LayoutRect(0, 0, width(), borderTop()));
            // right
            canvas->setColor(style()->borderRightColor());
            canvas->drawRect(LayoutRect(width()-borderRight(), 0, borderRight(), height()));
            // bottom
            canvas->setColor(style()->borderBottomColor());
            canvas->drawRect(LayoutRect(0, height()-borderBottom(), width(), borderBottom()));
            // left
            canvas->setColor(style()->borderLeftColor());
            canvas->drawRect(LayoutRect(0, 0, borderLeft(), height()));
        } else {
            // top
            canvas->setColor(style()->borderTopColor());
            canvas->drawRect(
                LayoutLocation(0, 0),
                LayoutLocation(width(), 0),
                LayoutLocation(width() - borderRight(), borderTop()),
                LayoutLocation(borderLeft(), borderTop())
            );

            // right
            canvas->setColor(style()->borderRightColor());
            canvas->drawRect(
                LayoutLocation(width() - borderRight(), borderTop()),
                LayoutLocation(width(), 0),
                LayoutLocation(width(), height()),
                LayoutLocation(width() - borderRight(), height() - borderBottom())
            );

            // bottom
            canvas->setColor(style()->borderBottomColor());
            canvas->drawRect(
                LayoutLocation(borderLeft(), height() - borderBottom()),
                LayoutLocation(width() - borderRight(), height() - borderBottom()),
                LayoutLocation(width(), height()),
                LayoutLocation(0, height())
            );

            // left
            canvas->setColor(style()->borderLeftColor());
            canvas->drawRect(
                LayoutLocation(0, 0),
                LayoutLocation(borderLeft(), borderTop()),
                LayoutLocation(borderLeft(), height() - borderBottom()),
                LayoutLocation(0, height())
            );
        }
    }

    canvas->restore();
}

}
