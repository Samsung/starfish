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
    Frame* child = m_container->firstChild();

    while (child) {
        if (child->isGridItem()) {
            m_orderedGridItems.push_back(child->asFrameBox());
        }
        child = child->next();
    }

    std::stable_sort(m_orderedGridItems.begin(), m_orderedGridItems.end(),
                     [](FrameBox* a, FrameBox* b) {
                         return a->style()->order() < b->style()->order();
                     });

    buildGridLineTemplate();
    layoutGridItems();
}

void GridFormattingContext::layoutGridItems()
{
    auto item = m_orderedGridItems.begin();
    LayoutUnit heightOfSum(0);

    for (size_t row = 0; row < m_gridLineRows.size() - 1; row++) {
        LayoutUnit widthOfSum(0);
        heightOfSum += m_gridLineRows[row].offset();
        for (size_t col = 0; col < m_gridLineColumns.size() - 1; col++) {
            LayoutUnit offsetX, offsetY;
            FrameBox* gridItem = (*item);

            offsetY = heightOfSum + m_container->borderTop() +
                      m_container->paddingTop();

            widthOfSum += m_gridLineColumns[col].offset();
            offsetX = widthOfSum + m_container->borderLeft() +
                      m_container->paddingLeft();

            gridItem->setX(offsetX);
            gridItem->setY(offsetY);
            item++;
            if (item == m_orderedGridItems.end()) {
                row++;
                heightOfSum += m_gridLineRows[row].offset();
                m_container->computeContentHeight(m_layoutContext, heightOfSum);
                return;
            }
        }
    }
}

void GridFormattingContext::applyFrUnitsWithColumns()
{
    LayoutUnit computedSum(0);
    LayoutUnit frOfSum(0);
    for (size_t i = 0; i < m_gridLineColumns.size(); i++) {
        GridLine line = m_gridLineColumns[i];
        if (line.isComputed()) {
            computedSum += line.offset();
        } else {
            frOfSum += line.fr();
        }
    }

    // https://www.w3.org/TR/css-grid-1/#leftover-space
    // If this value is less than 1, set it to 1 instead.
    if (frOfSum < 1.0f) {
        frOfSum = 1.0f;
    }

    LayoutUnit remainingSpace = m_availableWidth - computedSum;

    for (size_t i = 0; i < m_gridLineColumns.size(); i++) {
        GridLine& line = m_gridLineColumns[i];
        if (!line.isComputed()) {
            if (remainingSpace > 0) {
                LayoutUnit offset =
                    round((line.fr() * remainingSpace) / frOfSum);
                line.setOffset(offset, true);
            } else {
                line.setOffset(0, true);
            }
        }
    }
}

void GridFormattingContext::applyFrUnitsWithRows()
{
    LayoutUnit maxHeight(0);
    GridLine* maxGrid = nullptr;
    for (size_t i = 1; i < m_gridLineRows.size(); i++) {
        GridLine line = m_gridLineRows[i];
        if (!line.isComputed()) {
            if (maxHeight < line.offset()) {
                maxHeight = line.offset();
                maxGrid = &m_gridLineRows[i];
            }
        }
    }

    if (!maxGrid) {
        return;
    } else {
        maxGrid->setComputed(true);
    }

    for (size_t i = 1; i < m_gridLineRows.size(); i++) {
        GridLine* line = &m_gridLineRows[i];
        if (!line->isComputed()) {
            LayoutUnit offset =
                round(maxGrid->offset() * line->fr() / maxGrid->fr());
            offset = std::max(offset, line->offset());
            line->setOffset(offset, true);
        }
    }
}

void GridFormattingContext::buildGridLineTemplate()
{
    const GCVector<GridLength>* columns =
        m_container->style()->gridTemplateColumns();

    m_gridLineColumns.push_back(GridLine(0));
    m_gridLineRows.push_back(GridLine(0));

    if (columns) {
        for (size_t i = 0; i < columns->size(); i++) {
            GridLength gridLength = (*columns)[i];

            if (gridLength.isLength() && gridLength.length().isFixed()) {
                Length length = gridLength.length();
                GridLine line = GridLine(length.numberData());
                m_gridLineColumns.push_back(line);
            } else if (gridLength.isFr()) {
                double value = gridLength.fr();

                GridLine line = GridLine(value, false);
                m_gridLineColumns.push_back(line);
            }
        }
    }

    if (!columns) {
        m_gridLineColumns.push_back(GridLine(m_availableWidth));
    }

    const GCVector<GridLength>* rows = m_container->style()->gridTemplateRows();

    if (rows) {
        for (size_t i = 0; i < rows->size(); i++) {
            GridLength gridLength = (*rows)[i];

            if (gridLength.isLength() && gridLength.length().isFixed()) {
                Length length = gridLength.length();
                GridLine line = GridLine(length.numberData());
                m_gridLineRows.push_back(line);
            } else if (gridLength.isFr()) {
                double value = gridLength.fr();

                GridLine line = GridLine(value, false);
                m_gridLineRows.push_back(line);
            }
        }
    }

    // Apply Flex(Fr) units for Columns such as <1fr>.
    applyFrUnitsWithColumns();

    arrangeGridLines();

    applyFrUnitsWithRows();

    arrangeGridLines();
}

void GridFormattingContext::arrangeGridLines()
{
    // This part is to create hypothetical lines for columns and rows.
    size_t columnIndex = 0;
    size_t rowIndex = 0;
    double maxHeight = 0;

    auto item = m_orderedGridItems.begin();

    while (item != m_orderedGridItems.end()) {
        bool needNewLine = false;

        if (m_gridLineRows.size() <= rowIndex + 1) {
            needNewLine = true;
        }

        GridLine gridColumnLine = m_gridLineColumns[columnIndex + 1];

        {
            FrameBox* gridItem = (*item);
            GridLayoutScope scope(gridItem);

            ComputedStyle* style = gridItem->style();

            // This case is very critical, because there is no remaining spaces
            // for <fr> unit, and then in this case put a prefered width into a
            // line.
            if (gridColumnLine.offset() != 0) {
                Length width = style->width();
                if (!width.isFixed()) {
                    style->setWidth(
                        Length(Length::Fixed, gridColumnLine.offset()));
                }
            } else {
                PreferredWidthMainContext mainContext;
                PreferredWidthContext p(m_layoutContext, mainContext, gridItem,
                                        gridItem, 0);
                p.computePreferredWidth();
                LayoutUnit contentWidth = p.preferredWidth();
                style->setWidth(Length(Length::Fixed, contentWidth));
                GridLine& frColumn = m_gridLineColumns[columnIndex + 1];
                frColumn.setOffset(contentWidth, true);
            }

            if (!needNewLine) {
                if (rowIndex + 1 < m_gridLineRows.size()) {
                    Length height = style->height();
                    if (!height.isFixed() &&
                        m_gridLineRows[rowIndex + 1].isComputed()) {
                        style->setHeight(
                            Length(Length::Fixed,
                                   m_gridLineRows[rowIndex + 1].offset()));
                    }
                }
            }

            gridItem->layout(m_layoutContext,
                             Frame::LayoutWantToResolve::ResolveAll);

            maxHeight = std::max(maxHeight, gridItem->height().toDouble());
        }

        item++;

        if ((columnIndex + 1 >= m_gridLineColumns.size() - 1) ||
            (item == m_orderedGridItems.end())) {
            if (needNewLine) {
                GridLine line = GridLine(maxHeight);
                m_gridLineRows.push_back(line);
            } else {
                if (m_gridLineRows[rowIndex + 1].isFr()) {
                    GridLine& frRow = m_gridLineRows[rowIndex + 1];
                    frRow.setOffset(maxHeight, false);
                }
            }

            columnIndex = maxHeight = 0;
            rowIndex++;
        } else {
            columnIndex++;
        }
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
