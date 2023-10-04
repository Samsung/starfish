/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#include "StarfishConfig.h"

#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Node.h"
#include "core/layout/FrameGridBox.h"
#include "core/layout/FrameDocument.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSParser.h"
#include "core/layout/FrameFlexibleBox.h"

namespace Starfish {

struct GridLayoutScope {
    GridLayoutScope(FrameBox* box)
        : m_box(box)
    {
        ComputedStyle* style = box->style();
        m_width = style->width();
        m_height = style->height();
        m_minWidth = style->minWidth();
        m_maxWidth = style->maxWidth();
        m_minHeight = style->minHeight();
        m_maxHeight = style->maxHeight();

        m_margin = style->margin();
        m_border = style->border();
        m_padding = style->padding();
    }

    ~GridLayoutScope()
    {
        ComputedStyle* style = m_box->style();
        style->setWidth(m_width);
        style->setHeight(m_height);
        style->setMinWidth(m_minWidth);
        style->setMaxWidth(m_maxWidth);
        style->setMinHeight(m_minHeight);
        style->setMaxHeight(m_maxHeight);

        *style->rareComputedStyleData()->ensureMargin() = m_margin;
        *style->rareComputedStyleData()->ensureBorder() = m_border;
        *style->rareComputedStyleData()->ensurePadding() = m_padding;
    }

    FrameBox* m_box;
    Length m_width;
    Length m_height;
    Length m_minWidth;
    Length m_maxWidth;
    Length m_minHeight;
    Length m_maxHeight;

    LengthData m_margin;
    BorderData m_border;
    LengthData m_padding;
};

GridFormattingContext::GridFormattingContext(LayoutContext& ctx,
                                             FrameGridBox* container,
                                             LayoutUnit availableWidth)
    : m_layoutContext(ctx)
    , m_container(container)
    , m_availableWidth(availableWidth)
    , m_rowGap(0)
    , m_columnGap(0)
{
    if (m_container->style()->gridRowGap().isFixed()) {
        m_rowGap = m_container->style()->gridRowGap().fixed();
    }

    if (m_container->style()->gridColumnGap().isFixed()) {
        m_columnGap = m_container->style()->gridColumnGap().fixed();
    }
}

LayoutUnit GridFormattingContext::preferredWidth()
{
    LayoutUnit sumOfWidths = 0;
    for (size_t col = 1; col < m_gridTemplateColumns.size(); col++) {
        sumOfWidths += m_gridTemplateColumns[col].size();
    }

    sumOfWidths += m_columnGap * (m_gridTemplateColumns.size() - 2);

    return sumOfWidths;
}

void GridFormattingContext::computeColumnsAndRows()
{
    buildGridTrackTemplate();
    layoutGridItems();
    layoutNonGridItems();
}

void GridFormattingContext::layoutGridItems()
{
    for (auto& area : m_orderedGridArea) {
        FrameBox* gridItem = area.box();

        LayoutUnit xPosSoFar =
            m_container->borderLeft() + m_container->paddingLeft();

        for (size_t i = 1; i < area.columnStart(); i++) {
            xPosSoFar += m_gridTemplateColumns[i].size();
        }

        xPosSoFar += m_columnGap * (area.columnStart() - 1);

        if (area.isMarginLeftAuto()) {
            LayoutUnit margin = 0;
            for (size_t i = area.columnStart(); i < area.columnEnd(); i++) {
                margin += m_gridTemplateColumns[i].size();
            }
            if (area.isMarginRightAuto()) {
                margin = (margin - gridItem->width()) / 2;
            } else {
                margin -= gridItem->width() + gridItem->marginRight();
            }
            xPosSoFar += margin;
        } else if (gridItem->marginLeft()) {
            if (gridItem->width() > gridItem->mbpWidth()) {
                xPosSoFar += gridItem->marginLeft();
            }
        }

        LayoutUnit yPosSoFar =
            m_container->borderTop() + m_container->paddingTop();

        for (size_t i = 1; i < area.rowStart(); i++) {
            yPosSoFar += m_gridTemplateRows[i].size();
        }

        yPosSoFar += m_rowGap * (area.rowStart() - 1);
        yPosSoFar += gridItem->marginTop();

        gridItem->setX(xPosSoFar);
        gridItem->setY(yPosSoFar);
    }

    LayoutUnit sumOfHeights = 0;
    for (size_t i = 1; i < m_gridTemplateRows.size(); i++) {
        sumOfHeights += m_gridTemplateRows[i].size();
    }

    sumOfHeights += m_rowGap * (m_gridTemplateRows.size() - 2);

    m_container->computeContentHeight(m_layoutContext, sumOfHeights);
    applyAlignItems();
}

void GridFormattingContext::insertNamedGridArea(const std::pair<std::string, GridArea>& pair)
{
    auto iter = m_namedAreaMap.find(pair.first);
    if (iter != m_namedAreaMap.end()) {
        iter.value().push_back(pair.second);
    } else {
        GCVector<GridArea> v;
        v.push_back(pair.second);
        m_namedAreaMap.insert(std::make_pair(pair.first, std::move(v)));
    }
}

GridArea* GridFormattingContext::getNamedGridArea(String* name)
{
    auto it = m_namedAreaMap.find(name->toUTF8NonGCString());
    if (it != m_namedAreaMap.end()) {
        return it.value().data();
    }

    return nullptr;
}

// https://drafts.csswg.org/css-grid/#grid-item-placement-algorithm
void GridFormattingContext::placeGridItemsIntoCells()
{
    GCVector<FrameBox*> orderedGridItems;
    for (Frame* c = m_container->firstChild(); c; c = c->next()) {
        if (c->isGridItem()) {
            orderedGridItems.push_back(c->asFrameBox());
        }
    }

    std::stable_sort(orderedGridItems.begin(), orderedGridItems.end(),
                     [](FrameBox* a, FrameBox* b) {
                         STARFISH_ASSERT(a != nullptr);
                         STARFISH_ASSERT(b != nullptr);
                         return a->style()->order() < b->style()->order();
                     });

    // 1. Position anything that’s not auto-positioned.
    GCVector<GridArea*> gridAreasDefinite;
    GCVector<GridArea*> gridAreasAuto;
    size_t documentOrder = 0;
    for (auto gridItem : orderedGridItems) {
        if (gridItem->style()->position() == AbsolutePositionValue ||
            gridItem->style()->position() == FixedPositionValue) {
            m_nonGridItems.push_back(gridItem);
            continue;
        }

        GridArea* gridArea = new GridArea(gridItem, documentOrder);
        gridArea->parseGridRowAndColumnValues(*this);
        gridArea->resolveDefinitePositionValues();
        bool rowOk = addImplicitGridLineRows(gridArea);
        bool colOk = addImplicitGridLineColumns(gridArea);

        if (rowOk && colOk) {
            if (gridArea->isDefinite()) {
                gridAreasDefinite.push_back(gridArea);
            } else {
                gridAreasAuto.push_back(gridArea);
            }
        }

        documentOrder++;
    }

    placeGridAreasWithDefinitePositions(gridAreasDefinite);

    // 2. Process the items locked to a given row.
    placeGridAreasLockedToRows(gridAreasAuto);

    // 4. Position the remaining grid items.
    placeRemainingGridAreas(gridAreasAuto);

    for (auto gridArea : gridAreasDefinite) {
        m_orderedGridArea.push_back(*gridArea);
    }

    for (auto gridArea : gridAreasAuto) {
        m_orderedGridArea.push_back(*gridArea);
    }

    std::stable_sort(m_orderedGridArea.begin(), m_orderedGridArea.end(),
                     [](const GridArea& a, const GridArea& b) {
                         return a.rowStart() < b.rowStart();
                     });
}

void GridArea::parseGridRowAndColumnValues(GridFormattingContext& ctx)
{
    // NOTE: When `grid-row: n` is given, both gridRowStart() and gridRowEnd()
    // returns n. The value for `grid-row-end` will be correctly adjusted in the
    // later step. The same applies to `grid-column: n`.
    {
        String* gridRowStart = box()->style()->gridRowStart();
        String* gridRowEnd = box()->style()->gridRowEnd();

        m_rowStartLine = parseGridLine(gridRowStart);
        m_rowEndLine = parseGridLine(gridRowEnd);

        if (m_rowStartLine->hasSpan()) {
            if (m_rowStartLine->spanValue() < 1) {
                STARFISH_LOG_WARN("invalid value given");
            }
        } else if (m_rowStartLine->hasCustomIdent()) {
            GridArea* namedArea =
                ctx.getNamedGridArea(m_rowStartLine->customIdent());
            if (namedArea) {
                m_rowStartLine->setValue(namedArea->rowStart());
            }
        }

        if (m_rowEndLine->hasSpan()) {
            if (m_rowEndLine->spanValue() < 1) {
                STARFISH_LOG_WARN("invalid value given");
            }
        } else if (m_rowEndLine->hasCustomIdent()) {
            GridArea* namedArea =
                ctx.getNamedGridArea(m_rowEndLine->customIdent());
            if (namedArea) {
                m_rowEndLine->setValue(namedArea->rowEnd());
            }
        }

        if (m_rowStartLine->value() > 0 && m_rowEndLine->isAuto()) {
            m_rowEndLine->setValue(m_rowStartLine->value() +
                                   m_rowStartLine->spanValue());
        } else if (m_rowStartLine->isAuto() && m_rowEndLine->value() > 0) {
            m_rowStartLine->setValue(m_rowEndLine->value() -
                                     m_rowEndLine->spanValue());
        }
    }

    {
        String* gridColumnStart = box()->style()->gridColumnStart();
        String* gridColumnEnd = box()->style()->gridColumnEnd();

        m_columnStartLine = parseGridLine(gridColumnStart);
        m_columnEndLine = parseGridLine(gridColumnEnd);

        if (m_columnStartLine->hasSpan()) {
            if (m_columnStartLine->spanValue() < 1) {
                STARFISH_LOG_WARN("invalid span value given");
            }
        } else if (m_columnStartLine->hasCustomIdent()) {
            GridArea* namedArea =
                ctx.getNamedGridArea(m_columnStartLine->customIdent());
            if (namedArea) {
                m_columnStartLine->setValue(namedArea->columnStart());
            }
        }

        if (m_columnEndLine->hasSpan()) {
            if (m_columnEndLine->spanValue() < 1) {
                STARFISH_LOG_WARN("invalid value given");
            }
        } else if (m_columnEndLine->hasCustomIdent()) {
            GridArea* namedArea =
                ctx.getNamedGridArea(m_columnEndLine->customIdent());
            if (namedArea) {
                m_columnEndLine->setValue(namedArea->columnEnd());
            }
        }

        if (m_columnStartLine->value() > 0 && m_columnEndLine->isAuto()) {
            m_columnEndLine->setValue(m_columnStartLine->value() +
                                      m_columnStartLine->spanValue());
        } else if (m_columnStartLine->isAuto() &&
                   m_columnEndLine->value() > 0) {
            m_columnStartLine->setValue(m_columnEndLine->value() -
                                        m_columnEndLine->spanValue());
        }
    }
}

void GridArea::resolveDefinitePositionValues()
{
    {
        if (m_rowStartLine->isDefinite() && m_rowEndLine->isDefinite()) {
            if (rowStart() >= rowEnd()) {
                setRowEnd(rowStart() + 1);
            }
        } else if (m_rowStartLine->isDefinite()) {
            if (m_rowEndLine->isAuto()) {
                setRowEnd(rowStart() + 1);
            } else if (m_rowEndLine->hasSpan()) {
                setRowEnd(rowStart() + m_rowEndLine->spanValue());
            } else {
                STARFISH_LOG_WARN("missing values");
            }
        } else if (m_rowEndLine->isDefinite()) {
            if (m_rowStartLine->isAuto()) {
                setRowStart(rowEnd() - 1);
            } else if (m_rowStartLine->hasSpan()) {
                setRowStart(rowEnd() - m_rowStartLine->spanValue());
            } else {
                STARFISH_LOG_WARN("missing values");
            }
        }
    }

    {
        if (m_columnStartLine->isDefinite() && m_columnEndLine->isDefinite()) {
            if (columnStart() >= columnEnd()) {
                setColumnEnd(columnStart() + 1);
            }
        } else if (m_columnStartLine->isDefinite()) {
            if (m_columnEndLine->isAuto()) {
                setColumnEnd(columnStart() + 1);
            } else if (m_columnEndLine->hasSpan()) {
                setColumnEnd(columnStart() + m_columnEndLine->spanValue());
            } else {
                STARFISH_LOG_WARN("missing values");
            }
        } else if (m_columnEndLine->isDefinite()) {
            if (m_columnStartLine->isAuto()) {
                setColumnStart(columnEnd() - 1);
            } else if (m_columnStartLine->hasSpan()) {
                setColumnStart(columnEnd() - m_columnStartLine->spanValue());
            } else {
                STARFISH_LOG_WARN("missing values");
            }
        }
    }
}

GridLine* GridArea::parseGridLine(String* gridLineValue)
{
    auto val = gridLineValue->toUTF8NonGCString();
    CSSTokenVector tokens;
    CSSStyleDeclaration::tokenizeCSSValue(tokens, val.data(), val.length());

    GridLine* gridLine = new GridLine();
    bool foundSpan = false;
    for (size_t i = 0; i < tokens.size(); i++) {
        CSSTokenValue token = tokens[i];
        if (token.equals("auto")) {
        } else if (token.equals("span")) {
            foundSpan = true;
            if (i + 1 < tokens.size()) {
                CSSTokenValue nextToken = tokens[i + 1];
                String* nextTokenStr =
                    String::fromUTF8(nextToken.data(), nextToken.length());
                if (String::validDouble(nextTokenStr)) {
                    gridLine->setSpanValue(String::parseDouble(nextTokenStr));
                } else {
                    gridLine->setCustomIdent(nextTokenStr);
                }
                i++;
            } else {
                STARFISH_LOG_WARN("missing a value after span");
            }
        } else {
            String* tokenStr = String::fromUTF8(token.data(), token.length());
            if (String::validDouble(tokenStr)) {
                gridLine->setValue(String::parseDouble(tokenStr));
                if (i + 1 < tokens.size()) {
                    CSSTokenValue nextToken = tokens[i + 1];
                    String* nextTokenStr =
                        String::fromUTF8(nextToken.data(), nextToken.length());
                    if (!String::validDouble(nextTokenStr)) {
                        gridLine->setCustomIdent(nextTokenStr);
                        i++;
                    }
                }
            } else {
                gridLine->setCustomIdent(tokenStr);
            }
        }
    }

    return gridLine;
}

size_t GridArea::rowSpanValue()
{
    if (rowStart() > 0 && rowEnd() > 0) {
        return rowEnd() - rowStart();
    } else if (m_rowStartLine->hasSpan() && !m_rowEndLine->hasSpan()) {
        return m_rowStartLine->spanValue();
    } else if (!m_rowStartLine->hasSpan() && m_rowEndLine->hasSpan()) {
        return m_rowEndLine->spanValue();
    } else {
        return m_rowStartLine->spanValue();
    }
}

size_t GridArea::columnSpanValue()
{
    if (columnStart() > 0 && columnEnd() > 0) {
        return columnEnd() - columnStart();
    } else if (m_columnStartLine->hasSpan() && !m_columnEndLine->hasSpan()) {
        return m_columnStartLine->spanValue();
    } else if (!m_columnStartLine->hasSpan() && m_columnEndLine->hasSpan()) {
        return m_columnEndLine->spanValue();
    } else {
        return m_columnStartLine->spanValue();
    }
}

void GridFormattingContext::placeGridAreasWithDefinitePositions(
    GCVector<GridArea*>& gridAreaDefinite)
{
    for (auto gridArea : gridAreaDefinite) {
        placeGridArea(gridArea);
    }
}

bool GridFormattingContext::addImplicitGridLineRows(GridArea* gridArea)
{
    if (GridCellTable::MAX_TRACK < gridArea->rowStart() ||
        GridCellTable::MAX_TRACK < gridArea->rowEnd()) {
        STARFISH_LOG_WARN("exceeds the max number of rows");
        return false;
    }

    if (gridArea->rowStart() == 0 && gridArea->rowEnd() == 0) {
        return true;
    }

    size_t numOfRows = m_gridTemplateRows.size();
    if (gridArea->rowStart() < numOfRows && gridArea->rowEnd() <= numOfRows) {
        return true;
    }

    size_t numOfTracksToAdd = gridArea->rowEnd() - m_gridTemplateRows.size();
    for (size_t i = 0; i < numOfTracksToAdd; i++) {
        GridTrack track = GridTrack();
        m_gridTemplateRows.push_back(track);
    }

    return true;
}

bool GridFormattingContext::addImplicitGridLineColumns(GridArea* gridArea)
{
    if (GridCellTable::MAX_TRACK < gridArea->columnStart() ||
        GridCellTable::MAX_TRACK < gridArea->columnEnd()) {
        STARFISH_LOG_WARN("exceeds the max number of columns");
        return false;
    }

    if (gridArea->columnStart() == 0 && gridArea->columnEnd() == 0) {
        return true;
    }

    size_t numOfColumns = m_gridTemplateColumns.size();
    if (gridArea->columnStart() < numOfColumns &&
        gridArea->columnEnd() <= numOfColumns) {
        return true;
    }

    size_t numOfTracksToAdd =
        gridArea->columnEnd() - m_gridTemplateColumns.size();
    for (size_t i = 0; i < numOfTracksToAdd; i++) {
        m_gridTemplateColumns.push_back(GridTrack());
    }

    return true;
}

void GridFormattingContext::placeGridAreasLockedToRows(
    GCVector<GridArea*>& gridAreasAuto)
{
    // Impl "sparse" packing
    std::stable_sort(gridAreasAuto.begin(), gridAreasAuto.end(),
                     [](const GridArea* a, const GridArea* b) {
                         return a->rowStart() < b->rowStart();
                     });

    for (auto gridArea : gridAreasAuto) {
        if (gridArea->rowStart() == 0) {
            continue;
        }

        if (GridCellTable::MAX_TRACK < gridArea->rowStart()) {
            continue;
        }

        size_t firstEmptyColumn = 1;
        bool found = false;
        for (size_t c = 1; c < m_gridTemplateColumns.size(); c++) {
            if (hasAvailableGridCells(gridArea, gridArea->rowStart(), c)) {
                firstEmptyColumn = c;
                found = true;
                break;
            }
        }

        if (found) {
            gridArea->setColumnStart(firstEmptyColumn);
        } else {
            gridArea->setColumnStart(m_gridTemplateColumns.size());
        }
        gridArea->setColumnEnd(gridArea->columnStart() + 1);

        if (gridArea->columnEnd() > m_gridTemplateColumns.size()) {
            m_gridTemplateColumns.push_back(GridTrack());
        }

        placeGridArea(gridArea);
    }
}

void GridFormattingContext::placeRemainingGridAreas(
    GCVector<GridArea*>& gridAreasAuto)
{
    size_t firstImplicitRow = 1;
    size_t firstImplicitColumn = 1;
    bool startPosFound = false;
    for (size_t r = 1; r < m_gridTemplateRows.size() && !startPosFound; r++) {
        for (size_t c = 1; c < m_gridTemplateColumns.size() && !startPosFound;
             c++) {
            if (m_gridTemplateRows[r].isImplicitLine() &&
                m_gridTemplateColumns[c].isImplicitLine()) {
                firstImplicitRow = r;
                firstImplicitColumn = c;
                startPosFound = true;
            }
        }
    }

    size_t curRow = firstImplicitRow;
    size_t curCol = firstImplicitColumn;

    GridArea* prevGridArea = nullptr;
    for (auto gridArea : gridAreasAuto) {
        if (gridArea->hasRowAndColumnValues()) {
            continue;
        }

        if (gridArea->columnStart() != 0) {
            curCol = gridArea->columnStart();
            if (prevGridArea &&
                (prevGridArea->columnStart() > gridArea->columnStart())) {
                curRow++;
            }
        }

        bool found = false;
        for (size_t r = curRow; r < m_gridTemplateRows.size(); r++) {
            for (size_t c = curCol; c < m_gridTemplateColumns.size(); c++) {
                if (hasAvailableGridCells(gridArea, r, c)) {
                    curRow = r;
                    curCol = c;
                    found = true;
                    break;
                }
            }
            if (found) {
                break;
            }
            curCol = 1;
        }

        if (!found) {
            m_gridTemplateRows.push_back(GridTrack());
            curCol = 1;
            curRow = m_gridTemplateRows.size() - 1;
        }

        gridArea->setRowStart(curRow);
        gridArea->setRowEnd(gridArea->rowStart() + gridArea->rowSpanValue());

        if (gridArea->columnStart() == 0) {
            gridArea->setColumnStart(curCol);
        }
        if (gridArea->columnEnd() == 0) {
            gridArea->setColumnEnd(gridArea->columnStart() +
                                   gridArea->columnSpanValue());
        }

        placeGridArea(gridArea);
        prevGridArea = gridArea;
    }
}

bool GridFormattingContext::hasAvailableGridCells(GridArea* gridArea,
                                                  size_t row, size_t col)
{
    size_t width = gridArea->columnEnd() - gridArea->columnStart();
    if (width <= 0) {
        width = 1;
    }
    size_t height = gridArea->rowEnd() - gridArea->rowStart();
    if (height <= 0) {
        height = 1;
    }

    size_t availableCells = 0;
    for (size_t r = row; r < std::min(row + height, m_gridTemplateRows.size());
         r++) {
        for (size_t c = col;
             c < std::min(col + width, m_gridTemplateColumns.size()); c++) {
            if (m_gridCellTable.hasFreeSlot(r, c)) {
                availableCells++;
            }
        }
    }

    return availableCells == (width * height);
}

void GridFormattingContext::placeGridArea(GridArea* gridArea)
{
    for (size_t r = gridArea->rowStart(); r < gridArea->rowEnd(); r++) {
        for (size_t c = gridArea->columnStart(); c < gridArea->columnEnd();
             c++) {
            m_gridCellTable.setOccupied(r, c);
        }
    }
}

GridCellTable::GridCellTable()
{
    m_gridCellTable.reset();
}

bool GridCellTable::hasFreeSlot(size_t row, size_t col)
{
    if (GridCellTable::MAX_TRACK <= row || GridCellTable::MAX_TRACK <= col) {
        return false;
    }

    return !m_gridCellTable[(row * GridCellTable::MAX_TRACK) + col];
}

void GridCellTable::setOccupied(size_t row, size_t col)
{
    if (GridCellTable::MAX_TRACK <= row || GridCellTable::MAX_TRACK <= col) {
        return;
    }

    m_gridCellTable[(row * GridCellTable::MAX_TRACK) + col] = true;
}

std::string GridCellTable::toString()
{
    std::string str;
    for (size_t row = 1; row < MAX_TRACK; row++) {
        for (size_t col = 1; col < MAX_TRACK; col++) {
            if (hasFreeSlot(row, col)) {
                str += '.';
            } else {
                str += '*';
            }
        }
        str += '\n';
    }

    return str;
}

void GridFormattingContext::parseGridTemplateAreas()
{
    struct Area {
        size_t columnStart;
        size_t columnEnd;
        size_t rowStart;
        size_t rowEnd;
    };

    // Parsing GridTemplateAreas.
    String* str = m_container->style()->gridTemplateAreas();

    if (!str) {
        return;
    }

    auto raw = str->toUTF8NonGCString();

    CSSTokenVector tokens;
    CSSStyleDeclaration::tokenizeCSSValue(tokens, raw.data(), raw.length());

    std::unordered_multimap<std::string, Area> collector;
    SetForGrid<std::string> areaSet;

    for (size_t row = 0; row < tokens.size(); row++) {
        auto ss = tokens[row];
        ss.trim();
        CSSPropertyParser parser((char*)ss.data(), ss.length());
        parser.consumeContentString();
        const auto& separator = parser.parsedString();
        if (separator.length() == 0) {
            return;
        }

        auto s = separator;
        CSSTokenVector areas;
        CSSStyleDeclaration::tokenizeCSSValue(areas, s.data(), s.length());

        for (size_t col = 0; col < areas.size(); col++) {
            const std::string& name = areas[col];
            Area area;
            area.columnStart = col + 1;
            area.columnEnd = area.columnStart + 1;
            area.rowStart = row + 1;
            area.rowEnd = area.rowStart + 1;
            collector.insert(std::make_pair(name, area));
            areaSet.insert(name);
        }
    }

    for (const std::string& name : areaSet.set()) {
        std::vector<struct Area> stack;
        for (auto it = collector.find(name); it != collector.end(); it++) {
            if (name.compare(it->first)) {
                break;
            }

            if (!stack.size()) {
                stack.push_back(it->second);
            } else {
                bool merge = false;
                struct Area target = it->second;
                for (size_t i = 0; i < stack.size(); i++) {
                    struct Area* area = &stack[i];
                    SetForGrid<size_t> set;
                    if (area->columnStart == target.columnStart &&
                        area->columnEnd == target.columnEnd) {
                        set.insert(area->rowStart);
                        set.insert(area->rowEnd);
                        set.insert(target.rowStart);
                        set.insert(target.rowEnd);

                        if (set.size() != 3) {
                            continue;
                        }

                        std::vector<size_t>& orderedTracks = set.set();
                        size_t previous = orderedTracks[0];
                        for (size_t i = 1; i < orderedTracks.size(); i++) {
                            if ((orderedTracks[i] - previous) != 1) {
                                continue;
                            }
                            previous = orderedTracks[i];
                        }

                        area->rowStart = orderedTracks[0];
                        area->rowEnd = orderedTracks[2];
                        merge = true;

                        break;
                    } else if (area->rowStart == target.rowStart &&
                               area->rowEnd == target.rowEnd) {
                        set.insert(area->columnStart);
                        set.insert(area->columnEnd);
                        set.insert(target.columnStart);
                        set.insert(target.columnEnd);
                        if (set.size() != 3) {
                            continue;
                        }

                        std::vector<size_t>& orderedTracks = set.set();
                        size_t previous = orderedTracks[0];

                        for (size_t i = 1; i < orderedTracks.size(); i++) {
                            if ((orderedTracks[i] - previous) != 1) {
                                continue;
                            }
                            previous = orderedTracks[i];
                        }

                        area->columnStart = orderedTracks[0];
                        area->columnEnd = orderedTracks[2];
                        merge = true;
                        break;
                    }
                }

                if (!merge) {
                    stack.push_back(it->second);
                }
            }
        }

        if (stack.size() != 1) {
            while (stack.size() != 1) {
                struct Area target = stack.back();
                stack.pop_back();
                bool merge = false;
                for (size_t i = 0; i < stack.size(); i++) {
                    struct Area* area = &stack[i];
                    SetForGrid<size_t> set;
                    if (area->columnStart == target.columnStart &&
                        area->columnEnd == target.columnEnd) {
                        set.insert(area->rowStart);
                        set.insert(area->rowEnd);
                        set.insert(target.rowStart);
                        set.insert(target.rowEnd);

                        if (set.size() != 3) {
                            continue;
                        }

                        std::vector<size_t>& orderedTracks = set.set();

                        size_t previous = orderedTracks[0];

                        for (size_t i = 1; i < orderedTracks.size(); i++) {
                            if ((orderedTracks[i] - previous) != 1) {
                                continue;
                            }
                            previous = orderedTracks[i];
                        }

                        area->rowStart = orderedTracks[0];
                        area->rowEnd = orderedTracks[2];
                        merge = true;
                        break;
                    } else if (area->rowStart == target.rowStart &&
                               area->rowEnd == target.rowEnd) {
                        set.insert(area->columnStart);
                        set.insert(area->columnEnd);
                        set.insert(target.columnStart);
                        set.insert(target.columnEnd);
                        if (set.size() != 3) {
                            continue;
                        }

                        std::vector<size_t>& orderedTracks = set.set();

                        size_t previous = orderedTracks[0];

                        for (size_t i = 1; i < orderedTracks.size(); i++) {
                            if ((orderedTracks[i] - previous) != 1) {
                                continue;
                            }
                            previous = orderedTracks[i];
                        }

                        area->columnStart = orderedTracks[0];
                        area->columnEnd = orderedTracks[2];
                        merge = true;
                        break;
                    }
                }

                if (merge) {
                    struct Area area = stack.back();
                    GridArea gridArea(nullptr, -1, area.rowStart, area.rowEnd,
                                      area.columnStart, area.columnEnd);
                    insertNamedGridArea(std::make_pair(name, gridArea));
                } else {
                    return;
                }
            }
        } else {
            struct Area area = stack.back();
            GridArea gridArea(nullptr, -1, area.rowStart, area.rowEnd,
                              area.columnStart, area.columnEnd);
            insertNamedGridArea(std::make_pair(name, gridArea));
        }
    }
}

void GridFormattingContext::initializeGridTrackColumns(
    const GCVector<GridTrackSize>* gridTrackColumns)
{
    STARFISH_ASSERT(gridTrackColumns);

    for (auto& trackSize : *gridTrackColumns) {
        GridLength gridLength = trackSize.min();
        LayoutUnit baseSize = intMaxForLayoutUnit;

        if (trackSize.isLength()) {
            if (gridLength.isLength() &&
                gridLength.length().isDefinite(m_availableWidth !=
                                               intMaxForLayoutUnit)) {
                baseSize = gridLength.length().specifiedValue(m_availableWidth,
                                                              m_container);
                if (m_availableWidth == 0 && gridLength.length().isPercent()) {
                    m_gridTemplateColumns.push_back(GridTrack(GridTrack::Auto));
                } else {
                    m_gridTemplateColumns.push_back(GridTrack(baseSize));
                }
            } else if (gridLength.isAuto()) {
                m_gridTemplateColumns.push_back(GridTrack(GridTrack::Auto));
            }
        } else if (trackSize.isFr()) {
            GridTrack track = GridTrack(gridLength.fr(), false);
            m_gridTemplateColumns.push_back(track);
        } else if (trackSize.isMinMax()) {
            // TODO: support values other than fixed
            GridTrack track = GridTrack(trackSize.min(), trackSize.max());
            m_gridTemplateColumns.push_back(track);
        } else if (trackSize.isMinContent()) {
            m_gridTemplateColumns.push_back(GridTrack(GridTrack::MinContent));
        } else if (trackSize.isMaxContent()) {
            m_gridTemplateColumns.push_back(GridTrack(GridTrack::MaxContent));
        }
    }
}

void GridFormattingContext::initializeGridTrackRows(
    const GCVector<GridTrackSize>* gridTrackRows)
{
    STARFISH_ASSERT(gridTrackRows);

    for (auto& trackSize : *gridTrackRows) {
        GridLength gridLength = trackSize.min();

        if (trackSize.isLength()) {
            if (gridLength.isLength() && gridLength.length().isFixed()) {
                Length length = gridLength.length();
                GridTrack track = GridTrack(length.numberData());
                m_gridTemplateRows.push_back(track);
            } else if (gridLength.isAuto()) {
                m_gridTemplateRows.push_back(GridTrack(GridTrack::Auto));
            }
        } else if (trackSize.isFr()) {
            GridTrack track = GridTrack(gridLength.fr(), false);
            m_gridTemplateRows.push_back(track);
        } else if (trackSize.isMinMax()) {
            GridTrack track = GridTrack(trackSize.min(), trackSize.max());
            m_gridTemplateRows.push_back(track);
        } else if (trackSize.isMinContent()) {
            m_gridTemplateRows.push_back(GridTrack(GridTrack::MinContent));
        } else if (trackSize.isMaxContent()) {
            m_gridTemplateRows.push_back(GridTrack(GridTrack::MaxContent));
        }
    }
}

void GridFormattingContext::buildGridTrackTemplate()
{
    GridTrack dummyPlaceholder = GridTrack(0);
    m_gridTemplateColumns.push_back(dummyPlaceholder);
    const GCVector<GridTrackSize>* columns =
        m_container->style()->gridTemplateColumns();
    if (columns) {
        initializeGridTrackColumns(columns);
    } else {
        m_gridTemplateColumns.push_back(GridTrack());
    }

    m_gridTemplateRows.push_back(dummyPlaceholder);
    const GCVector<GridTrackSize>* rows =
        m_container->style()->gridTemplateRows();
    if (rows) {
        initializeGridTrackRows(rows);
    }

    parseGridTemplateAreas();
    placeGridItemsIntoCells();

    initializePreferredWidths();
    resolveIntrinsicColumnTrackSizes();
    maximizeColumnTracks();
    expandFrColumnTracks();
    stretchAutoColumnTracks();

    initializeContentHeights();
    applyImplicitTrackSizing();
    resolveIntrinsicRowTrackSizes();
    expandFrRowTracks();
    layoutGridItemFrameBoxes();
}
LayoutSize GridFormattingContext::fetchFixedMargin(FrameGridBox* grid,
                                                   ComputedStyle* style)
{
    LayoutSize result;

    LengthData margin = style->margin();
    if (margin.left().isDefinite(false)) {
        result.setWidth(result.width() + margin.left().specifiedValue(0, grid));
    }
    if (margin.right().isDefinite(false)) {
        result.setWidth(result.width() +
                        margin.right().specifiedValue(0, grid));
    }
    if (margin.top().isDefinite(false)) {
        result.setHeight(result.height() +
                         margin.top().specifiedValue(0, grid));
    }
    if (margin.bottom().isDefinite(false)) {
        result.setHeight(result.height() +
                         margin.bottom().specifiedValue(0, grid));
    }

    return result;
}

LayoutSize GridFormattingContext::fetchFixedMarginBorderPadding(
    FrameGridBox* grid, ComputedStyle* style)
{
    LayoutSize result;
    // margin
    result = fetchFixedMargin(grid, style);

    // border
    BorderData border = style->border();
    if (border.left().width().isDefinite(false)) {
        result.setWidth(result.width() +
                        border.left().width().specifiedValue(0, grid));
    }
    if (border.right().width().isDefinite(false)) {
        result.setWidth(result.width() +
                        border.right().width().specifiedValue(0, grid));
    }
    if (border.top().width().isDefinite(false)) {
        result.setHeight(result.height() +
                         border.top().width().specifiedValue(0, grid));
    }
    if (border.bottom().width().isDefinite(false)) {
        result.setHeight(result.height() +
                         border.bottom().width().specifiedValue(0, grid));
    }

    // padding
    LengthData padding = style->padding();
    if (padding.left().isDefinite(false)) {
        result.setWidth(result.width() +
                        padding.left().specifiedValue(0, grid));
    }
    if (padding.right().isDefinite(false)) {
        result.setWidth(result.width() +
                        padding.right().specifiedValue(0, grid));
    }
    if (padding.top().isDefinite(false)) {
        result.setHeight(result.height() +
                         padding.top().specifiedValue(0, grid));
    }
    if (padding.bottom().isDefinite(false)) {
        result.setHeight(result.height() +
                         padding.bottom().specifiedValue(0, grid));
    }

    return result;
}

void GridFormattingContext::initializePreferredWidths()
{
    for (GridArea& gridArea : m_orderedGridArea) {
        FrameBox* gridItemBox = gridArea.box();
        ComputedStyle* style = gridItemBox->style();

        LayoutUnit contentWidth;
        LayoutUnit preferredMinWidth;

        LayoutSize mbp = fetchFixedMarginBorderPadding(m_container, style);

        if (style->width().isFixed()) {
            LayoutUnit width = style->width().fixed() + mbp.width();
            preferredMinWidth = contentWidth = width;
        } else {
            auto cache = m_layoutContext.testGridItemPreferredWidthCache(
                gridItemBox, m_availableWidth);
            if (cache.hasValue()) {
                contentWidth = cache.getValue() + mbp.width();
            } else {
                PreferredWidthContext p(m_layoutContext, nullptr, gridItemBox,
                                        gridItemBox, m_availableWidth);
                p.computePreferredWidth();
                contentWidth = p.preferredWidth() + mbp.width();
                preferredMinWidth = p.preferredMinWidth() + mbp.width();
                m_layoutContext.registerToGridItemPreferredWidthCache(
                    gridItemBox, m_availableWidth, p.preferredWidth());
            }
        }
        gridArea.setPreferredWidth(contentWidth);
        gridArea.setPreferredMinWidth(preferredMinWidth);
    }
}

void GridFormattingContext::resolveIntrinsicColumnTrackSizes()
{
    GCVector<GridArea*> gridAreasWithSpans;
    // 1-2
    // Cal widths using only the items with span=1.
    // Consider widths with min-content, max-content, auto, and fit-content.
    for (auto& gridArea : m_orderedGridArea) {
        if (gridArea.columnEnd() - gridArea.columnStart() > 1) {
            gridAreasWithSpans.push_back(&gridArea);
            continue;
        }

        GridTrack& track = m_gridTemplateColumns[gridArea.columnStart()];
        LayoutUnit maxContent =
            std::max(track.size(), gridArea.preferredWidth());
        LayoutUnit minContent =
            std::max(track.size(), gridArea.preferredMinWidth());

        if (track.isAuto() || track.isImplicitLine()) {
            track.setSize(minContent);
            track.setGrowthLimit(maxContent);
        } else if (track.isMinContent()) {
            track.setSize(minContent);
        } else if (track.isMaxContent()) {
            track.setSize(maxContent);
        } else if (track.isMinMax()) {
            // TODO: min- and max-content
            if (track.min().isFixed()) {
                track.setSize(track.min().length().fixed());
            } else if (track.min().isAuto()) {
                track.setSize(minContent);
            }

            if (track.max().isFixed()) {
                track.setGrowthLimit(track.max().length().fixed());
            }
        } else if (track.isFr()) {
            track.setSize(minContent);
        }
    }

    std::stable_sort(gridAreasWithSpans.begin(), gridAreasWithSpans.end(),
                     [](const GridArea* a, const GridArea* b) {
                         STARFISH_ASSERT(a != nullptr);
                         STARFISH_ASSERT(b != nullptr);

                         size_t spanLength1 = a->columnEnd() - a->columnStart();
                         size_t spanLength2 = b->columnEnd() - b->columnStart();
                         return spanLength1 < spanLength2;
                     });
    increaseColumnGridTracksForSpans(gridAreasWithSpans);

    // 4: Cal widths for the items with spans > 1, and with fr
}

void GridFormattingContext::increaseColumnGridTracksForSpans(
    GCVector<GridArea*>& gridAreasWithSpans)
{
    // 3: Adjust the size of widths considering spans
    // intrinsic: min-content, max-content, auto, fit-content
    // no fr and minmax(intrinsic, _);
    // 3.1, increase basesize of a track with minmax(intrinsic, _)
    // 3.2, increase basesize of a track with
    //      minmax(min-content | max-content, _)
    // 3.3, increase basesize of a track with minmax(auto | max-content, _), if
    //      the grid container is being sized unera max-content constraint.
    // 3.4, if growthLimit < baseSize, growthLimit = baseSize
    // 3.5, increase the growthLimit of a track with minmax(_, intrinsic)
    // 3.6, increase the growthLimit of a track with minmax(_, max-content)

    for (auto gridArea : gridAreasWithSpans) {
        if (isFrPartOfTrack(gridArea)) {
            continue;
        }

        LayoutUnit sumOfTracks = 0;
        LayoutUnit numOfAutoTracks = 0;
        for (size_t i = gridArea->columnStart(); i < gridArea->columnEnd();
             i++) {
            GridTrack* track = &m_gridTemplateColumns[i];
            sumOfTracks += track->size();
            if (track->isAuto()) {
                numOfAutoTracks += 1;
            }
        }

        if (numOfAutoTracks == 0) {
            continue;
        }

        LayoutUnit requiredSpace = gridArea->preferredWidth() - sumOfTracks;
        LayoutUnit eachColumnSize = requiredSpace.toDouble() / numOfAutoTracks;
        for (size_t i = gridArea->columnStart(); i < gridArea->columnEnd();
             i++) {
            GridTrack* track = &m_gridTemplateColumns[i];
            if (track->isAuto()) {
                track->setSize(std::max(track->size(), eachColumnSize));
                track->setGrowthLimit(track->size());
            }
        }
    }
}

bool GridFormattingContext::isFrPartOfTrack(GridArea* gridArea)
{
    bool hasFr = false;
    for (size_t i = gridArea->columnStart(); i < gridArea->columnEnd(); i++) {
        if (m_gridTemplateColumns[i].isFr()) {
            hasFr = true;
            break;
        }
    }
    return hasFr;
}

bool GridFormattingContext::isFrPartOfRowTrack(GridArea* gridArea)
{
    bool hasFr = false;
    for (size_t i = gridArea->rowStart(); i < gridArea->rowEnd(); i++) {
        if (m_gridTemplateRows[i].isFr()) {
            hasFr = true;
            break;
        }
    }
    return hasFr;
}

void GridFormattingContext::maximizeColumnTracks()
{
    LayoutUnit sumOfColumnWidths = 0;
    GCVector<GridTrack*> growableTracks;
    for (size_t i = 1; i < m_gridTemplateColumns.size(); i++) {
        GridTrack& track = m_gridTemplateColumns[i];
        sumOfColumnWidths += track.size();
        if (track.isAuto()) {
            growableTracks.push_back(&track);
        } else if (track.isMinMax()) {
            if (track.max().isFixed()) {
                growableTracks.push_back(&track);
            }
        }
    }

    LayoutUnit gapSpace = m_columnGap * (m_gridTemplateColumns.size() - 2);
    LayoutUnit remainingSpace =
        m_availableWidth - (sumOfColumnWidths + gapSpace);

    if (remainingSpace <= 0) {
        return;
    }

    LayoutUnit availableSpace = remainingSpace;
    // Iterate until either all remaining space is allocated or tracks
    // can no longer be extended.
    while (availableSpace.toInt() > 0 && growableTracks.size() > 0) {
        LayoutUnit additionalWidth = availableSpace / growableTracks.size();

        GCVector<GridTrack*> remainingGrowableTracks;
        for (auto track : growableTracks) {
            LayoutUnit newWidth = track->size() + additionalWidth;

            if (newWidth >= track->growthLimit()) {
                availableSpace -= track->growthLimit() - track->size();
                track->setSize(track->growthLimit());
            } else {
                availableSpace -= additionalWidth;
                track->setSize(newWidth);
                remainingGrowableTracks.push_back(track);
            }
        }

        growableTracks.clear();
        growableTracks.insert(growableTracks.end(),
                              remainingGrowableTracks.begin(),
                              remainingGrowableTracks.end());
    }
}

void GridFormattingContext::expandFrColumnTracks()
{
    LayoutUnit sumOfFrs = 0;
    LayoutUnit sumOfColumnWidths = 0;
    for (size_t i = 1; i < m_gridTemplateColumns.size(); i++) {
        GridTrack& track = m_gridTemplateColumns[i];

        if (track.isFr()) {
            sumOfFrs += track.fr();
        } else if (track.isMinMax() && track.max().isFr()) {
            sumOfFrs += LayoutUnit(track.max().fr());
        } else {
            sumOfColumnWidths += m_gridTemplateColumns[i].size();
        }
    }

    if (sumOfFrs < 1) {
        sumOfFrs = 1;
    }

    LayoutUnit gapSpace = m_columnGap * (m_gridTemplateColumns.size() - 2);
    LayoutUnit remainingSpace =
        m_availableWidth - (sumOfColumnWidths + gapSpace);

    if (remainingSpace <= 0) {
        return;
    }

    for (size_t i = 1; i < m_gridTemplateColumns.size(); i++) {
        GridTrack& track = m_gridTemplateColumns[i];

        if (track.isFr()) {
            LayoutUnit width =
                (track.fr().toDouble() / sumOfFrs) * remainingSpace;
            track.setSize(std::max(track.size(), width));
        } else if (track.isMinMax() && track.max().isFr()) {
            LayoutUnit width = (track.max().fr() / sumOfFrs) * remainingSpace;
            track.setSize(std::max(track.size(), width));
        }
    }
}

void GridFormattingContext::stretchAutoColumnTracks()
{
    if (m_availableWidth <= 0) {
        return;
    }

    if (!m_container->canStratchItem()) {
        return;
    }

    LayoutUnit sumOfAllNonAutoWidths;
    int numOfAutoTracks = 0;
    for (size_t i = 1; i < m_gridTemplateColumns.size(); i++) {
        GridTrack& track = m_gridTemplateColumns[i];
        sumOfAllNonAutoWidths += track.size();
        if (track.isAuto() && !track.isMinContent() && !track.isMaxContent()) {
            numOfAutoTracks++;
        }
    }

    if (numOfAutoTracks == 0) {
        return;
    }

    LayoutUnit gapSpace = m_columnGap * (m_gridTemplateColumns.size() - 2);
    LayoutUnit remainingSpace =
        m_availableWidth - (sumOfAllNonAutoWidths + gapSpace);

    if (remainingSpace <= 0) {
        return;
    }

    LayoutUnit additionalAutoTrackSpace =
        remainingSpace.toDouble() / numOfAutoTracks;
    for (size_t i = 1; i < m_gridTemplateColumns.size(); i++) {
        GridTrack& track = m_gridTemplateColumns[i];
        if (track.isAuto() && !track.isMinContent() && !track.isMaxContent()) {
            track.setSize(track.size() + additionalAutoTrackSpace);
        }
    }
}

void GridFormattingContext::applyImplicitTrackSizing()
{
    const GCVector<GridTrackSize>* rows =
        m_container->style()->gridTemplateRows();

    if (rows) {
        return;
    }

    if (m_container->hasFixedStyleHeight()) {
        double height = m_container->style()->height().fixed();
        double eachRowHeight = height / (m_gridTemplateRows.size() - 1);
        for (size_t i = 1; i < m_gridTemplateRows.size(); i++) {
            GridTrack& track = m_gridTemplateRows[i];
            track.setSize(LayoutUnit(eachRowHeight));
        }
    }
}

void GridFormattingContext::initializeContentHeights()
{
    for (GridArea& gridArea : m_orderedGridArea) {
        FrameBox* gridItem = gridArea.box();
        GridLayoutScope scope(gridItem);
        ComputedStyle* style = gridItem->style();
        LayoutSize mbp = fetchFixedMarginBorderPadding(m_container, style);
        LayoutSize margin = fetchFixedMargin(m_container, style);

        LayoutUnit contentHeight = 0;

        if (style->height().isFixed()) {
            if (style->boxSizing() == BoxSizingValue::BorderBoxBoxSizingValue) {
                contentHeight = style->height().fixed() + margin.height();
            } else {
                contentHeight = style->height().fixed() + mbp.height();
            }
        } else {
            layoutGridItemFrameBox(gridArea, true);
            contentHeight = gridItem->contentHeight() + mbp.height();
        }

        gridArea.setContentHeight(contentHeight);
    }
}

void GridFormattingContext::resolveIntrinsicRowTrackSizes()
{
    GCVector<GridArea*> gridAreasWithSpans;

    for (GridArea& gridArea : m_orderedGridArea) {
        if (gridArea.rowEnd() - gridArea.rowStart() > 1) {
            gridAreasWithSpans.push_back(&gridArea);
            continue;
        }

        GridTrack& track = m_gridTemplateRows[gridArea.rowStart()];
        LayoutUnit maxContent =
            std::max(track.size(), gridArea.contentHeight());

        if (track.isAuto() || track.isImplicitLine()) {
            track.setSize(maxContent);
            track.setGrowthLimit(maxContent);
        } else if (track.isFr()) {
            track.setSize(maxContent);
        }
    }

    std::stable_sort(gridAreasWithSpans.begin(), gridAreasWithSpans.end(),
                     [](const GridArea* a, const GridArea* b) {
                         STARFISH_ASSERT(a != nullptr);
                         STARFISH_ASSERT(b != nullptr);

                         size_t spanLength1 = a->columnEnd() - a->columnStart();
                         size_t spanLength2 = b->columnEnd() - b->columnStart();
                         return spanLength1 < spanLength2;
                     });
    increaseRowGridTracksForSpans(gridAreasWithSpans);
}

void GridFormattingContext::increaseRowGridTracksForSpans(
    GCVector<GridArea*>& gridAreasWithSpans)
{
    for (auto gridArea : gridAreasWithSpans) {
        if (isFrPartOfRowTrack(gridArea)) {
            continue;
        }

        LayoutUnit sumOfTracks = 0;
        LayoutUnit numOfAutoTracks = 0;
        for (size_t i = gridArea->rowStart(); i < gridArea->rowEnd(); i++) {
            GridTrack* track = &m_gridTemplateRows[i];
            sumOfTracks += track->size();
            if (track->isAuto()) {
                numOfAutoTracks += 1;
            }
        }

        if (numOfAutoTracks == 0) {
            continue;
        }

        LayoutUnit requiredSpace = gridArea->contentHeight() - sumOfTracks;
        LayoutUnit additionalRowHeight =
            requiredSpace.toDouble() / numOfAutoTracks;

        if (additionalRowHeight <= 0) {
            continue;
        }

        for (size_t i = gridArea->rowStart(); i < gridArea->rowEnd(); i++) {
            GridTrack* track = &m_gridTemplateRows[i];
            if (track->isAuto()) {
                // increase size
                track->setSize(track->size() + additionalRowHeight);
                track->setGrowthLimit(track->size());
            }
        }
    }
}

void GridFormattingContext::expandFrRowTracks()
{
    LayoutUnit sumOfFrs = 0;
    LayoutUnit sumOfRowHeights = 0;
    LayoutUnit maxHeightSoFar = 0;
    for (size_t i = 1; i < m_gridTemplateRows.size(); i++) {
        GridTrack& track = m_gridTemplateRows[i];

        if (track.isFr()) {
            sumOfFrs += track.fr();
            maxHeightSoFar = std::max(maxHeightSoFar, track.size());
        } else if (track.isMinMax() && track.max().isFr()) {
            sumOfFrs += LayoutUnit(track.max().fr());
        } else {
            sumOfRowHeights += m_gridTemplateRows[i].size();
        }
    }

    if (sumOfFrs < 1) {
        sumOfFrs = 1;
    }

    LayoutUnit flexFraction = 0;
    LayoutUnit flexFactor = 1;
    if (m_container->hasFixedStyleHeight()) {
        LayoutUnit availableHeight = m_container->style()->height().fixed();
        LayoutUnit gapSpace = m_rowGap * (m_gridTemplateRows.size() - 2);
        LayoutUnit remainingSpace =
            availableHeight - (sumOfRowHeights + gapSpace);
        flexFraction = remainingSpace;
        flexFactor = sumOfFrs;
    } else {
        flexFraction = maxHeightSoFar;
    }

    if (flexFraction <= 0) {
        return;
    }

    for (size_t i = 1; i < m_gridTemplateRows.size(); i++) {
        GridTrack& track = m_gridTemplateRows[i];

        if (track.isFr()) {
            LayoutUnit height =
                (track.fr().toDouble() / flexFactor) * flexFraction;
            track.setSize(std::max(track.size(), height));
        } else if (track.isMinMax() && track.max().isFr()) {
            LayoutUnit height = (track.max().fr() / flexFactor) * flexFraction;
            track.setSize(std::max(track.size(), height));
        }
    }
}

void GridFormattingContext::layoutGridItemFrameBoxes()
{
    for (GridArea& gridArea : m_orderedGridArea) {
        layoutGridItemFrameBox(gridArea, false);
    }
}

void GridFormattingContext::layoutGridItemFrameBox(GridArea& gridArea,
                                                   bool widthOnly)
{
    FrameBox* gridItem = gridArea.box();
    GridLayoutScope scope(gridItem);
    ComputedStyle* style = gridItem->style();

    LayoutSize margin = fetchFixedMargin(m_container, style);
    LayoutSize mbp = fetchFixedMarginBorderPadding(m_container, style);

    LayoutUnit width = 0;
    if (style->width().isFixed()) {
        width = style->width().fixed() + mbp.width();
    } else {
        if (gridArea.isMarginLeftAuto() || gridArea.isMarginRightAuto()) {
            width = gridArea.preferredWidth();
        } else {
            GridTrack& colTrack = m_gridTemplateColumns[gridArea.columnStart()];
            for (size_t i = gridArea.columnStart(); i < gridArea.columnEnd();
                 i++) {
                width += m_gridTemplateColumns[i].size();
            }

            width += ((gridArea.columnEnd() - gridArea.columnStart() - 1) *
                      m_columnGap);
        }
        if (style->boxSizing() == BoxSizingValue::BorderBoxBoxSizingValue) {
            width += mbp.width() - margin.width();
        }
    }

    LayoutUnit widthWillBe = width;

    // ref: https://www.w3.org/TR/css-grid-1/#auto-margins
    // TODO: auto margins absorb positive free space prior to alignment via the
    // box alignment properties.
    if (style->margin().left().isAuto()) {
        style->setMarginLeft(Length(Length::Fixed, 0));
    } else {
        style->setMarginLeft(
            Length(Length::Fixed,
                   style->margin().left().specifiedValue(width, m_container)));
        widthWillBe -= style->margin().left().fixed();
    }

    if (style->margin().right().isAuto()) {
        style->setMarginRight(Length(Length::Fixed, 0));
    } else {
        style->setMarginRight(
            Length(Length::Fixed,
                   style->margin().right().specifiedValue(width, m_container)));
        widthWillBe -= style->margin().right().fixed();
    }

    if (style->padding().left().isAuto()) {
        style->setPaddingLeft(Length(Length::Fixed, 0));
    } else {
        style->setPaddingLeft(
            Length(Length::Fixed,
                   style->padding().left().specifiedValue(width, m_container)));
        widthWillBe -= style->padding().left().fixed();
    }

    if (style->padding().right().isAuto()) {
        style->setPaddingRight(Length(Length::Fixed, 0));
    } else {
        style->setPaddingRight(Length(
            Length::Fixed,
            style->padding().right().specifiedValue(width, m_container)));
        widthWillBe -= style->padding().right().fixed();
    }

    // Initialize the border to avoid setting an unintended value when setting
    // the border in layout GridItem() in a situation where no border is given.
    if (style->nullableBorder().hasValue() == false) {
        style->rareComputedStyleData()->ensureBorder()->makeZeroWidth();
    }

    style->setBorderLeftWidth(Length(
        Length::Fixed,
        style->border().left().width().specifiedValue(width, m_container)));
    widthWillBe -= style->border().left().width().fixed();
    style->setBorderRightWidth(Length(
        Length::Fixed,
        style->border().right().width().specifiedValue(width, m_container)));
    widthWillBe -= style->border().right().width().fixed();

    width = widthWillBe;
    if (width < 0) {
        width = 0;
    }

    style->setWidth(Length(Length::Fixed, width));

    if (widthOnly) {
        if (needsGridItemLayout(gridItem, style, true)) {
            gridItem->markNeedsLayout();
        }
        gridItem->layout(m_layoutContext,
                         Frame::LayoutWantToResolve::ResolveAll);
        return;
    }

    LayoutUnit height = 0;
    if (style->height().isFixed()) {
        height = style->height().fixed() + mbp.height();
    } else {
        GridTrack& rowTrack = m_gridTemplateRows[gridArea.rowStart()];
        for (size_t i = gridArea.rowStart(); i < gridArea.rowEnd(); i++) {
            height += m_gridTemplateRows[i].size();
        }

        height += ((gridArea.rowEnd() - gridArea.rowStart() - 1) * m_rowGap);
        if (style->boxSizing() == BoxSizingValue::BorderBoxBoxSizingValue) {
            height += mbp.height() - margin.height();
        }
    }

    LayoutUnit heightWillBe = height;

    if (style->margin().top().isAuto()) {
        style->setMarginTop(Length(Length::Fixed, 0));
    } else {
        style->setMarginTop(
            Length(Length::Fixed,
                   style->margin().top().specifiedValue(height, m_container)));
        heightWillBe -= style->margin().top().fixed();
    }

    if (style->margin().bottom().isAuto()) {
        style->setMarginBottom(Length(Length::Fixed, 0));
    } else {
        style->setMarginBottom(Length(
            Length::Fixed,
            style->margin().bottom().specifiedValue(height, m_container)));
        heightWillBe -= style->margin().bottom().fixed();
    }

    if (style->padding().top().isAuto()) {
        style->setPaddingTop(Length(Length::Fixed, 0));
    } else {
        style->setPaddingTop(
            Length(Length::Fixed,
                   style->padding().top().specifiedValue(height, m_container)));
        heightWillBe -= style->padding().top().fixed();
    }

    if (style->padding().bottom().isAuto()) {
        style->setPaddingBottom(Length(Length::Fixed, 0));
    } else {
        style->setPaddingBottom(Length(
            Length::Fixed,
            style->padding().bottom().specifiedValue(height, m_container)));
        heightWillBe -= style->padding().bottom().fixed();
    }

    style->setBorderTopWidth(Length(
        Length::Fixed,
        style->border().top().width().specifiedValue(height, m_container)));
    heightWillBe -= style->border().top().width().fixed();
    style->setBorderBottomWidth(Length(
        Length::Fixed,
        style->border().bottom().width().specifiedValue(height, m_container)));
    heightWillBe -= style->border().bottom().width().fixed();

    height = heightWillBe;

    if (height < 0) {
        height = 0;
    }

    style->setHeight(Length(Length::Fixed, height));

    if (gridItem->needsLayout() ||
        needsGridItemLayout(gridItem, style, false)) {
        gridItem->markNeedsLayout();
        gridItem->layout(m_layoutContext,
                         Frame::LayoutWantToResolve::ResolveAll);
    }
}

void GridFormattingContext::applyAlignItems()
{
    AlignItemValue alignItem = m_container->style()->alignItems();

    if (alignItem == AlignItemValue::CenterAlignItemValue) {
        applyAlignItemsCenter();
    }
}

void GridFormattingContext::applyAlignItemsCenter()
{
    GCVector<LayoutUnit> yOffsetsForRows;
    LayoutUnit yOffsetForRowsSoFar = 0;
    yOffsetsForRows.push_back(yOffsetForRowsSoFar);
    for (size_t i = 1; i < m_gridTemplateRows.size(); i++) {
        yOffsetsForRows.push_back(yOffsetForRowsSoFar);
        GridTrack& track = m_gridTemplateRows[i];
        yOffsetForRowsSoFar += m_rowGap + track.size();
    }

    for (GridArea& area : m_orderedGridArea) {
        STARFISH_ASSERT(area.rowStart() < yOffsetsForRows.size());
        area.box()->setHeight(area.contentHeight());
        LayoutUnit yOffset = yOffsetsForRows[area.rowStart()];

        LayoutUnit trackSize;
        for (size_t i = area.rowStart(); i < area.rowEnd(); i++) {
            trackSize += m_gridTemplateRows[i].size();
        }
        trackSize += (area.rowEnd() - area.rowStart() - 1) * m_rowGap;

        LayoutUnit yPos =
            yOffset + (trackSize / 2) - (area.box()->height() / 2);
        area.box()->setY(yPos);
    }
}

void GridFormattingContext::layoutNonGridItems()
{
    // https://drafts.csswg.org/css-grid/#abspos
    // https://www.w3.org/TR/css-position-3/#staticpos-rect
    // GridItems with position: absolute do not participate in the grid layout.
    LayoutUnit xPosSoFar =
        m_container->borderLeft() + m_container->paddingLeft();
    LayoutUnit yPosSoFar = m_container->borderTop() + m_container->paddingTop();

    for (auto nonGridItem : m_nonGridItems) {
        GridLayoutScope scope(nonGridItem);

        nonGridItem->layout(m_layoutContext,
                            Frame::LayoutWantToResolve::ResolveAll);

        auto position = nonGridItem->style()->position();
        if (position == AbsolutePositionValue) {
            nonGridItem->setX(xPosSoFar);
            nonGridItem->setY(yPosSoFar);
        } else if (position == FixedPositionValue) {
            repositionFixedNonGridItem(nonGridItem);
        }
    }
}

void GridFormattingContext::repositionFixedNonGridItem(FrameBox* nonGridItem)
{
    LayoutUnit xPosSoFar =
        m_container->borderLeft() + m_container->paddingLeft();
    LayoutUnit yPosSoFar = m_container->borderTop() + m_container->paddingTop();

    LayoutLocation absLocation =
        m_container->absolutePoint(m_layoutContext.frameDocument());
    LengthData insets = m_container->insets();

    if (nonGridItem->style()->top().isAuto()) {
        nonGridItem->setY(yPosSoFar);
    } else if (nonGridItem->style()->top().isFixed()) {
        if (insets.top().isFixed()) {
            nonGridItem->setY(nonGridItem->style()->top().fixed() -
                              absLocation.y().toDouble() -
                              insets.top().fixed());
        } else {
            STARFISH_UNIMPLEMENTED("insets: percentage");
        }
    } else {
        STARFISH_UNIMPLEMENTED("nonGridItem: other units");
    }

    if (nonGridItem->style()->left().isAuto()) {
        nonGridItem->setX(xPosSoFar);
    } else if (nonGridItem->style()->left().isFixed()) {
        if (insets.left().isFixed()) {
            nonGridItem->setX(nonGridItem->style()->left().fixed() -
                              absLocation.x().toDouble() -
                              insets.left().fixed());
        } else {
            STARFISH_UNIMPLEMENTED("insets: percentage");
        }
    } else {
        STARFISH_UNIMPLEMENTED("nonGridItem: other units");
    }
}

bool GridFormattingContext::needsGridItemLayout(FrameBox* gridItem,
                                                ComputedStyle* style,
                                                bool testWidthOnly)
{
    bool changed = false;

    if (style->boxSizing() == BoxSizingValue::BorderBoxBoxSizingValue) {
        changed = changed || (gridItem->width() != style->width().fixed());
    } else {
        changed =
            changed || (gridItem->contentWidth() != style->width().fixed());
    }

    auto styleMargin = style->margin();
    auto stylePadding = style->padding();
    auto styleBorder = style->border();

    changed = changed || (gridItem->marginLeft() != styleMargin.left().fixed());
    changed =
        changed || (gridItem->marginRight() != styleMargin.right().fixed());

    changed =
        changed || (gridItem->paddingLeft() != stylePadding.left().fixed());
    changed =
        changed || (gridItem->paddingRight() != stylePadding.right().fixed());

    changed = changed ||
              (gridItem->borderLeft() != styleBorder.left().width().fixed());
    changed = changed ||
              (gridItem->borderRight() != styleBorder.right().width().fixed());

    if (testWidthOnly) {
        return changed;
    }

    if (style->boxSizing() == BoxSizingValue::BorderBoxBoxSizingValue) {
        changed = changed || (gridItem->height() != style->height().fixed());
    } else {
        changed =
            changed || (gridItem->contentHeight() != style->height().fixed());
    }

    changed = changed || (gridItem->marginTop() != styleMargin.top().fixed());
    changed =
        changed || (gridItem->marginBottom() != styleMargin.bottom().fixed());

    changed = changed || (gridItem->paddingTop() != stylePadding.top().fixed());
    changed =
        changed || (gridItem->paddingBottom() != stylePadding.bottom().fixed());

    changed =
        changed || (gridItem->borderTop() != styleBorder.top().width().fixed());
    changed = changed || (gridItem->borderBottom() !=
                          styleBorder.bottom().width().fixed());

    return changed;
}

bool GridFormattingContext::doesParticipateInGridFormattingContext(
    Frame* gridItem)
{
    if (gridItem->isFrameBlockBox() && gridItem->isAnonymous()) {
        FrameBlockBox* blockBox = gridItem->asFrameBlockBox();
        for (Frame* c = blockBox->firstChild(); c; c = c->next()) {
            // If the entire sequence of child text runs contains only white
            // space,
            // it is instead not rendered.
            if (!(c->isFrameText() &&
                  c->asFrameText()->text()->containsOnlyWhitespace())) {
                return true;
            }
        }

        return false;
    }

    return true;
}

FrameGridBox::FrameGridBox(Node* node, ComputedStyle* style1)
    : FrameBlockBox(node, style1)
{
    m_hasFixedStyleWidth = style()->width().isFixed();
    m_hasFixedStyleHeight = style()->height().isFixed();
}

void FrameGridBox::computeStyleFlags()
{
    FrameBlockBox::computeStyleFlags();
    m_hasFixedStyleWidth = style()->width().isFixed();
    m_hasFixedStyleHeight = style()->height().isFixed();
}

bool FrameGridBox::shouldLayout(LayoutContext& ctx,
                                LayoutWantToResolve resolveWhat,
                                FrameBox* containingBox)
{
    if (FrameBlockBox::shouldLayout(ctx, resolveWhat, containingBox)) {
        return true;
    }

    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isGridItem() && c->needsLayout()) {
            return true;
        }
    }

    return false;
}

void FrameGridBox::layoutGrid(LayoutContext& ctx)
{
    GridFormattingContext gridFormattingContext(ctx, this, contentWidth());
    gridFormattingContext.computeColumnsAndRows();
}

bool FrameGridBox::canStratchItem()
{
    if (isFlexItem()) {
        auto cb = containingBlock(this);
        if (cb->isFrameFlexibleBox()) {
            if (cb->asFrameFlexibleBox()->isColumnDirection()) {
                return cb->style()->flexWrap() ==
                           FlexWrapValue::NoWrapFlexWrapValue &&
                       cb->style()->alignItems() == StretchAlignItemValue;
            }
        }
    }
    return true;
}

// https://www.w3.org/TR/css-position-3/#inset-properties
// https://www.w3.org/TR/css-position-3/#staticpos-rect
LengthData FrameGridBox::insets()
{
    if (style()->position() == AbsolutePositionValue ||
        style()->position() == RelativePositionValue) {
        return style()->offset();
    }

    return LengthData();
}
} // namespace Starfish
