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

LayoutUnit GridFormattingContext::preferredWidth()
{
    LayoutUnit widthOfSum(0);
    for (size_t col = 0; col < m_gridLineColumns.size(); col++) {
        if (m_gridLineColumns[col].isComputed()) {
            widthOfSum += m_gridLineColumns[col].offset();
        }
    }
    return widthOfSum;
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
    for (auto area : m_orderedGridArea) {
        FrameBox* gridItem = area.m_box;
        LayoutUnit offsetX, offsetY;

        LayoutUnit heightOfSum(0);
        for (size_t i = 0; i < area.m_rowStart; i++) {
            heightOfSum += m_gridLineRows[i].offset();
        }

        LayoutUnit widthOfSum(0);
        for (size_t i = 0; i < area.m_columnStart; i++) {
            widthOfSum += m_gridLineColumns[i].offset();
        }

        offsetY =
            heightOfSum + m_container->borderTop() + m_container->paddingTop();

        offsetX =
            widthOfSum + m_container->borderLeft() + m_container->paddingLeft();
        gridItem->setX(offsetX);
        gridItem->setY(offsetY);
    }

    LayoutUnit heightOfSum(0);
    for (size_t i = 0; i < m_gridLineRows.size(); i++) {
        heightOfSum += m_gridLineRows[i].offset();
    }

    m_container->computeContentHeight(m_layoutContext, heightOfSum);
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
                LayoutUnit offset = (line.fr() * remainingSpace) / frOfSum;
                double value = round(offset.toDouble());
                line.setOffset(value, true);
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
        if (!line.isComputed() && line.isFr()) {
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
            LayoutUnit offset = maxGrid->offset() * line->fr() / maxGrid->fr();
            double value = round(offset.toDouble());
            offset = std::max(value, line->offset().toDouble());
            line->setOffset(offset, true);
        }
    }
}

static void adaptStartAndEndValueForRow(size_t numberOfRows, size_t& start,
                                        size_t& end)
{
    if (start > 0 && end > 0) {
        if (start > end) {
            size_t temp = start;
            start = end;
            end = temp;
        }
    } else if (start > 0 && !end) {
        end = start + 1;
    } else if (end > 0 && !start) {
        start = end;
        end = start + 1;
    }

    // FIXME : this exception is wrong
    // to control lines over the number of fixed rows.
    if (start > numberOfRows - 1 || end > numberOfRows) {
        start = end = 0;
    }

    if (start > GRID_MAX_TRACK - 1 || end > GRID_MAX_TRACK) {
        start = end = 0;
    }
}

static void adaptStartAndEndValueForColumn(size_t numberOfColumns,
                                           size_t& start, size_t& end)
{
    if (start > 0 && end > 0) {
        if (start > end) {
            size_t temp = start;
            start = end;
            end = temp;
        }
    } else if (start > 0 && !end) {
        end = start + 1;
    } else if (end > 0 && !start) {
        start = end - 1;
    }

    // FIXME : this exception is wrong
    // to control lines over the number of fixed columns.
    if (start > numberOfColumns - 1 || end > numberOfColumns) {
        start = end = 0;
    }

    if (start > GRID_MAX_TRACK - 1 || end > GRID_MAX_TRACK) {
        start = end = 0;
    }
}

bool GridFormattingContext::fixGridAreaWithDefine(GridArea* area, size_t row)
{
    if (!area) {
        return false;
    }

    if (m_gridLineColumns.size() >= GRID_MAX_TRACK || row >= GRID_MAX_TRACK) {
        return true;
    }

    size_t rowStart = area->m_rowStart;
    size_t rowEnd = area->m_rowEnd;

    if (rowStart != row + 1) {
        return false;
    }

    size_t columnStart = area->m_columnStart;
    size_t columnEnd = area->m_columnEnd;

    bool available = false;
    if (!columnStart && !columnEnd) {
        size_t columnLength = m_gridLineColumns.size() - 1;
        for (size_t i = 0; i < columnLength; i++) {
            if (m_areaChecker[row][i]) {
                columnStart = i + 1;
                columnEnd = i + 2;
                available = true;
                break;
            }
        }
    } else {
        available = true;
    }

    if (!available) {
        return false;
    }

    for (size_t row = rowStart - 1; row < rowEnd - 1; row++) {
        for (size_t col = columnStart - 1; col < columnEnd - 1; col++) {
            if (!m_areaChecker[row][col]) {
                available = false;
            }
        }
    }

    if (!available) {
        return false;
    }

    for (size_t row = rowStart - 1; row < rowEnd - 1; row++) {
        for (size_t col = columnStart - 1; col < columnEnd - 1; col++) {
            m_areaChecker[row][col] = false;
        }
    }

    area->m_rowStart = rowStart;
    area->m_rowEnd = rowEnd;
    area->m_columnStart = columnStart;
    area->m_columnEnd = columnEnd;

    m_orderedGridArea.push_back(*area);

    return true;
}

bool GridFormattingContext::fixGridAreaWithUndefine(GridArea* area, size_t row)
{
    if (!area) {
        return false;
    }

    ComputedStyle* style = area->m_box->style();
    size_t columnLength = m_gridLineColumns.size() - 1;
    size_t position = 0;
    bool available = true;

    size_t start = style->gridColumnStart();
    size_t end = style->gridColumnEnd();
    adaptStartAndEndValueForColumn(m_gridLineColumns.size(), start, end);

    if (columnLength + 1 >= GRID_MAX_TRACK || row >= GRID_MAX_TRACK) {
        return true;
    }

    if (!start && !end) {
        for (size_t i = 0; i < columnLength; i++) {
            if (m_areaChecker[row][i]) {
                position = i + 1;
                m_areaChecker[row][i] = false;
                break;
            }
        }
    } else {
        for (size_t i = start - 1; i < end - 1; i++) {
            if (!m_areaChecker[row][i]) {
                available = false;
                break;
            }
        }

        if (available) {
            for (size_t i = start - 1; i < end - 1; i++) {
                m_areaChecker[row][i] = false;
            }
            position = style->gridColumnStart();
        }
    }

    if (row + 1 > m_gridLineRows.size() - 1) {
        GridLine line = GridLine(0);
        line.setComputed(false);

        if (m_gridLineRows.size() >= GRID_MAX_TRACK) {
            return true;
        }

        m_gridLineRows.push_back(line);
    }

    if (!position || !available) {
        return false;
    }

    area->m_rowStart = row + 1;
    area->m_rowEnd = area->m_rowStart + 1;

    area->m_columnStart = position;
    if (!start && !end) {
        area->m_columnEnd = area->m_columnStart + 1;
    } else {
        area->m_columnEnd = end;
    }

    m_orderedGridArea.push_back(*area);

    return true;
}

void GridFormattingContext::buildGridAreaAndOrdering()
{
    for (size_t i = 0; i < GRID_MAX_TRACK; i++) {
        for (size_t j = 0; j < GRID_MAX_TRACK; j++) {
            m_areaChecker[i][j] = true;
        }
    }

    size_t idx = 0;
    std::vector<GridArea> defined;
    std::vector<GridArea> undefined;
    for (auto gridItem : m_orderedGridItems) {
        ComputedStyle* style = gridItem->style();

        size_t rowStart = style->gridRowStart();
        size_t rowEnd = style->gridRowEnd();
        size_t columnStart = style->gridColumnStart();
        size_t columnEnd = style->gridColumnEnd();

        adaptStartAndEndValueForRow(m_gridLineRows.size(), rowStart, rowEnd);
        adaptStartAndEndValueForColumn(m_gridLineColumns.size(), columnStart,
                                       columnEnd);
        if (rowStart && rowEnd) {
            GridArea area(gridItem, idx, rowStart, rowEnd, columnStart,
                          columnEnd);
            defined.push_back(area);
        } else {
            GridArea area(gridItem, idx, rowStart, rowEnd, columnStart,
                          columnEnd);
            undefined.push_back(area);
        }
        idx++;
    }

    std::stable_sort(defined.begin(), defined.end(),
                     [](const GridArea& a, const GridArea& b) {
                         return a.m_rowStart < b.m_rowStart;
                     });

    size_t definedIdx = 0;
    size_t undefinedIdx = 0;

    for (size_t row = 0; row < m_gridLineRows.size();) {
        GridArea* definedGridArea = nullptr;
        GridArea* undefinedGridArea = nullptr;
        // Make a context stack.

        if (definedIdx < defined.size()) {
            definedGridArea = &defined[definedIdx];
        }

        if (undefinedIdx < undefined.size()) {
            undefinedGridArea = &undefined[undefinedIdx];
        }

        // First, order defined grid items.
        while (fixGridAreaWithDefine(definedGridArea, row)) {
            definedIdx++;
            if (definedIdx < defined.size()) {
                definedGridArea = &defined[definedIdx];
            } else {
                definedGridArea = nullptr;
            }
        }

        while (fixGridAreaWithUndefine(undefinedGridArea, row)) {
            undefinedIdx++;
            if (undefinedIdx < undefined.size()) {
                undefinedGridArea = &undefined[undefinedIdx];
            } else {
                undefinedGridArea = nullptr;
            }
        }

        row++;
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

    // ordering item and make line.
    buildGridAreaAndOrdering();

    applyFrUnitsWithColumns();

    arrageGridLinesWithGridAreas();

    applyFrUnitsWithRows();

    arrageGridLinesWithGridAreas();
}

void GridFormattingContext::arrageGridLinesWithGridAreas()
{
    for (auto area : m_orderedGridArea) {
        FrameBox* gridItem = area.m_box;
        GridLayoutScope scope(gridItem);
        ComputedStyle* style = gridItem->style();

        LayoutUnit width;
        for (size_t i = area.m_columnStart; i <= area.m_columnEnd - 1; i++) {
            width += m_gridLineColumns[i].offset();
        }

        Length styleWidth = style->width();
        if (styleWidth.isFixed()) {
            width = styleWidth.fixed();
        } else {
            if (width <= 0) {
                PreferredWidthMainContext mainContext;
                PreferredWidthContext p(m_layoutContext, mainContext, gridItem,
                                        gridItem, 0);
                p.computePreferredWidth();
                LayoutUnit contentWidth = p.preferredWidth();
                GridLine& frColumn = m_gridLineColumns[area.m_columnStart];
                frColumn.setOffset(contentWidth, true);
                width = contentWidth;
            }
        }

        style->setWidth(Length(Length::Fixed, width));

        if (!style->height().isFixed()) {
            LayoutUnit height;
            for (size_t i = area.m_rowStart; i <= area.m_rowEnd - 1; i++) {
                height += m_gridLineRows[i].offset();
            }

            if (m_gridLineRows[area.m_rowStart].isComputed()) {
                style->setHeight(Length(Length::Fixed, height));
            }
        }

        gridItem->layout(m_layoutContext,
                         Frame::LayoutWantToResolve::ResolveAll);

        if (!m_gridLineRows[area.m_rowStart].isComputed()) {
            GridLine& line = m_gridLineRows[area.m_rowStart];
            line.setOffset(std::max(line.offset(), gridItem->height()), false);
        }
    }

    for (size_t row = 0; row < m_gridLineRows.size(); row++) {
        if (!m_gridLineRows[row].isComputed() && !m_gridLineRows[row].isFr()) {
            GridLine& line = m_gridLineRows[row];
            line.setComputed(true);
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
