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
    for (size_t row = 0; row < m_gridLineRows.size() - 1; row++) {
        for (size_t col = 0; col < m_gridLineColumns.size() - 1; col++) {
            LayoutUnit offsetX, offsetY;
            FrameBox* gridItem = (*item);
            if (!row) {
                offsetY = m_container->borderTop() + m_container->paddingTop();
            } else {
                offsetY = m_gridLineRows[row].gap() + m_container->borderTop() +
                          m_container->paddingTop();
            }

            if (!col) {
                offsetX =
                    m_container->borderLeft() + m_container->paddingLeft();
            } else {
                offsetX = m_gridLineColumns[col].gap() +
                          m_container->borderLeft() +
                          m_container->paddingLeft();
            }
            gridItem->setX(offsetX);
            gridItem->setY(offsetY);
            item++;
            if (item == m_orderedGridItems.end()) {
                m_container->computeContentHeight(
                    m_layoutContext,
                    m_gridLineRows[m_gridLineRows.size() - 1].gap());
                return;
            }
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
            GridLine preGridLine = m_gridLineColumns[i];

            if (gridLength.isLength() && gridLength.length().isFixed()) {
                Length length = gridLength.length();
                GridLine line =
                    GridLine(preGridLine.gap() + length.numberData());
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
            GridLine preGridLine = m_gridLineRows[i];

            if (gridLength.isLength() && gridLength.length().isFixed()) {
                Length length = gridLength.length();
                GridLine line =
                    GridLine(preGridLine.gap() + length.numberData());
                m_gridLineRows.push_back(line);
            }
        }
    }

    auto item = m_orderedGridItems.begin();
    size_t lineNumber = 0;
    size_t columnIndex = 0;
    size_t rowIndex = 0;
    double maxHeight = 0;

    while (item != m_orderedGridItems.end()) {
        bool needNewLine = false;

        if (m_gridLineRows.size() <= rowIndex + 1) {
            needNewLine = true;
        }

        GridLine preGridLine = m_gridLineColumns[columnIndex];
        GridLine gridLength = m_gridLineColumns[columnIndex + 1];

        {
            FrameBox* gridItem = (*item);
            GridLayoutScope scope(gridItem);
            ComputedStyle* style = gridItem->style();

            style->setWidth(
                Length(Length::Fixed, gridLength.gap() - preGridLine.gap()));
            if (!needNewLine) {
                if (rowIndex + 1 < m_gridLineRows.size()) {
                    style->setHeight(Length(
                        Length::Fixed, m_gridLineRows[rowIndex + 1].gap() -
                                           m_gridLineRows[rowIndex].gap()));
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
                GridLine line =
                    GridLine(m_gridLineRows[rowIndex].gap() + maxHeight);
                m_gridLineRows.push_back(line);
            }

            columnIndex = 0;
            maxHeight = 0;
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
