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
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSParser.h"

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
        if (m_gridTemplateColumns[col].isComputed()) {
            sumOfWidths += m_gridTemplateColumns[col].size();
        }
    }

    sumOfWidths += m_columnGap * (m_gridTemplateColumns.size() - 2);

    return sumOfWidths;
}

void GridFormattingContext::computeColumnsAndRows()
{
    for (Frame* c = m_container->firstChild(); c; c = c->next()) {
        if (c->isGridItem()) {
            m_orderedGridItems.push_back(c->asFrameBox());
        }
    }

    std::stable_sort(m_orderedGridItems.begin(), m_orderedGridItems.end(),
                     [](FrameBox* a, FrameBox* b) {
                         STARFISH_ASSERT(a != nullptr);
                         STARFISH_ASSERT(b != nullptr);
                         return a->style()->order() < b->style()->order();
                     });

    buildGridLineTemplate();
    layoutGridItems();
}

void GridFormattingContext::layoutGridItems()
{
    for (auto area : m_orderedGridArea) {
        FrameBox* gridItem = area.box();

        LayoutUnit xPosSoFar =
            m_container->borderLeft() + m_container->paddingLeft();

        for (size_t i = 1; i < area.columnStart(); i++) {
            xPosSoFar += m_gridTemplateColumns[i].size();
        }

        xPosSoFar += m_columnGap * (area.columnStart() - 1);

        if (gridItem->marginLeft()) {
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
            track.setSize(LayoutUnit(eachRowHeight), true);
        }
    }
}

void GridFormattingContext::applyFrUnitsWithColumns()
{
    LayoutUnit computedSum(0);
    LayoutUnit sumOfFrs(0);
    bool hasMinMax = false;
    bool isMinMaxComputed = false;
    for (size_t i = 1; i < m_gridTemplateColumns.size(); i++) {
        GridTrack line = m_gridTemplateColumns[i];
        // fixed, percentage, auto, fr, minmax, etc.
        if (line.isLength()) {
            computedSum += line.size();
        } else if (line.isFr()) {
            sumOfFrs += line.fr();
        } else if (line.isMinMax()) {
            computedSum += line.size();
            hasMinMax = true;
            if (line.isComputed()) {
                isMinMaxComputed = true;
            }
            if (line.max().isFr()) {
                sumOfFrs += (LayoutUnit)line.max().fr();
            }
        }
    }

    // https://www.w3.org/TR/css-grid-1/#leftover-space
    // If this value is less than 1, set it to 1 instead.
    if (sumOfFrs < 1.0f) {
        sumOfFrs = 1.0f;
    }

    LayoutUnit gapSpace = m_columnGap * (m_gridTemplateColumns.size() - 2);

    LayoutUnit remainingSpace = (m_availableWidth - gapSpace) - computedSum;
    for (size_t i = 1; i < m_gridTemplateColumns.size(); i++) {
        GridTrack& line = m_gridTemplateColumns[i];
        if (line.isFr()) {
            if (remainingSpace > 0) {
                LayoutUnit offset = (line.fr() * remainingSpace) / sumOfFrs;
                if (offset > line.size()) {
                    double value = round(offset.toDouble());
                    line.setSize(value, true);
                }
            } else {
                if (!line.isComputed()) {
                    line.setSize(0, true);
                }
            }
        } else if (line.isMinMax() && line.max().isFr()) {
            if (remainingSpace > 0) {
                LayoutUnit offset =
                    (line.max().fr() * remainingSpace) / sumOfFrs;
                if (offset > line.size()) {
                    double value = round(offset.toDouble());
                    line.setSize(value, true);
                }
            } else {
                if (!line.isComputed()) {
                    line.setSize(0, true);
                }
            }
        }
    }
}

void GridFormattingContext::applyFrUnitsWithRows()
{
    LayoutUnit maxHeight(0);
    LayoutUnit sumOfFixedHeights(0);
    GridTrack* maxGrid = nullptr;
    for (size_t i = 1; i < m_gridTemplateRows.size(); i++) {
        GridTrack line = m_gridTemplateRows[i];
        if (line.isFr()) {
            if (maxHeight < line.size()) {
                maxHeight = line.size();
                maxGrid = &m_gridTemplateRows[i];
            }
        } else {
            sumOfFixedHeights += line.size();
        }
    }

    if (!maxGrid) {
        return;
    }

    LayoutUnit availableHeight = maxGrid->size();

    if (m_container->hasFixedStyleHeight()) {
        availableHeight = m_container->style()->height().fixed();
        availableHeight -= sumOfFixedHeights;
        if (availableHeight <= 0) {
            availableHeight = maxGrid->size();
        }
    }

    for (size_t i = 1; i < m_gridTemplateRows.size(); i++) {
        GridTrack* line = &m_gridTemplateRows[i];
        if (line->isFr()) {
            LayoutUnit offset = availableHeight * line->fr() / maxGrid->fr();
            double value = round(offset.toDouble());
            offset = std::max(value, line->size().toDouble());
            line->setSize(offset, true);
        }
    }
}

GridArea* GridFormattingContext::getNamedGridArea(String* name)
{
    auto it = m_namedAreaMap.find(name->toUTF8NonGCString().data());
    if (it != m_namedAreaMap.end()) {
        return &(it->second);
    }

    return nullptr;
}

// https://drafts.csswg.org/css-grid/#grid-item-placement-algorithm
void GridFormattingContext::placeGridItemsIntoCells()
{
    for (size_t r = 0; r < GRID_MAX_TRACK + 1; r++) {
        for (size_t c = 0; c < GRID_MAX_TRACK + 1; c++) {
            m_isCellAvailable[r][c] = true;
        }
    }

    // 1. Position anything that’s not auto-positioned.
    GCVector<GridArea*> gridAreasDefinite;
    GCVector<GridArea*> gridAreasAuto;
    size_t documentOrder = 0;
    for (auto gridItem : m_orderedGridItems) {
        if (gridItem->style()->position() == AbsolutePositionValue) {
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
    {
        String* gridRowStart = box()->style()->gridRowStart();
        String* gridRowEnd = box()->style()->gridRowEnd();

        m_rowStartLine = parseGridLine(gridRowStart);
        m_rowEndLine = parseGridLine(gridRowEnd);

        if (m_rowStartLine->hasSpan()) {
            if (m_rowStartLine->hasValue()) {
                if (m_rowStartLine->value() <= 0) {
                    STARFISH_LOG_WARN("invalid value given");
                }
            } else {
                m_rowStartLine->setValue(1);
            }
        } else if (m_rowStartLine->hasCustomIdent()) {
            GridArea* namedArea =
                ctx.getNamedGridArea(m_rowStartLine->customIdent());
            if (namedArea) {
                m_rowStartLine->setValue(namedArea->rowStart());
            }
        }

        if (m_rowEndLine->hasSpan()) {
            if (m_rowEndLine->hasValue()) {
                if (m_rowEndLine->value() <= 0) {
                    STARFISH_LOG_WARN("invalid value given");
                }
            } else {
                m_rowEndLine->setValue(1);
            }
        } else if (m_rowEndLine->hasCustomIdent()) {
            GridArea* namedArea =
                ctx.getNamedGridArea(m_rowEndLine->customIdent());
            if (namedArea) {
                m_rowEndLine->setValue(namedArea->rowEnd());
            }
        }
    }

    {
        String* gridColumnStart = box()->style()->gridColumnStart();
        String* gridColumnEnd = box()->style()->gridColumnEnd();

        m_columnStartLine = parseGridLine(gridColumnStart);
        m_columnEndLine = parseGridLine(gridColumnEnd);

        if (m_columnStartLine->hasSpan()) {
            if (m_columnStartLine->hasValue()) {
                if (m_columnStartLine->value() <= 0) {
                    STARFISH_LOG_WARN("invalid value given");
                }
            } else {
                m_columnStartLine->setValue(1);
            }
        } else if (m_columnStartLine->hasCustomIdent()) {
            GridArea* namedArea =
                ctx.getNamedGridArea(m_columnStartLine->customIdent());
            if (namedArea) {
                m_columnStartLine->setValue(namedArea->columnStart());
            }
        }

        if (m_columnEndLine->hasSpan()) {
            if (m_columnEndLine->hasValue()) {
                if (m_columnEndLine->value() <= 0) {
                    STARFISH_LOG_WARN("invalid value given");
                }
            } else {
                m_columnEndLine->setValue(1);
            }
        } else if (m_columnEndLine->hasCustomIdent()) {
            GridArea* namedArea =
                ctx.getNamedGridArea(m_columnEndLine->customIdent());
            if (namedArea) {
                m_columnEndLine->setValue(namedArea->columnEnd());
            }
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
                setRowEnd(rowStart() + m_rowEndLine->value());
            } else {
                STARFISH_LOG_WARN("missing values");
            }
        } else if (m_rowEndLine->isDefinite()) {
            if (m_rowStartLine->isAuto()) {
                setRowStart(rowEnd() - 1);
            } else if (m_rowStartLine->hasSpan()) {
                setRowStart(rowEnd() - m_rowStartLine->value());
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
                setColumnEnd(columnStart() + m_columnEndLine->value());
            } else {
                STARFISH_LOG_WARN("missing values");
            }
        } else if (m_columnEndLine->isDefinite()) {
            if (m_columnStartLine->isAuto()) {
                setColumnStart(columnEnd() - 1);
            } else if (m_columnStartLine->hasSpan()) {
                setColumnStart(columnEnd() - m_columnStartLine->value());
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
    for (auto token : tokens) {
        if (token.equals("auto")) {
        } else if (token.equals("span")) {
            gridLine->setHasSpan(true);
        } else {
            String* tokenStr = String::fromUTF8(token.data(), token.length());

            if (String::validDouble(tokenStr)) {
                gridLine->setValue(String::parseDouble(tokenStr));
            } else {
                gridLine->setCustomIdent(tokenStr);
            }
        }
    }

    return gridLine;
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
    if (GRID_MAX_TRACK < gridArea->rowStart() ||
        GRID_MAX_TRACK < gridArea->rowEnd()) {
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
    if (GRID_MAX_TRACK < gridArea->columnStart() ||
        GRID_MAX_TRACK < gridArea->columnEnd()) {
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

        if (GRID_MAX_TRACK < gridArea->rowStart()) {
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
    for (size_t r = 1; r < m_gridTemplateRows.size(); r++) {
        for (size_t c = 1; c < m_gridTemplateColumns.size(); c++) {
            if (m_gridTemplateRows[r].isImplicitLine() &&
                m_gridTemplateColumns[c].isImplicitLine()) {
                firstImplicitRow = r;
                firstImplicitColumn = c;
                break;
            }
        }
    }

    size_t curRow = firstImplicitRow;
    size_t curCol = firstImplicitColumn;
    for (auto gridArea : gridAreasAuto) {
        if (gridArea->hasRowAndColumnValues()) {
            continue;
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
        gridArea->setRowEnd(gridArea->rowStart() + 1);

        if (gridArea->columnStart() == 0) {
            gridArea->setColumnStart(curCol);
        }
        if (gridArea->columnEnd() == 0) {
            gridArea->setColumnEnd(gridArea->columnStart() + 1);
        }

        placeGridArea(gridArea);
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
            if (m_isCellAvailable[r][c]) {
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
            m_isCellAvailable[r][c] = false;
        }
    }
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

    GCUnorderedMultiMap<std::string, struct Area> collector;
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
            std::string name = areas[col];
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
                    m_namedAreaMap.insert(std::make_pair(name, gridArea));
                } else {
                    return;
                }
            }
        } else {
            struct Area area = stack.back();
            GridArea gridArea(nullptr, -1, area.rowStart, area.rowEnd,
                              area.columnStart, area.columnEnd);
            m_namedAreaMap.insert(std::make_pair(name, gridArea));
        }
    }
}

void GridFormattingContext::initializeGridLineColumns(
    const GCVector<GridTrackSize>* columns)
{
    STARFISH_ASSERT(columns);

    size_t colSize = columns->size();
    for (size_t i = 0; i < colSize; i++) {
        GridTrackSize trackSize = (*columns)[i];
        GridLength gridLength = trackSize.min();
        LayoutUnit baseSize = intMaxForLayoutUnit;

        if (trackSize.isLength()) {
            if (gridLength.isLength() &&
                gridLength.length().isDefinite(m_availableWidth !=
                                               intMaxForLayoutUnit)) {
                baseSize = gridLength.length().specifiedValue(m_availableWidth,
                                                              m_container);
                if (m_availableWidth == 0 && gridLength.length().isPercent()) {
                    GridTrack line = GridTrack(0);
                    line.setAuto(true);
                    line.setFixed(false);
                    m_gridTemplateColumns.push_back(line);
                } else {
                    GridTrack line = GridTrack(baseSize);
                    m_gridTemplateColumns.push_back(line);
                }
            } else if (gridLength.isAuto()) {
                GridTrack line = GridTrack(0);
                line.setAuto(true);
                line.setFixed(false);
                m_gridTemplateColumns.push_back(line);
            }
        } else if (trackSize.isFr()) {
            GridTrack line = GridTrack(gridLength.fr(), false);
            m_gridTemplateColumns.push_back(line);
        } else if (trackSize.isMinMax()) {
            // TODO: when min or max is fr or auto, we should treat it.
            GridTrack line = GridTrack(trackSize.min(), trackSize.max());
            m_gridTemplateColumns.push_back(line);
        } else if (trackSize.isMinContent()) {
            m_gridTemplateColumns.push_back(GridTrack(GridTrack::MinContent));
        } else if (trackSize.isMaxContent()) {
            m_gridTemplateColumns.push_back(GridTrack(GridTrack::MaxContent));
        }
    }
}

void GridFormattingContext::initializeGridLineRows(
    const GCVector<GridTrackSize>* rows)
{
    STARFISH_ASSERT(rows);

    size_t rowSize = rows->size();
    for (size_t i = 0; i < rowSize; i++) {
        GridTrackSize trackSize = (*rows)[i];
        GridLength gridLength = trackSize.min();

        if (trackSize.isLength()) {
            if (gridLength.isLength() && gridLength.length().isFixed()) {
                Length length = gridLength.length();
                GridTrack line = GridTrack(length.numberData());
                m_gridTemplateRows.push_back(line);
            } else if (gridLength.isAuto()) {
                GridTrack line = GridTrack(0);
                line.setAuto(true);
                line.setFixed(false);
                m_gridTemplateRows.push_back(line);
            }
        } else if (trackSize.isFr()) {
            GridTrack line = GridTrack(gridLength.fr(), false);
            m_gridTemplateRows.push_back(line);
        } else if (trackSize.isMinMax()) {
            GridTrack line = GridTrack(trackSize.min(), trackSize.max());
            m_gridTemplateColumns.push_back(line);
        }
    }
}

void GridFormattingContext::applyMinMaxGridLineColumns()
{
    // Distribute 'auto' size;
    LayoutUnit sumOfColumns = 0;

    size_t numberOfMinMax = 0;
    size_t numberOfFr = 0;
    std::vector<GridTrack*> minMaxLines;
    bool hasFrInMinMax = false;
    for (size_t i = 1; i < m_gridTemplateColumns.size(); i++) {
        if (m_gridTemplateColumns[i].isMinMax()) {
            m_gridTemplateColumns[i].setSize(
                m_gridTemplateColumns[i].min().length().numberData(), true);
            numberOfMinMax++;
            minMaxLines.push_back(&m_gridTemplateColumns[i]);
            if (m_gridTemplateColumns[i].max().isFr()) {
                numberOfFr++;
                hasFrInMinMax = true;
            }
        } else if (m_gridTemplateColumns[i].isFr()) {
            numberOfFr++;
        }
    }

    if (numberOfMinMax > 0) {
        for (size_t i = 1; i < m_gridTemplateColumns.size(); i++) {
            sumOfColumns += m_gridTemplateColumns[i].size();
        }
    }

    sumOfColumns += (m_gridTemplateColumns.size() - 2) * m_columnGap;
    LayoutUnit availableWidth = m_availableWidth - sumOfColumns;
    LayoutUnit totalMinMaxGap = 0.0;
    if (numberOfMinMax && availableWidth > 0) {
        std::stable_sort(
            minMaxLines.begin(), minMaxLines.end(),
            [](const GridTrack* a, const GridTrack* b) {
                STARFISH_ASSERT(a != nullptr);
                STARFISH_ASSERT(b != nullptr);
                float maxA = a->max().isFr() ? a->size().toFloat()
                                             : a->max().length().numberData();
                float minA = a->min().isFr() ? a->size().toFloat()
                                             : a->min().length().numberData();
                float maxB = b->max().isFr() ? b->size().toFloat()
                                             : b->max().length().numberData();
                float minB = b->min().isFr() ? b->size().toFloat()
                                             : b->min().length().numberData();
                return (maxA - minA) < (maxB - minB);
            });

        for (size_t i = 0; i < minMaxLines.size(); i++) {
            GridTrack* line = (minMaxLines[i]);
            float maxLine = line->max().isFr()
                                ? line->size().toFloat()
                                : line->max().length().numberData();
            float minLine = line->min().isFr()
                                ? line->size().toFloat()
                                : line->min().length().numberData();

            LayoutUnit minmaxGap = maxLine - minLine;
            LayoutUnit totalMinMaxWidth = minmaxGap * numberOfMinMax;

            if (totalMinMaxWidth <= availableWidth) {
                line->setSize(line->size() + minmaxGap, true);
                availableWidth -= minmaxGap;
                totalMinMaxGap += minmaxGap;
            } else {
                LayoutUnit dividedWidth = availableWidth / numberOfMinMax;
                for (size_t j = i; j < minMaxLines.size(); j++) {
                    GridTrack* line = (minMaxLines[j]);
                    line->setSize(line->size() + dividedWidth, true);
                    totalMinMaxGap += dividedWidth;
                }
                break;
            }
            numberOfMinMax--;
        }
    }
}

void GridFormattingContext::buildGridLineTemplate()
{
    // FIXME: Combine GridTrackSize and GridTrack
    GridTrack dummyPlaceholder = GridTrack(0);
    m_gridTemplateColumns.push_back(dummyPlaceholder);
    const GCVector<GridTrackSize>* columns =
        m_container->style()->gridTemplateColumns();
    if (columns) {
        initializeGridLineColumns(columns);
    } else {
        m_gridTemplateColumns.push_back(GridTrack());
    }

    m_gridTemplateRows.push_back(dummyPlaceholder);
    const GCVector<GridTrackSize>* rows =
        m_container->style()->gridTemplateRows();
    if (rows) {
        initializeGridLineRows(rows);
    }

    parseGridTemplateAreas();
    placeGridItemsIntoCells();
    initializeColumnTrackSizes();

    {
        resolveIntrinsicTrackSizes();
        // compute minmax size for grid lines
        applyMinMaxGridLineColumns();
        // update offset using flex factor
        applyFrUnitsWithColumns();
    }

    applyImplicitTrackSizing();
    stretchAutoTracks();

    layoutGridLinesWithGridAreas();
    applyFrUnitsWithRows();

    relayoutGridLinesWithGridAreasIfNeeded();
}

static bool hasBigAreasIncludingCurrentArea(GCVector<GridArea>& list,
                                            size_t rowStart, size_t start,
                                            size_t end, size_t& bigAreaStart,
                                            size_t& bigAreaEnd,
                                            std::vector<size_t>& bigAreaIds)
{
    GridArea* target = nullptr;
    std::vector<GridArea*> areas;
    for (size_t i = 0; i < list.size(); i++) {
        GridArea preArea = list[i];

        if (preArea.rowStart() < rowStart) {
            if ((preArea.columnStart() <= start &&
                 preArea.columnEnd() >= end) &&
                (end - start < preArea.columnEnd() - preArea.columnStart())) {
                areas.push_back(&list[i]);
                bigAreaIds.push_back(preArea.index());
            }
        }
    }

    if (areas.size() == 0) {
        return false;
    }

    std::stable_sort(areas.begin(), areas.end(),
                     [](const GridArea* a, const GridArea* b) {
                         STARFISH_ASSERT(a != nullptr);
                         STARFISH_ASSERT(b != nullptr);
                         return a->columnStart() < b->columnStart();
                     });
    bigAreaStart = areas[0]->columnStart();

    std::stable_sort(areas.begin(), areas.end(),
                     [](const GridArea* a, const GridArea* b) {
                         STARFISH_ASSERT(a != nullptr);
                         STARFISH_ASSERT(b != nullptr);
                         return a->columnEnd() > b->columnEnd();
                     });
    bigAreaEnd = areas[0]->columnEnd();

    return true;
}

// WHAT: get the largest GridArea that includes the given grid area
static GridArea* getBiggestAreaWithRow(GCVector<GridArea>& list,
                                       size_t columnStart, size_t rowStart,
                                       size_t rowEnd)
{
    GridArea* target = nullptr;
    std::vector<GridArea*> areas;
    for (size_t i = 0; i < list.size(); i++) {
        GridArea preArea = list[i];

        if (preArea.columnStart() < columnStart) {
            if (preArea.rowStart() <= rowStart && preArea.rowEnd() >= rowEnd) {
                areas.push_back(&list[i]);
            }
        }
    }

    std::stable_sort(
        areas.begin(), areas.end(), [](const GridArea* a, const GridArea* b) {
            STARFISH_ASSERT(a != nullptr);
            STARFISH_ASSERT(b != nullptr);
            return a->rowEnd() - a->rowStart() < b->rowEnd() - b->rowStart();
        });

    for (size_t i = 0; i < areas.size(); i++) {
        if (rowEnd - rowStart < areas[i]->rowEnd() - areas[i]->rowStart()) {
            target = areas[i];
            break;
        }
    }

    for (size_t i = 0; i < areas.size(); i++) {
        if (rowEnd - rowStart < areas[i]->rowEnd() - areas[i]->rowStart() &&
            rowEnd == areas[i]->rowEnd()) {
            target = areas[i];
            break;
        }
    }

    return target;
}

void GridFormattingContext::updateGridTemplateColumnWidths(GridArea& gridArea)
{
    size_t columnStart = gridArea.columnStart();
    size_t columnEnd = gridArea.columnEnd();
    LayoutUnit contentWidth = gridArea.preferredWidth();

    LayoutUnit sumOfAllColumnWidths = 0;
    for (size_t i = columnStart; i < columnEnd; i++) {
        sumOfAllColumnWidths += m_gridTemplateColumns[i].size();
    }

    if (sumOfAllColumnWidths == 0) {
        LayoutUnit eachColumnWidth = contentWidth / (columnEnd - columnStart);
        for (size_t i = columnStart; i < columnEnd; i++) {
            GridTrack& gridTrack = m_gridTemplateColumns[i];
            gridTrack.setSize(eachColumnWidth, true);
        }

        return;
    }

    size_t bigAreaColumnStart;
    size_t bigAreaColumnEnd;
    std::vector<size_t> bigAreaIds;
    bool hasBigAreas = hasBigAreasIncludingCurrentArea(
        m_orderedGridArea, gridArea.rowStart(), columnStart, columnEnd,
        bigAreaColumnStart, bigAreaColumnEnd, bigAreaIds);

    GCVector<GridTrack*> gridTracksWithNonFixedWidths;
    if (hasBigAreas) {
        // find all other grid areas that contain this girdArea, and
        // also contained by the 'bigArea'
        GCVector<GridArea*> innerAreas;
        for (auto& candidate : m_orderedGridArea) {
            auto itr = std::find(bigAreaIds.begin(), bigAreaIds.end(),
                                 candidate.index());
            if (itr != bigAreaIds.end()) {
                // skip if the candidate is already in the list
                continue;
            }

            if (candidate.index() > gridArea.index()) {
                break;
            }

            if (candidate.index() == gridArea.index()) {
                continue;
            }

            if (candidate.rowStart() <= gridArea.rowStart()) {
                if (bigAreaColumnStart <= candidate.columnStart() &&
                    candidate.columnEnd() <= bigAreaColumnEnd) {
                    innerAreas.push_back(&candidate);
                }
            }
        }

        SetForGrid<size_t> set;
        for (auto inner : innerAreas) {
            for (size_t col = inner->columnStart() + 1;
                 col <= inner->columnEnd(); col++) {
                set.insert(col);
            }
        }

        for (size_t i = bigAreaColumnStart; i < bigAreaColumnEnd; i++) {
            if (gridArea.columnStart() <= i && i < gridArea.columnEnd()) {
                continue;
            }

            if (!set.find(i + 1) && !m_gridTemplateColumns[i].isFixed()) {
                gridTracksWithNonFixedWidths.push_back(
                    &m_gridTemplateColumns[i]);
            }
        }
    }

    if (contentWidth < sumOfAllColumnWidths) {
        if (hasBigAreas) {
            if (gridTracksWithNonFixedWidths.size() > 0) {
                LayoutUnit additionalWidth =
                    (sumOfAllColumnWidths - contentWidth) /
                    gridTracksWithNonFixedWidths.size();
                for (auto gridTrack : gridTracksWithNonFixedWidths) {
                    gridTrack->setSize(gridTrack->size() + additionalWidth,
                                       true);
                }

                LayoutUnit eachColumnWidth =
                    contentWidth / (columnEnd - columnStart);
                for (size_t i = columnStart; i < columnEnd; i++) {
                    GridTrack& gridTrack = m_gridTemplateColumns[i];
                    gridTrack.setSize(eachColumnWidth, true);
                }
            } else {
                GCVector<GridTrack*> noneFixed;
                LayoutUnit sumOfFixed = 0;
                for (size_t i = columnStart; i < columnEnd; i++) {
                    if (m_gridTemplateColumns[i].isFixed() &&
                        !m_gridTemplateColumns[i].isFr()) {
                        sumOfFixed += m_gridTemplateColumns[i].size();
                    } else {
                        noneFixed.push_back(&m_gridTemplateColumns[i]);
                    }
                }
                if (noneFixed.size() > 0) {
                    LayoutUnit diff = contentWidth / noneFixed.size();

                    for (auto gridTrack : noneFixed) {
                        gridTrack->setSize(diff, true);
                    }

                    noneFixed.clear();
                    sumOfFixed = 0;

                    size_t startForTarget = bigAreaColumnStart;
                    size_t endForTarget = bigAreaColumnEnd;
                    for (size_t i = startForTarget; i < endForTarget; i++) {
                        if (m_gridTemplateColumns[i].isFixed() &&
                            !m_gridTemplateColumns[i].isFr()) {
                            sumOfFixed += m_gridTemplateColumns[i].size();
                        } else {
                            noneFixed.push_back(&m_gridTemplateColumns[i]);
                        }
                    }

                    if (noneFixed.size()) {
                        diff = (sumOfAllColumnWidths - contentWidth) /
                               noneFixed.size();
                        for (auto gridTrack : noneFixed) {
                            gridTrack->setSize(gridTrack->size() + diff, true);
                        }
                    }
                }
            }
        }
    } else if (contentWidth > sumOfAllColumnWidths) {
        // the current gridarea's width is greater than the width allocated
        // in the gridtemplatecolumns. In this case, adjust the size of
        // gridtemplatecolumns so that the gridarea can fit.

        if (hasBigAreas) {
            if (gridTracksWithNonFixedWidths.size() > 0) {
                LayoutUnit diff = contentWidth - sumOfAllColumnWidths;

                LayoutUnit sumOfNonFixedWidths = 0;
                for (auto gridTrack : gridTracksWithNonFixedWidths) {
                    sumOfNonFixedWidths += gridTrack->size();
                }

                LayoutUnit remaining = 0;
                for (auto gridTrack : gridTracksWithNonFixedWidths) {
                    LayoutUnit offset =
                        diff * (gridTrack->size() / sumOfNonFixedWidths);

                    if (offset < gridTrack->size()) {
                        gridTrack->setSize(gridTrack->size() - offset, true);
                    } else {
                        remaining += offset - gridTrack->size();
                        gridTrack->setSize(0, true);
                    }
                }

                // FIXME: If remaining is not '0', we have to distribute width.

                LayoutUnit eachColumnWidth =
                    contentWidth / (columnEnd - columnStart);
                for (size_t i = columnStart; i < columnEnd; i++) {
                    GridTrack& gridTrack = m_gridTemplateColumns[i];
                    gridTrack.setSize(eachColumnWidth, true);
                }
            } else {
                GCVector<GridTrack*> noneFixed;
                LayoutUnit sumOfFixed = 0;
                for (size_t i = columnStart; i < columnEnd; i++) {
                    if (m_gridTemplateColumns[i].isFixed() &&
                        !m_gridTemplateColumns[i].isFr()) {
                        sumOfFixed += m_gridTemplateColumns[i].size();
                    } else {
                        noneFixed.push_back(&m_gridTemplateColumns[i]);
                    }
                }

                if (noneFixed.size() > 0) {
                    LayoutUnit dividedWidth =
                        (contentWidth - (sumOfAllColumnWidths - sumOfFixed)) /
                        noneFixed.size();
                    for (auto gridTrack : noneFixed) {
                        gridTrack->setSize(gridTrack->size() + dividedWidth,
                                           true);
                    }
                }
            }
        } else {
            // find any unallocated gridtemplatecolumns, if any, and assign
            // required widths
            size_t numOfUnallocatedColumns = 0;
            for (size_t i = columnStart; i < columnEnd; i++) {
                if (m_gridTemplateColumns[i].size() == 0) {
                    numOfUnallocatedColumns++;
                }
            }

            if (numOfUnallocatedColumns > 0) {
                LayoutUnit eachColumnWidth =
                    (contentWidth - sumOfAllColumnWidths) /
                    numOfUnallocatedColumns;
                for (size_t i = columnStart; i < columnEnd; i++) {
                    if (m_gridTemplateColumns[i].size() == 0) {
                        GridTrack& gridTrack = m_gridTemplateColumns[i];
                        gridTrack.setSize(eachColumnWidth, true);
                    }
                }
            } else {
                GCVector<GridTrack*> noneFixed; // fr grid line for column
                LayoutUnit sumOfFixed(0);

                for (size_t i = columnStart; i < columnEnd; i++) {
                    if (m_gridTemplateColumns[i].isLength() &&
                        m_gridTemplateColumns[i].isFixed()) {
                        sumOfFixed += m_gridTemplateColumns[i].size();
                    } else if (m_gridTemplateColumns[i].isFr()) {
                        noneFixed.push_back(&m_gridTemplateColumns[i]);
                    }
                }

                // If the grid area has multiple grid lines with fr unit,
                // each grid line should have a value of (contentWidth -
                // (gridlines' offset + fixedSum) /n-fr))
                if (noneFixed.size()) {
                    LayoutUnit dividedWidth =
                        (contentWidth - (sumOfAllColumnWidths + sumOfFixed)) /
                        noneFixed.size();
                    for (auto gridTrack : noneFixed) {
                        gridTrack->setSize(gridTrack->size() + dividedWidth,
                                           true);
                    }
                }
            }
        }
    }
}

void GridFormattingContext::updateGridTemplateRowHeights(GridArea& gridArea)
{
    FrameBox* gridItem = gridArea.box();
    LayoutUnit sumOfRowHeights = 0;
    size_t rowStart = gridArea.rowStart();
    size_t rowEnd = gridArea.rowEnd();
    LayoutUnit contentHeight =
        gridItem->height() +
        gridItem->style()->margin().top().specifiedValue(0, m_container) +
        gridItem->style()->margin().bottom().specifiedValue(0, m_container);

    gridArea.setContentHeight(gridItem->height());

    for (size_t i = rowStart; i < rowEnd; i++) {
        sumOfRowHeights += m_gridTemplateRows[i].size();
    }

    if (sumOfRowHeights == 0) {
        LayoutUnit eachRowHeight = contentHeight / (rowEnd - rowStart);
        for (size_t i = rowStart; i < rowEnd; i++) {
            GridTrack& gridTrack = m_gridTemplateRows[i];
            gridTrack.setSize(eachRowHeight, true);
        }

        return;
    }

    GridArea* biggest = getBiggestAreaWithRow(
        m_orderedGridArea, gridArea.columnStart(), rowStart, rowEnd);

    GCVector<GridTrack*> gridTracksWithNonFixedHeights;

    if (biggest) {
        GCVector<GridArea*> innerAreas;
        for (auto& candidate : m_orderedGridArea) {
            if ((candidate.index() == biggest->index()) ||
                (candidate.index() == gridArea.index())) {
                continue;
            }

            if (candidate.index() > gridArea.index()) {
                break;
            }

            if (candidate.columnStart() <= gridArea.columnStart()) {
                if (biggest->rowStart() <= candidate.rowStart() &&
                    candidate.rowEnd() <= biggest->rowEnd()) {
                    innerAreas.push_back(&candidate);
                }
            }
        }

        SetForGrid<size_t> set;
        for (auto inner : innerAreas) {
            for (size_t row = inner->rowStart() + 1; row <= inner->rowEnd();
                 row++) {
                set.insert(row);
            }
        }

        for (size_t i = biggest->rowStart(); i < biggest->rowEnd(); i++) {
            if (gridArea.rowStart() <= i && i < gridArea.rowEnd()) {
                continue;
            }

            if (!set.find(i + 1) && !m_gridTemplateRows[i].isFixed()) {
                gridTracksWithNonFixedHeights.push_back(&m_gridTemplateRows[i]);
            }
        }
    }

    if (sumOfRowHeights > contentHeight) {
        if (biggest) {
            if (gridTracksWithNonFixedHeights.size() > 0) {
                LayoutUnit additionalHeight =
                    (sumOfRowHeights - contentHeight) /
                    gridTracksWithNonFixedHeights.size();
                for (size_t i = 0; i < gridTracksWithNonFixedHeights.size();
                     i++) {
                    GridTrack* line = gridTracksWithNonFixedHeights[i];
                    line->setSize(line->size() + additionalHeight, true);
                }

                LayoutUnit eachRowHeight = contentHeight / (rowEnd - rowStart);
                for (size_t i = rowStart; i < rowEnd; i++) {
                    GridTrack& gridTrack = m_gridTemplateRows[i];
                    gridTrack.setSize(eachRowHeight, true);
                }
            } else {
                GCVector<GridTrack*> noneFixed;
                LayoutUnit sumOfFixed = 0;

                for (size_t i = rowStart; i < rowEnd; i++) {
                    if (m_gridTemplateRows[i].isFixed() &&
                        !m_gridTemplateRows[i].isFr()) {
                        sumOfFixed += m_gridTemplateRows[i].size();
                    } else {
                        noneFixed.push_back(&m_gridTemplateRows[i]);
                    }
                }

                if (noneFixed.size() > 0) {
                    LayoutUnit diff = contentHeight / noneFixed.size();
                    for (auto gridTrack : noneFixed) {
                        gridTrack->setSize(diff, true);
                    }

                    noneFixed.clear();
                    sumOfFixed = 0;

                    size_t startForTarget = biggest->rowStart();
                    size_t endForTarget = biggest->rowEnd();
                    for (size_t i = startForTarget; i < endForTarget; i++) {
                        if (m_gridTemplateRows[i].isFixed() &&
                            !m_gridTemplateRows[i].isFr()) {
                            sumOfFixed += m_gridTemplateRows[i].size();
                        } else {
                            noneFixed.push_back(&m_gridTemplateRows[i]);
                        }
                    }

                    if (noneFixed.size()) {
                        diff = (sumOfRowHeights - contentHeight) /
                               noneFixed.size();
                        for (auto gridTrack : noneFixed) {
                            gridTrack->setSize(gridTrack->size() + diff, true);
                        }
                    }
                }
            }
        }
    } else if (sumOfRowHeights < contentHeight) {
        if (biggest) {
            if (gridTracksWithNonFixedHeights.size() > 0) {
                LayoutUnit diff = contentHeight - sumOfRowHeights;

                LayoutUnit sumOfHeights = 0;
                for (auto gridTrack : gridTracksWithNonFixedHeights) {
                    sumOfHeights += gridTrack->size();
                }

                LayoutUnit remaining = 0;
                for (auto gridTrack : gridTracksWithNonFixedHeights) {
                    LayoutUnit offset =
                        diff * (gridTrack->size() / sumOfHeights);

                    if (gridTrack->size() - offset > 0) {
                        gridTrack->setSize(gridTrack->size() - offset, true);
                    } else {
                        remaining += offset - gridTrack->size();
                        gridTrack->setSize(0, true);
                    }
                }

                // FIXME: If remaining is not '0', we have to distribute height.

                LayoutUnit eachRowHeight = contentHeight / (rowEnd - rowStart);
                for (size_t i = rowStart; i < rowEnd; i++) {
                    GridTrack& gridTrack = m_gridTemplateRows[i];
                    gridTrack.setSize(eachRowHeight, true);
                }
            } else {
                GCVector<GridTrack*> noneFixed;
                LayoutUnit sumOfFixed = 0;

                for (size_t i = rowStart; i < rowEnd; i++) {
                    if (m_gridTemplateRows[i].isFixed() &&
                        !m_gridTemplateRows[i].isFr()) {
                        sumOfFixed += m_gridTemplateRows[i].size();
                    } else {
                        noneFixed.push_back(&m_gridTemplateRows[i]);
                    }
                }

                if (noneFixed.size()) {
                    LayoutUnit additionalHeight =
                        (contentHeight - (sumOfRowHeights - sumOfFixed)) /
                        noneFixed.size();
                    for (auto gridTrack : noneFixed) {
                        gridTrack->setSize(gridTrack->size() + additionalHeight,
                                           true);
                    }
                }
            }
        } else {
            size_t numOfUnallocatedRows = 0;
            for (size_t i = rowStart; i < rowEnd; i++) {
                if (m_gridTemplateRows[i].size() == 0) {
                    numOfUnallocatedRows++;
                }
            }

            if (numOfUnallocatedRows) {
                LayoutUnit eachRowHeight =
                    (contentHeight - sumOfRowHeights) / numOfUnallocatedRows;
                for (size_t i = rowStart; i < rowEnd; i++) {
                    if (!m_gridTemplateRows[i].size()) {
                        GridTrack& gridTrack = m_gridTemplateRows[i];
                        gridTrack.setSize(eachRowHeight, true);
                    }
                }
            } else {
                GCVector<GridTrack*> noneFixed;
                LayoutUnit sumOfFixed = 0;

                for (size_t i = rowStart; i < rowEnd; i++) {
                    if (m_gridTemplateRows[i].isFixed() &&
                        !m_gridTemplateRows[i].isFr()) {
                        sumOfFixed += m_gridTemplateRows[i].size();
                    } else {
                        noneFixed.push_back(&m_gridTemplateRows[i]);
                    }
                }

                if (noneFixed.size()) {
                    LayoutUnit eachRowHeight =
                        (contentHeight - sumOfFixed) / noneFixed.size();
                    for (auto gridTrack : noneFixed) {
                        gridTrack->setSize(eachRowHeight, true);
                    }
                }
            }
        }
    }
}

LayoutSize fetchFixedMarginBorderPadding(FrameGridBox* grid,
                                         ComputedStyle* style)
{
    LayoutSize result;
    // margin
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

void GridFormattingContext::initializeColumnTrackSizes()
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
        // This part relies on calculating 'width'.
        // grid lines' offset is updated.
        updateGridTemplateColumnWidths(gridArea);
    }
}

void GridFormattingContext::resolveIntrinsicTrackSizes()
{
    for (size_t i = 1; i < m_gridTemplateColumns.size(); i++) {
        GridTrack& track = m_gridTemplateColumns[i];

        if (track.isMaxContent() || track.isMinContent()) {
            resolveMinMaxContentSize(i);
        }
    }
}

void GridFormattingContext::resolveMinMaxContentSize(size_t gridTrackIndex)
{
    // TODO: consider spans
    LayoutUnit maxWidthSoFar;
    GridTrack& track = m_gridTemplateColumns[gridTrackIndex];
    for (auto area : m_orderedGridArea) {
        if ((area.columnStart() == gridTrackIndex) &&
            (gridTrackIndex < area.columnEnd())) {
            LayoutUnit width;
            if (track.isMinContent()) {
                width = area.preferredMinWidth();
            } else if (track.isMaxContent()) {
                width = area.preferredWidth();
            }

            maxWidthSoFar = std::max(maxWidthSoFar, width);
        }
    }
    track.setSize(maxWidthSoFar, true);
}

void GridFormattingContext::stretchAutoTracks()
{
    if (m_availableWidth <= 0) {
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
        (m_availableWidth - gapSpace) - sumOfAllNonAutoWidths;
    LayoutUnit additionalAutoTrackSpace = remainingSpace / numOfAutoTracks;
    for (size_t i = 1; i < m_gridTemplateColumns.size(); i++) {
        GridTrack& track = m_gridTemplateColumns[i];
        if (track.isAuto() && !track.isMinContent() && !track.isMaxContent()) {
            track.setSize(track.size() + additionalAutoTrackSpace, true);
        }
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

bool GridFormattingContext::needsGridItemLayout(FrameBox* gridItem,
                                                ComputedStyle* style,
                                                bool testWidthOnly)
{
    bool changed = false;

    if (testWidthOnly) {
        if (style->boxSizing() == BoxSizingValue::BorderBoxBoxSizingValue) {
            changed = changed || (gridItem->width() != style->width().fixed());
        } else {
            changed =
                changed || (gridItem->contentWidth() != style->width().fixed());
        }

        auto styleMargin = style->margin();
        auto stylePadding = style->padding();
        auto styleBorder = style->border();

        changed =
            changed || (gridItem->marginLeft() != styleMargin.left().fixed());
        changed =
            changed || (gridItem->marginRight() != styleMargin.right().fixed());

        changed =
            changed || (gridItem->paddingLeft() != stylePadding.left().fixed());
        changed = changed ||
                  (gridItem->paddingRight() != stylePadding.right().fixed());

        changed = changed || (gridItem->borderLeft() !=
                              styleBorder.left().width().fixed());
        changed = changed || (gridItem->borderRight() !=
                              styleBorder.right().width().fixed());
    } else {
        if (style->boxSizing() == BoxSizingValue::BorderBoxBoxSizingValue) {
            changed = changed || (gridItem->width() != style->width().fixed());
            changed =
                changed || (gridItem->height() != style->height().fixed());
        } else {
            changed =
                changed || (gridItem->contentWidth() != style->width().fixed());
            changed = changed ||
                      (gridItem->contentHeight() != style->height().fixed());
        }

        auto styleMargin = style->margin();
        auto stylePadding = style->padding();
        auto styleBorder = style->border();

        changed =
            changed || (gridItem->marginLeft() != styleMargin.left().fixed());
        changed =
            changed || (gridItem->marginTop() != styleMargin.top().fixed());
        changed =
            changed || (gridItem->marginRight() != styleMargin.right().fixed());
        changed = changed ||
                  (gridItem->marginBottom() != styleMargin.bottom().fixed());

        changed =
            changed || (gridItem->paddingLeft() != stylePadding.left().fixed());
        changed =
            changed || (gridItem->paddingTop() != stylePadding.top().fixed());
        changed = changed ||
                  (gridItem->paddingRight() != stylePadding.right().fixed());
        changed = changed ||
                  (gridItem->paddingBottom() != stylePadding.bottom().fixed());

        changed = changed || (gridItem->borderLeft() !=
                              styleBorder.left().width().fixed());
        changed = changed ||
                  (gridItem->borderTop() != styleBorder.top().width().fixed());
        changed = changed || (gridItem->borderRight() !=
                              styleBorder.right().width().fixed());
        changed = changed || (gridItem->borderBottom() !=
                              styleBorder.bottom().width().fixed());
    }

    return changed;
}

void GridFormattingContext::layoutGridLinesWithGridAreas()
{
    for (auto& area : m_orderedGridArea) {
        FrameBox* gridItem = area.box();
        GridLayoutScope scope(gridItem);
        ComputedStyle* style = gridItem->style();

        LayoutUnit width;
        LayoutUnit styleWidth;
        LayoutUnit contentWidth;
        bool isFixed = true;

        LayoutSize mbp = fetchFixedMarginBorderPadding(m_container, style);

        if (style->width().isFixed()) {
            width = style->width().fixed() + mbp.width();
            contentWidth = width;
            isFixed = true;
        } else {
            auto cache = m_layoutContext.testGridItemPreferredWidthCache(
                gridItem, m_availableWidth);
            contentWidth = cache.getValue() + mbp.width();
            isFixed = false;
        }

        if (!isFixed) {
            width = 0;
            for (size_t i = area.columnStart(); i < area.columnEnd(); i++) {
                width += m_gridTemplateColumns[i].size();
            }

            // Add the gap size of columns.
            width +=
                ((area.columnEnd() - area.columnStart() - 1) * m_columnGap);
        }

        LayoutUnit widthWillBe = width;
        style->setMarginLeft(
            Length(Length::Fixed,
                   style->margin().left().specifiedValue(width, m_container)));
        widthWillBe -= style->margin().left().fixed();
        style->setMarginRight(
            Length(Length::Fixed,
                   style->margin().right().specifiedValue(width, m_container)));
        widthWillBe -= style->margin().right().fixed();

        style->setPaddingLeft(
            Length(Length::Fixed,
                   style->padding().left().specifiedValue(width, m_container)));
        widthWillBe -= style->padding().left().fixed();
        style->setPaddingRight(Length(
            Length::Fixed,
            style->padding().right().specifiedValue(width, m_container)));
        widthWillBe -= style->padding().right().fixed();

        style->setBorderLeftWidth(Length(
            Length::Fixed,
            style->border().left().width().specifiedValue(width, m_container)));
        widthWillBe -= style->border().left().width().fixed();
        style->setBorderRightWidth(Length(
            Length::Fixed, style->border().right().width().specifiedValue(
                               width, m_container)));
        widthWillBe -= style->border().right().width().fixed();

        width = widthWillBe;
        if (width < 0) {
            width = 0;
        }
        style->setWidth(Length(Length::Fixed, width));

        style->setMarginTop(
            Length(Length::Fixed,
                   style->margin().top().specifiedValue(width, m_container)));
        style->setMarginBottom(Length(
            Length::Fixed,
            style->margin().bottom().specifiedValue(width, m_container)));

        style->setPaddingTop(
            Length(Length::Fixed,
                   style->padding().top().specifiedValue(width, m_container)));
        style->setPaddingBottom(Length(
            Length::Fixed,
            style->padding().bottom().specifiedValue(width, m_container)));

        style->setBorderTopWidth(Length(
            Length::Fixed,
            style->border().top().width().specifiedValue(width, m_container)));
        style->setBorderBottomWidth(Length(
            Length::Fixed, style->border().bottom().width().specifiedValue(
                               width, m_container)));

        if (needsGridItemLayout(gridItem, style, true)) {
            gridItem->markNeedsLayout();
        }
        gridItem->layout(m_layoutContext,
                         Frame::LayoutWantToResolve::ResolveAll);

        updateGridTemplateRowHeights(area);
    }
}

void GridFormattingContext::relayoutGridLinesWithGridAreasIfNeeded()
{
    for (auto area : m_orderedGridArea) {
        FrameBox* gridItem = area.box();
        GridLayoutScope scope(gridItem);
        ComputedStyle* style = gridItem->style();

        LayoutUnit width;
        LayoutUnit styleWidth;
        LayoutUnit contentWidth;
        bool isFixed = true;

        LayoutSize mbp = fetchFixedMarginBorderPadding(m_container, style);

        if (style->width().isFixed()) {
            width = style->width().fixed() + mbp.width();
            contentWidth = width;
            isFixed = true;
        } else {
            auto cache = m_layoutContext.testGridItemPreferredWidthCache(
                gridItem, m_availableWidth);
            contentWidth = cache.getValue() + mbp.width();
            isFixed = false;
        }

        if (!isFixed) {
            width = 0;
            for (size_t i = area.columnStart(); i <= area.columnEnd() - 1;
                 i++) {
                width += m_gridTemplateColumns[i].size();
            }

            // Add the gap size of columns.
            width +=
                ((area.columnEnd() - area.columnStart() - 1) * m_columnGap);
        }

        LayoutUnit widthWillBe = width;
        style->setMarginLeft(
            Length(Length::Fixed,
                   style->margin().left().specifiedValue(width, m_container)));
        widthWillBe -= style->margin().left().fixed();
        style->setMarginRight(
            Length(Length::Fixed,
                   style->margin().right().specifiedValue(width, m_container)));
        widthWillBe -= style->margin().right().fixed();

        style->setPaddingLeft(
            Length(Length::Fixed,
                   style->padding().left().specifiedValue(width, m_container)));
        widthWillBe -= style->padding().left().fixed();
        style->setPaddingRight(Length(
            Length::Fixed,
            style->padding().right().specifiedValue(width, m_container)));
        widthWillBe -= style->padding().right().fixed();

        style->setBorderLeftWidth(Length(
            Length::Fixed,
            style->border().left().width().specifiedValue(width, m_container)));
        widthWillBe -= style->border().left().width().fixed();
        style->setBorderRightWidth(Length(
            Length::Fixed, style->border().right().width().specifiedValue(
                               width, m_container)));
        widthWillBe -= style->border().right().width().fixed();

        width = widthWillBe;
        if (width < 0) {
            width = 0;
        }
        style->setWidth(Length(Length::Fixed, width));

        if (!style->height().isFixed()) {
            LayoutUnit height;

            for (size_t i = area.rowStart(); i < area.rowEnd(); i++) {
                height += m_gridTemplateRows[i].size();
            }

            height += ((area.rowEnd() - area.rowStart() - 1) * m_rowGap);

            LayoutUnit heightWillBe = height;
            style->setMarginTop(Length(
                Length::Fixed,
                style->margin().top().specifiedValue(width, m_container)));
            heightWillBe -= style->margin().top().fixed();
            style->setMarginBottom(Length(
                Length::Fixed,
                style->margin().bottom().specifiedValue(width, m_container)));
            heightWillBe -= style->margin().bottom().fixed();

            style->setPaddingTop(Length(
                Length::Fixed,
                style->padding().top().specifiedValue(width, m_container)));
            heightWillBe -= style->padding().top().fixed();
            style->setPaddingBottom(Length(
                Length::Fixed,
                style->padding().bottom().specifiedValue(width, m_container)));
            heightWillBe -= style->padding().bottom().fixed();

            style->setBorderTopWidth(Length(
                Length::Fixed, style->border().top().width().specifiedValue(
                                   width, m_container)));
            heightWillBe -= style->border().top().width().fixed();
            style->setBorderBottomWidth(Length(
                Length::Fixed, style->border().bottom().width().specifiedValue(
                                   width, m_container)));
            heightWillBe -= style->border().bottom().width().fixed();

            height = heightWillBe;
            if (height < 0) {
                height = 0;
            }

            style->setHeight(Length(Length::Fixed, height));
        } else {
            style->setMarginTop(Length(
                Length::Fixed,
                style->margin().top().specifiedValue(width, m_container)));
            style->setMarginBottom(Length(
                Length::Fixed,
                style->margin().bottom().specifiedValue(width, m_container)));

            style->setPaddingTop(Length(
                Length::Fixed,
                style->padding().top().specifiedValue(width, m_container)));
            style->setPaddingBottom(Length(
                Length::Fixed,
                style->padding().bottom().specifiedValue(width, m_container)));

            style->setBorderTopWidth(Length(
                Length::Fixed, style->border().top().width().specifiedValue(
                                   width, m_container)));
            style->setBorderBottomWidth(Length(
                Length::Fixed, style->border().bottom().width().specifiedValue(
                                   width, m_container)));
        }

        if (needsGridItemLayout(gridItem, style, false)) {
            gridItem->markNeedsLayout();
            gridItem->layout(m_layoutContext,
                             Frame::LayoutWantToResolve::ResolveAll);
        }

        updateGridTemplateRowHeights(area);
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

    Frame* child = firstChild();
    while (child) {
        if (child->isGridItem() && child->needsLayout()) {
            return true;
        }
        child = child->next();
    }

    return false;
}

void FrameGridBox::layoutGrid(LayoutContext& ctx)
{
    GridFormattingContext gridFormattingContext(ctx, this, contentWidth());
    gridFormattingContext.computeColumnsAndRows();
}
}
