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
    LayoutUnit widthOfSum(0);
    for (size_t col = 0; col < m_gridTemplateColumns.size(); col++) {
        if (m_gridTemplateColumns[col].isComputed()) {
            widthOfSum += m_gridTemplateColumns[col].offset();
        }
    }

    for (size_t i = 1; i < m_gridTemplateColumns.size() - 1; i++) {
        widthOfSum += m_columnGap;
    }

    return widthOfSum;
}

bool GridFormattingContext::existColumnTemplate()
{
    const GCVector<GridTrackSize>* columns =
        m_container->style()->gridTemplateColumns();

    if (columns) {
        return true;
    }

    return false;
}

bool GridFormattingContext::existRowTemplate()
{
    const GCVector<GridTrackSize>* rows =
        m_container->style()->gridTemplateRows();

    if (rows) {
        return true;
    }

    return false;
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
        LayoutUnit offsetX, offsetY;

        LayoutUnit heightOfSum(0);
        for (size_t i = 1; i < area.rowStart(); i++) {
            heightOfSum += m_gridTemplateRows[i].offset();
        }

        for (size_t i = 1; i < area.rowStart(); i++) {
            heightOfSum += m_rowGap;
        }

        LayoutUnit widthOfSum(0);
        for (size_t i = 1; i < area.columnStart(); i++) {
            widthOfSum += m_gridTemplateColumns[i].offset();
        }

        for (size_t i = 1; i < area.columnStart(); i++) {
            widthOfSum += m_columnGap;
        }
        offsetY =
            heightOfSum + m_container->borderTop() + m_container->paddingTop();

        if (gridItem->marginTop()) {
            offsetY += gridItem->marginTop();
        }

        offsetX =
            widthOfSum + m_container->borderLeft() + m_container->paddingLeft();

        if (gridItem->marginLeft()) {
            if (gridItem->width() > gridItem->mbpWidth()) {
                offsetX += gridItem->marginLeft();
            }
        }

        gridItem->setX(offsetX);
        gridItem->setY(offsetY);
    }

    LayoutUnit heightOfSum(0);
    for (size_t i = 1; i < m_gridTemplateRows.size(); i++) {
        heightOfSum += m_gridTemplateRows[i].offset();
    }

    for (size_t i = 1; i < m_gridTemplateRows.size() - 1; i++) {
        heightOfSum += m_rowGap;
    }

    m_container->computeContentHeight(m_layoutContext, heightOfSum);
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
            track.setOffset(LayoutUnit(eachRowHeight), true);
        }
    }
}

void GridFormattingContext::applyFrUnitsWithColumns()
{
    LayoutUnit computedSum(0);
    LayoutUnit frOfSum(0);
    bool hasMinMax = false;
    bool isMinMaxComputed = false;
    for (size_t i = 1; i < m_gridTemplateColumns.size(); i++) {
        GridTrack line = m_gridTemplateColumns[i];
        // fixed, percentage, auto, fr, minmax, etc.
        if (line.isLength()) {
            computedSum += line.offset();
        } else if (line.isFr()) {
            frOfSum += line.fr();
        } else if (line.isMinMax()) {
            computedSum += line.offset();
            hasMinMax = true;
            if (line.isComputed()) {
                isMinMaxComputed = true;
            }
            if (line.max().isFr()) {
                frOfSum += (LayoutUnit)line.max().fr();
            }
        }
    }

    // https://www.w3.org/TR/css-grid-1/#leftover-space
    // If this value is less than 1, set it to 1 instead.
    if (frOfSum < 1.0f) {
        frOfSum = 1.0f;
    }

    LayoutUnit gapSpace = m_columnGap * (m_gridTemplateColumns.size() - 2);

    LayoutUnit remainingSpace = (m_availableWidth - gapSpace) - computedSum;
    for (size_t i = 0; i < m_gridTemplateColumns.size(); i++) {
        GridTrack& line = m_gridTemplateColumns[i];
        if (line.isFr()) {
            if (remainingSpace > 0) {
                LayoutUnit offset = (line.fr() * remainingSpace) / frOfSum;
                if (offset > line.offset()) {
                    double value = round(offset.toDouble());
                    line.setOffset(value, true);
                }
            } else {
                if (!line.isComputed()) {
                    line.setOffset(0, true);
                }
            }
        } else if (line.isMinMax() && line.max().isFr()) {
            if (remainingSpace > 0) {
                LayoutUnit offset =
                    (line.max().fr() * remainingSpace) / frOfSum;
                if (offset > line.offset()) {
                    double value = round(offset.toDouble());
                    line.setOffset(value, true);
                }
            } else {
                if (!line.isComputed()) {
                    line.setOffset(0, true);
                }
            }
        }
    }
}

void GridFormattingContext::applyFrUnitsWithRows()
{
    LayoutUnit maxHeight(0);
    LayoutUnit sumOfFixedHeight(0);
    GridTrack* maxGrid = nullptr;
    for (size_t i = 1; i < m_gridTemplateRows.size(); i++) {
        GridTrack line = m_gridTemplateRows[i];
        if (line.isFr()) {
            if (maxHeight < line.offset()) {
                maxHeight = line.offset();
                maxGrid = &m_gridTemplateRows[i];
            }
        } else {
            sumOfFixedHeight += line.offset();
        }
    }

    if (!maxGrid) {
        return;
    }

    LayoutUnit availableHeight = maxGrid->offset();

    if (m_container->hasFixedStyleHeight()) {
        availableHeight = m_container->style()->height().fixed();
        availableHeight -= sumOfFixedHeight;
        if (availableHeight <= 0) {
            availableHeight = maxGrid->offset();
        }
    }

    for (size_t i = 1; i < m_gridTemplateRows.size(); i++) {
        GridTrack* line = &m_gridTemplateRows[i];
        if (line->isFr()) {
            LayoutUnit offset = availableHeight * line->fr() / maxGrid->fr();
            double value = round(offset.toDouble());
            offset = std::max(value, line->offset().toDouble());
            line->setOffset(offset, true);
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

static void adaptStartAndEndValueForRow(GridFormattingContext& ctx,
                                        FrameBox* gridItem, size_t numberOfRows,
                                        size_t& start, size_t& end)
{
    if (start > 0 && end > 0) {
        if (start > end) {
            size_t temp = start;
            start = end;
            end = temp;
        } else if (start == end) {
            end = start + 1;
        }
    } else if (start > 0 && !end) {
        end = start + 1;
    } else if (end > 0 && !start) {
        start = end;
        end = start + 1;
    }

    if (start > GRID_MAX_TRACK - 1 || end > GRID_MAX_TRACK) {
        start = end = 0;
    }

    if (start > numberOfRows - 1 || end > numberOfRows) {
        GCVector<GridTrack>& gridLineRows = ctx.gridTemplateRows();
        bool existFr = false;
        for (size_t i = 0; i < gridLineRows.size(); i++) {
            if (gridLineRows[i].isFr() && !gridLineRows[i].isNewLine()) {
                existFr = true;
                break;
            }
        }
        size_t numberOfNeededlines = end - numberOfRows;

        for (size_t i = 0; i < numberOfNeededlines; i++) {
            GridTrack line = GridTrack(0);
            line.setComputed(false);
            line.setNewLine(true);
            line.setFixed(false);
            gridLineRows.push_back(line);
        }
    }
}

static void adaptStartAndEndValueForColumn(GridFormattingContext& ctx,
                                           FrameBox* gridItem,
                                           size_t numberOfColumns,
                                           size_t& start, size_t& end)
{
    if (start > 0 && end > 0) {
        if (start > end) {
            size_t temp = start;
            start = end;
            end = temp;
        } else if (start == end) {
            end = start + 1;
        }
    } else if (start > 0 && !end) {
        end = start + 1;
    } else if (end > 0 && !start) {
        start = end - 1;
    }

    if (start > GRID_MAX_TRACK - 1 || end > GRID_MAX_TRACK) {
        start = end = 0;
    }

    if (start > numberOfColumns - 1 || end > numberOfColumns) {
        GCVector<GridTrack>& gridLineColumns = ctx.gridTemplateColumns();
        bool existFr = false;
        for (size_t i = 0; i < gridLineColumns.size(); i++) {
            if (gridLineColumns[i].isFr() && !gridLineColumns[i].isNewLine()) {
                existFr = true;
                break;
            }
        }

        if (!ctx.existColumnTemplate()) {
            existFr = false;
        }

        size_t numberOfNeededlines = end - numberOfColumns;
        for (size_t i = 0; i < numberOfNeededlines; i++) {
            GridTrack line = GridTrack(0);

            if (existFr) {
                line = GridTrack(0);
                line.setComputed(false);
                line.setFixed(false);
            } else {
                line.setAuto(true);
                line.setFixed(false);
            }

            line.setNewLine(true);

            gridLineColumns.push_back(line);
        }
    }
}

bool GridFormattingContext::fixGridAreaWithDefine(GridArea* area, size_t row)
{
    if (!area) {
        return false;
    }

    if (m_gridTemplateColumns.size() >= GRID_MAX_TRACK || row >= GRID_MAX_TRACK) {
        return true;
    }

    size_t rowStart = area->rowStart();
    size_t rowEnd = area->rowEnd();

    if (rowStart != row + 1) {
        return false;
    }

    size_t columnStart = area->columnStart();
    size_t columnEnd = area->columnEnd();

    bool available = false;
    if (!columnStart && !columnEnd) {
        size_t columnLength = m_gridTemplateColumns.size() - 1;
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

    area->setRowStart(rowStart);
    area->setRowEnd(rowEnd);
    area->setColumnStart(columnStart);
    area->setColumnEnd(columnEnd);

    m_orderedGridArea.push_back(*area);

    return true;
}

bool GridFormattingContext::fixGridAreaWithUndefine(GridArea** preArea,
                                                    GridArea* area, size_t row)
{
    if (!area) {
        return false;
    }

    ComputedStyle* style = area->box()->style();
    size_t columnLength = m_gridTemplateColumns.size();
    size_t position = 0;
    bool available = true;

    size_t start = area->columnStart();
    size_t end = area->columnEnd();

    if (*preArea) {
        if ((*preArea)->index() < area->index() &&
            (*preArea)->columnStart() > area->columnStart()) {
            if (!start && !end) {
                start = (*preArea)->columnEnd();
                end = start + 1;

                if (end > m_gridTemplateColumns.size()) {
                    return false;
                }
            } else {
                return false;
            }
        }
    }

    if (columnLength >= GRID_MAX_TRACK || row >= GRID_MAX_TRACK) {
        return true;
    }

    if (!start && !end) {
        for (size_t i = 0; i < columnLength - 1; i++) {
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
            position = start;
        }
    }

    if (row + 1 > m_gridTemplateRows.size() - 1) {
        GridTrack line = GridTrack(0);
        line.setComputed(false);

        if (m_gridTemplateRows.size() >= GRID_MAX_TRACK) {
            return true;
        }

        m_gridTemplateRows.push_back(line);
    }

    if (!position || !available) {
        return false;
    }

    area->setRowStart(row + 1);
    area->setRowEnd(area->rowStart() + 1);

    area->setColumnStart(position);
    if (!start && !end) {
        area->setColumnEnd(area->columnStart() + 1);
    } else {
        area->setColumnEnd(end);
    }

    *preArea = area;
    m_orderedGridArea.push_back(*area);

    return true;
}

size_t GridFormattingContext::convertToRealLine(String* str, size_t pos,
                                                ConvertType type)
{
    // TODO : Implement that other factors should be converted.
    auto raw = str->toUTF8NonGCString();
    CSSTokenVector tokens;
    CSSStyleDeclaration::tokenizeCSSValue(tokens, raw.data(), raw.length());

    if (tokens[0].equals("auto")) {
        return 0;
    }

    if (tokens.size() >= 2 && tokens[0].equals("span")) {
        auto ss = tokens[1].trim();
        CSSPropertyParser parser((char*)ss.data(), ss.length());
        parser.consumeWhitespaces();

        bool hasPoint = false;
        if (parser.consumeNumber(&hasPoint)) {
            size_t n = parser.parsedNumber();
            if (type < ROWEND) {
                n = pos - n;
                if (n <= 0) {
                    n = 1;
                }
            } else {
                n = pos + n;
            }

            return n;
        } else if (parser.consumeString(CSSPropertyParser::AllowWithoutUnit)) {
            return 0;
        }
    } else {
        auto ss = tokens[0].trim();
        CSSPropertyParser parser((char*)ss.data(), ss.length());
        parser.consumeWhitespaces();

        bool hasPoint = false;
        if (parser.consumeNumber(&hasPoint)) {
            if (tokens.size() >= 2) {
                for (size_t i = 1; i < tokens.size(); i++) {
                    auto str = tokens[i].trim();
                    CSSPropertyParser sub((char*)str.data(), str.length());
                    sub.consumeWhitespaces();
                    if (!sub.consumeString(
                            CSSPropertyParser::AllowWithoutUnit)) {
                        return 0;
                    }
                }
                return 0;
            } else {
                return parser.parsedNumber();
            }
        } else if (parser.consumeString(CSSPropertyParser::AllowWithoutUnit)) {
            auto name = parser.parsedString();
            GridArea* area = getNamedGridArea(
                String::createASCIIString(name.data(), name.size()));
            if (area) {
                if (type == COLUMNSTART) {
                    return area->columnStart();
                } else if (type == COLUMNEND) {
                    return area->columnEnd();
                } else if (type == ROWSTART) {
                    return area->rowStart();
                } else if (type == ROWEND) {
                    return area->rowEnd();
                }
            } else {
                return 0;
            }
        }
    }

    return 0;
}

void GridFormattingContext::convertToStartEndForRow(ComputedStyle* style,
                                                    size_t& start, size_t& end)
{
    String* rowStart = style->gridRowStart();
    String* rowEnd = style->gridRowEnd();

    bool spanStart = false;
    bool spanEnd = false;
    {
        auto raw = rowStart->toUTF8NonGCString();
        CSSTokenVector tokens;
        CSSStyleDeclaration::tokenizeCSSValue(tokens, raw.data(), raw.length());

        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].equals("span")) {
                spanStart = true;
                break;
            }
        }
    }

    {
        auto raw = rowEnd->toUTF8NonGCString();
        CSSTokenVector tokens;
        CSSStyleDeclaration::tokenizeCSSValue(tokens, raw.data(), raw.length());

        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].equals("span")) {
                spanEnd = true;
                break;
            }
        }
    }

    if (spanStart && spanEnd) {
        start = end = 0;
        return;
    }

    if (!spanStart && spanEnd) {
        size_t s = convertToRealLine(rowStart, 0, ROWSTART);
        size_t e = convertToRealLine(rowEnd, s, ROWEND);
        start = s;
        end = e;
    } else if (spanStart && !spanEnd) {
        size_t e = convertToRealLine(rowEnd, 0, ROWEND);
        size_t s = convertToRealLine(rowStart, e, ROWSTART);
        start = s;
        end = e;
    } else {
        size_t s = convertToRealLine(rowStart, 0, ROWSTART);
        size_t e = convertToRealLine(rowEnd, 0, ROWEND);
        start = s;
        end = e;
    }
}

void GridFormattingContext::convertToStartEndForColumn(ComputedStyle* style,
                                                       size_t& start,
                                                       size_t& end)
{
    String* columnStart = style->gridColumnStart();
    String* columnEnd = style->gridColumnEnd();

    bool spanStart = false;
    bool spanEnd = false;
    {
        auto raw = columnStart->toUTF8NonGCString();
        CSSTokenVector tokens;
        CSSStyleDeclaration::tokenizeCSSValue(tokens, raw.data(), raw.length());

        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].equals("span")) {
                spanStart = true;
                break;
            }
        }
    }

    {
        auto raw = columnEnd->toUTF8NonGCString();
        CSSTokenVector tokens;
        CSSStyleDeclaration::tokenizeCSSValue(tokens, raw.data(), raw.length());

        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].equals("span")) {
                spanEnd = true;
                break;
            }
        }
    }

    if (spanStart && spanEnd) {
        start = end = 0;
        return;
    }

    if (!spanStart && spanEnd) {
        size_t s = convertToRealLine(columnStart, 0, COLUMNSTART);
        size_t e = convertToRealLine(columnEnd, s, COLUMNEND);
        start = s;
        end = e;
    } else if (spanStart && !spanEnd) {
        size_t e = convertToRealLine(columnEnd, 0, COLUMNEND);
        size_t s = convertToRealLine(columnStart, e, COLUMNSTART);
        start = s;
        end = e;
    } else {
        size_t s = convertToRealLine(columnStart, 0, COLUMNSTART);
        size_t e = convertToRealLine(columnEnd, 0, COLUMNEND);
        start = s;
        end = e;
    }
}

// https://drafts.csswg.org/css-grid/#grid-item-placement-algorithm
void GridFormattingContext::placeGridItemsIntoCells()
{
    for (size_t r = 0; r < GRID_MAX_TRACK; r++) {
        for (size_t c = 0; c < GRID_MAX_TRACK; c++) {
            m_areaChecker[r][c] = true;
        }
    }

    // 1. Position anything that’s not auto-positioned.
    GCVector<GridArea*> gridAreasDefinite;
    GCVector<GridArea*> gridAreasAuto;
    size_t documentOrder = 0;
    for (auto gridItem : m_orderedGridItems) {
        ComputedStyle* style = gridItem->style();

        if (style->position() == AbsolutePositionValue) {
            continue;
        }

        GridArea* gridArea = new GridArea(gridItem, documentOrder);
        parseGridRowAndColumnValues(gridArea);
        resolveDefinitePositionValues(gridArea);
        bool rowOk = expandGridLineRows(gridArea);
        bool colOk = expandGridLineColumns(gridArea);

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

    // 3. Determine the columns in the implicit grid.

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

void GridFormattingContext::parseGridRowAndColumnValues(GridArea* gridArea)
{
    {
        String* gridRowStart = gridArea->box()->style()->gridRowStart();
        String* gridRowEnd = gridArea->box()->style()->gridRowEnd();

        GridLineValue* gridRowStartValue = parseGridLineValue(gridRowStart);
        GridLineValue* gridRowEndValue = parseGridLineValue(gridRowEnd);

        gridArea->setGridRowStart(gridRowStartValue);
        gridArea->setGridRowEnd(gridRowEndValue);

        if (gridRowStartValue->hasSpan()) {
            if (gridRowStartValue->hasValue()) {
                if (gridRowStartValue->value() <= 0) {
                    STARFISH_LOG_WARN("invalid value given");
                }
            } else {
                gridRowStartValue->setValue(1);
            }
        } else if (gridRowStartValue->hasCustomIdent()) {
            GridArea* namedArea =
                getNamedGridArea(gridRowStartValue->customIdent());
            if (namedArea) {
                gridRowStartValue->setValue(namedArea->rowStart());
            }
        }

        if (gridRowEndValue->hasSpan()) {
            if (gridRowEndValue->hasValue()) {
                if (gridRowEndValue->value() <= 0) {
                    STARFISH_LOG_WARN("invalid value given");
                }
            } else {
                gridRowEndValue->setValue(1);
            }
        } else if (gridRowEndValue->hasCustomIdent()) {
            GridArea* namedArea =
                getNamedGridArea(gridRowEndValue->customIdent());
            if (namedArea) {
                gridRowEndValue->setValue(namedArea->rowEnd());
            }
        }
    }

    {
        String* gridColumnStart = gridArea->box()->style()->gridColumnStart();
        String* gridColumnEnd = gridArea->box()->style()->gridColumnEnd();

        GridLineValue* gridColumnStartValue =
            parseGridLineValue(gridColumnStart);
        GridLineValue* gridColumnEndValue = parseGridLineValue(gridColumnEnd);

        gridArea->setGridColumnStart(gridColumnStartValue);
        gridArea->setGridColumnEnd(gridColumnEndValue);

        if (gridColumnStartValue->hasSpan()) {
            if (gridColumnStartValue->hasValue()) {
                if (gridColumnStartValue->value() <= 0) {
                    STARFISH_LOG_WARN("invalid value given");
                }
            } else {
                gridColumnStartValue->setValue(1);
            }
        } else if (gridColumnStartValue->hasCustomIdent()) {
            GridArea* namedArea =
                getNamedGridArea(gridColumnStartValue->customIdent());
            if (namedArea) {
                gridColumnStartValue->setValue(namedArea->columnStart());
            }
        }

        if (gridColumnEndValue->hasSpan()) {
            if (gridColumnEndValue->hasValue()) {
                if (gridColumnEndValue->value() <= 0) {
                    STARFISH_LOG_WARN("invalid value given");
                }
            } else {
                gridColumnEndValue->setValue(1);
            }
        } else if (gridColumnEndValue->hasCustomIdent()) {
            GridArea* namedArea =
                getNamedGridArea(gridColumnEndValue->customIdent());
            if (namedArea) {
                gridColumnEndValue->setValue(namedArea->columnEnd());
            }
        }
    }
}

void GridFormattingContext::resolveDefinitePositionValues(GridArea* gridArea)
{
    {
        GridLineValue* gridRowStart = gridArea->gridRowStart();
        GridLineValue* gridRowEnd = gridArea->gridRowEnd();

        if (gridRowStart->isDefinite() && gridRowEnd->isDefinite()) {
            gridArea->setRowStart(gridRowStart->value());
            gridArea->setRowEnd(gridRowEnd->value());
            if (gridArea->rowStart() >= gridArea->rowEnd()) {
                gridArea->setRowEnd(gridArea->rowStart() + 1);
            }
        } else if (gridRowStart->isDefinite()) {
            gridArea->setRowStart(gridRowStart->value());

            if (gridRowEnd->isAuto()) {
                gridArea->setRowEnd(gridArea->rowStart() + 1);
            } else if (gridRowEnd->hasSpan()) {
                gridArea->setRowEnd(gridArea->rowStart() + gridRowEnd->value());
            } else {
                STARFISH_LOG_WARN("missing values");
            }
        } else if (gridRowEnd->isDefinite()) {
            gridArea->setRowEnd(gridRowEnd->value());

            if (gridRowStart->isAuto()) {
                gridArea->setRowStart(gridArea->rowEnd() - 1);
            } else if (gridRowStart->hasSpan()) {
                gridArea->setRowStart(gridArea->rowEnd() -
                                      gridRowStart->value());
            } else {
                STARFISH_LOG_WARN("missing values");
            }
        }
    }

    {
        GridLineValue* gridColumnStart = gridArea->gridColumnStart();
        GridLineValue* gridColumnEnd = gridArea->gridColumnEnd();

        if (gridColumnStart->isDefinite() && gridColumnEnd->isDefinite()) {
            gridArea->setColumnStart(gridColumnStart->value());
            gridArea->setColumnEnd(gridColumnEnd->value());
            if (gridArea->columnStart() >= gridArea->columnEnd()) {
                gridArea->setColumnEnd(gridArea->columnStart() + 1);
            }
        } else if (gridColumnStart->isDefinite()) {
            gridArea->setColumnStart(gridColumnStart->value());
            if (gridColumnEnd->isAuto()) {
                gridArea->setColumnEnd(gridArea->columnStart() + 1);
            } else if (gridColumnEnd->hasSpan()) {
                gridArea->setColumnEnd(gridArea->columnStart() +
                                       gridColumnEnd->value());
            } else {
                STARFISH_LOG_WARN("missing values");
            }
        } else if (gridColumnEnd->isDefinite()) {
            gridArea->setColumnEnd(gridColumnEnd->value());
            if (gridColumnStart->isAuto()) {
                gridArea->setColumnStart(gridArea->columnEnd() - 1);
            } else if (gridColumnStart->hasSpan()) {
                gridArea->setColumnStart(gridArea->columnEnd() -
                                         gridColumnStart->value());
            } else {
                STARFISH_LOG_WARN("missing values");
            }
        }
    }
}

GridLineValue* GridFormattingContext::parseGridLineValue(String* gridLineValue)
{
    auto val = gridLineValue->toUTF8NonGCString();
    CSSTokenVector tokens;
    CSSStyleDeclaration::tokenizeCSSValue(tokens, val.data(), val.length());

    GridLineValue* lineValue = new GridLineValue();
    for (auto token : tokens) {
        if (token.equals("auto")) {
        } else if (token.equals("span")) {
            lineValue->setHasSpan(true);
        } else {
            String* tokenStr = String::fromUTF8(token.data(), token.length());

            if (String::validDouble(tokenStr)) {
                lineValue->setValue(String::parseDouble(tokenStr));
            } else {
                lineValue->setCustomIdent(tokenStr);
            }
        }
    }

    return lineValue;
}

void GridFormattingContext::placeGridAreasWithDefinitePositions(
    GCVector<GridArea*>& gridAreaDefinite)
{
    for (auto gridArea : gridAreaDefinite) {
        placeGridArea(gridArea);
    }
}

bool GridFormattingContext::expandGridLineRows(GridArea* gridArea)
{
    if (gridArea->rowStart() > GRID_MAX_TRACK ||
        gridArea->rowEnd() > GRID_MAX_TRACK) {
        STARFISH_LOG_WARN("exceeds the max number of rows");
        return false;
    }

    if (gridArea->rowStart() == 0 && gridArea->rowEnd() == 0) {
        return true;
    }

    size_t numOfRowLines = m_gridTemplateRows.size();
    if (gridArea->rowStart() < numOfRowLines &&
        gridArea->rowEnd() <= numOfRowLines) {
        return true;
    }

    size_t numOfLinesToAdd = gridArea->rowEnd() - m_gridTemplateRows.size();
    for (size_t i = 0; i < numOfLinesToAdd; i++) {
        GridTrack line = GridTrack(0);
        line.setFixed(false);
        line.setNewLine(true);
        line.setComputed(false);
        m_gridTemplateRows.push_back(line);
    }

    return true;
}

bool GridFormattingContext::expandGridLineColumns(GridArea* gridArea)
{
    if (gridArea->columnStart() > GRID_MAX_TRACK ||
        gridArea->columnEnd() > GRID_MAX_TRACK) {
        STARFISH_LOG_WARN("exceeds the max number of columns");
        return false;
    }

    if (gridArea->columnStart() == 0 && gridArea->columnEnd() == 0) {
        return true;
    }

    size_t numOfColumnLines = m_gridTemplateColumns.size();
    if (gridArea->columnStart() < numOfColumnLines &&
        gridArea->columnEnd() <= numOfColumnLines) {
        return true;
    }

    bool hasFr = false;
    if (existColumnTemplate()) {
        for (size_t i = 0; i < m_gridTemplateColumns.size(); i++) {
            if (m_gridTemplateColumns[i].isFr() &&
                !m_gridTemplateColumns[i].isNewLine()) {
                hasFr = true;
                break;
            }
        }
    }

    size_t numOfLinesToAdd = gridArea->columnEnd() - m_gridTemplateColumns.size();
    for (size_t i = 0; i < numOfLinesToAdd; i++) {
        GridTrack line = GridTrack(0);
        line.setFixed(false);
        line.setNewLine(true);
        if (hasFr) {
            line.setComputed(false);
        } else {
            line.setAuto(true);
        }

        m_gridTemplateColumns.push_back(line);
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

        if (gridArea->rowStart() > GRID_MAX_TRACK) {
            continue;
        }

        size_t firstEmptyColumn = 0;
        bool found = false;
        for (size_t c = 0; c < m_gridTemplateColumns.size() - 1; c++) {
            if (hasAvailableGridCells(gridArea, gridArea->rowStart() - 1, c)) {
                firstEmptyColumn = c + 1;
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
            bool hasFr = false;
            if (existColumnTemplate()) {
                for (size_t i = 0; i < m_gridTemplateColumns.size(); i++) {
                    if (m_gridTemplateColumns[i].isFr() &&
                        !m_gridTemplateColumns[i].isNewLine()) {
                        hasFr = true;
                        break;
                    }
                }
            }

            GridTrack line = GridTrack(0);
            line.setFixed(false);
            line.setNewLine(true);
            if (hasFr) {
                line.setComputed(false);
            } else {
                line.setAuto(true);
            }
            m_gridTemplateColumns.push_back(line);
        }

        placeGridArea(gridArea);
    }
}

void GridFormattingContext::placeRemainingGridAreas(
    GCVector<GridArea*>& gridAreasAuto)
{
    for (auto gridArea : gridAreasAuto) {
        if (gridArea->hasRowAndColumnValues()) {
            continue;
        }

        size_t curRow = 0;
        size_t curCol = 0;
        bool found = false;
        for (size_t r = curRow; r < m_gridTemplateRows.size() - 1; r++) {
            for (size_t c = curCol; c < m_gridTemplateColumns.size() - 1; c++) {
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
        }

        if (found) {
            gridArea->setRowStart(curRow + 1);
            gridArea->setRowEnd(gridArea->rowStart() + 1);

            if (gridArea->columnStart() == 0) {
                gridArea->setColumnStart(curCol + 1);
            }
            if (gridArea->columnEnd() == 0) {
                gridArea->setColumnEnd(gridArea->columnStart() + 1);
            }
        } else {
            GridTrack line = GridTrack(0);
            line.setFixed(false);
            line.setNewLine(true);
            line.setComputed(false);
            m_gridTemplateRows.push_back(line);

            gridArea->setRowStart(m_gridTemplateRows.size() - 1);
            gridArea->setRowEnd(gridArea->rowStart() + 1);

            if (gridArea->columnStart() == 0) {
                gridArea->setColumnStart(1);
            }
            if (gridArea->columnEnd() == 0) {
                gridArea->setColumnEnd(gridArea->columnStart() + 1);
            }
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
    for (size_t r = row; r < std::min(row + height, m_gridTemplateRows.size() - 1);
         r++) {
        for (size_t c = col;
             c < std::min(col + width, m_gridTemplateColumns.size() - 1); c++) {
            if (m_areaChecker[r][c]) {
                availableCells++;
            }
        }
    }

    return availableCells == (width * height);
}

void GridFormattingContext::placeGridArea(GridArea* gridArea)
{
    for (size_t r = gridArea->rowStart() - 1; r < gridArea->rowEnd() - 1; r++) {
        for (size_t c = gridArea->columnStart() - 1;
             c < gridArea->columnEnd() - 1; c++) {
            m_areaChecker[r][c] = false;
        }
    }
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

        if (style->position() == AbsolutePositionValue)
            continue;

        size_t rowStart, rowEnd;
        size_t columnStart, columnEnd;
        convertToStartEndForRow(style, rowStart, rowEnd);
        convertToStartEndForColumn(style, columnStart, columnEnd);

        adaptStartAndEndValueForRow(*this, gridItem, m_gridTemplateRows.size(),
                                    rowStart, rowEnd);
        adaptStartAndEndValueForColumn(
            *this, gridItem, m_gridTemplateColumns.size(), columnStart, columnEnd);

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
                         return a.rowStart() < b.rowStart();
                     });

    size_t definedIdx = 0;
    size_t undefinedIdx = 0;

    for (size_t row = 0; row < m_gridTemplateRows.size();) {
        GridArea* definedGridArea = nullptr;
        GridArea* undefinedGridArea = nullptr;
        GridArea* previousGridArea = nullptr;

        if (definedIdx < defined.size()) {
            definedGridArea = &defined[definedIdx];
        }

        if (undefinedIdx < undefined.size()) {
            undefinedGridArea = &undefined[undefinedIdx];
        }

        while (fixGridAreaWithDefine(definedGridArea, row)) {
            definedIdx++;
            if (definedIdx < defined.size()) {
                definedGridArea = &defined[definedIdx];
            } else {
                definedGridArea = nullptr;
            }
        }

        while (fixGridAreaWithUndefine(&previousGridArea, undefinedGridArea,
                                       row)) {
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
    LayoutUnit sumOfColumn = 0;
    size_t numberOfAuto = 0;

    size_t numberOfMinMax = 0;
    size_t numberOfFr = 0;
    std::vector<GridTrack*> minMaxLines;
    bool hasFrInMinMax = false;
    for (size_t i = 0; i < m_gridTemplateColumns.size(); i++) {
        if (m_gridTemplateColumns[i].isMinMax()) {
            m_gridTemplateColumns[i].setOffset(
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
        for (size_t i = 0; i < m_gridTemplateColumns.size(); i++) {
            sumOfColumn += m_gridTemplateColumns[i].offset();
        }
    }

    sumOfColumn += (m_gridTemplateColumns.size() - 2) * m_columnGap;
    LayoutUnit availableWidth = m_availableWidth - sumOfColumn;
    LayoutUnit totalMinMaxGap = 0.0;
    if (numberOfMinMax && availableWidth > 0) {
        std::stable_sort(
            minMaxLines.begin(), minMaxLines.end(),
            [](const GridTrack* a, const GridTrack* b) {
                STARFISH_ASSERT(a != nullptr);
                STARFISH_ASSERT(b != nullptr);
                float maxA = a->max().isFr() ? a->offset().toFloat()
                                             : a->max().length().numberData();
                float minA = a->min().isFr() ? a->offset().toFloat()
                                             : a->min().length().numberData();
                float maxB = b->max().isFr() ? b->offset().toFloat()
                                             : b->max().length().numberData();
                float minB = b->min().isFr() ? b->offset().toFloat()
                                             : b->min().length().numberData();
                return (maxA - minA) < (maxB - minB);
            });

        for (size_t i = 0; i < minMaxLines.size(); i++) {
            GridTrack* line = (minMaxLines[i]);
            float maxLine = line->max().isFr()
                                ? line->offset().toFloat()
                                : line->max().length().numberData();
            float minLine = line->min().isFr()
                                ? line->offset().toFloat()
                                : line->min().length().numberData();

            LayoutUnit minmaxGap = maxLine - minLine;
            LayoutUnit totalMinMaxWidth = minmaxGap * numberOfMinMax;

            if (totalMinMaxWidth <= availableWidth) {
                line->setOffset(line->offset() + minmaxGap, true);
                availableWidth -= minmaxGap;
                totalMinMaxGap += minmaxGap;
            } else {
                LayoutUnit dividedWidth = availableWidth / numberOfMinMax;
                for (size_t j = i; j < minMaxLines.size(); j++) {
                    GridTrack* line = (minMaxLines[j]);
                    line->setOffset(line->offset() + dividedWidth, true);
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
    // FIXME: Split the concept of GridLine into GridLine and GridTrack
    // FIXME: Combine GridTrackSize and GridTrack
    // FIXME: remove the dummy GridTrack at [0]
    m_gridTemplateColumns.push_back(GridTrack(0));
    const GCVector<GridTrackSize>* columns =
        m_container->style()->gridTemplateColumns();

    if (columns) {
        size_t colSize = columns->size();
        for (size_t i = 0; i < colSize; i++) {
            GridTrackSize trackSize = (*columns)[i];
            int a = 10;
        }
    }

    if (columns) {
        // initializing grid lines for columns
        initializeGridLineColumns(columns);
    } else {
        GridTrack line = GridTrack(0);
        line.setAuto(true);
        line.setFixed(false);
        m_gridTemplateColumns.push_back(line);
    }

    m_gridTemplateRows.push_back(GridTrack(0));
    const GCVector<GridTrackSize>* rows =
        m_container->style()->gridTemplateRows();
    if (rows) {
        initializeGridLineRows(rows);
    }

    // parse grid template areas and store this information.
    parseGridTemplateAreas();

    // add implicit grid lines, make grid areas, and order the areas.
    // buildGridAreaAndOrdering();
    placeGridItemsIntoCells();

    // WHAT: calculate grid track size for each column using only either
    // a content width or grid-template-column
    // FIXME: 1. initialize track sizes
    assumeGridItemWidths();

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

static GridArea* getSameAreaWithColumn(GCVector<GridArea>& list,
                                       size_t rowStart, size_t start,
                                       size_t end)
{
    GridArea* target = nullptr;
    size_t maxArea = 0;
    for (size_t i = 0; i < list.size(); i++) {
        GridArea preArea = list[i];

        if (preArea.rowStart() < rowStart) {
            if (preArea.columnStart() == start && preArea.columnEnd() == end) {
                target = &list[i];
                break;
            }
        }
    }
    return target;
}

// WHAT: get the largest GridArea that includes the given grid area
static GridArea* getBiggestAreaWithRow(GCVector<GridArea>& list,
                                       size_t columnStart, size_t start, // rowStart
                                       size_t end) // rowEnd
{
    GridArea* target = nullptr;
    std::vector<GridArea*> areas;
    for (size_t i = 0; i < list.size(); i++) {
        GridArea preArea = list[i];

        if (preArea.columnStart() < columnStart) {
            if (preArea.rowStart() <= start && preArea.rowEnd() >= end) {
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
        if (end - start < areas[i]->rowEnd() - areas[i]->rowStart()) {
            target = areas[i];
            break;
        }
    }

    for (size_t i = 0; i < areas.size(); i++) {
        if (end - start < areas[i]->rowEnd() - areas[i]->rowStart() &&
            end == areas[i]->rowEnd()) {
            target = areas[i];
            break;
        }
    }

    return target;
}

void GridFormattingContext::alignGridLinesForColumns(GridArea& area,
                                                     LayoutUnit& width,
                                                     LayoutUnit& contentWidth,
                                                     bool isFixed)
{
    size_t start = area.columnStart();
    size_t end = area.columnEnd();

    LayoutUnit sumWidth(0);
    for (size_t i = start; i < end; i++) {
        sumWidth += m_gridTemplateColumns[i].offset();
    }

    size_t bigAreaStart;
    size_t bigAreaEnd;
    std::vector<size_t> bigAreaIds;
    bool hasBigAreas = hasBigAreasIncludingCurrentArea(
        m_orderedGridArea, area.rowStart(), start, end, bigAreaStart,
        bigAreaEnd, bigAreaIds);

    if (!sumWidth) {
        if (!isFixed) {
            width = contentWidth;
        }

        LayoutUnit dividedWidth = width / (end - start);
        for (size_t i = start; i < end; i++) {
            GridTrack& line = m_gridTemplateColumns[i];
            line.setOffset(dividedWidth, true);
        }
    } else if (sumWidth > contentWidth) {
        if (hasBigAreas) {
            std::vector<GridArea*> innerAreas;
            for (size_t i = 0; i < m_orderedGridArea.size(); i++) {
                GridArea preArea = m_orderedGridArea[i];
                if (std::find(bigAreaIds.begin(), bigAreaIds.end(),
                              preArea.index()) != bigAreaIds.end()) {
                    continue;
                }

                if (preArea.index() > area.index()) {
                    break;
                }

                if (preArea.rowStart() <= area.rowStart() &&
                    area.index() != preArea.index()) {
                    if (bigAreaStart <= preArea.columnStart() &&
                        bigAreaEnd >= preArea.columnEnd()) {
                        innerAreas.push_back(&m_orderedGridArea[i]);
                    }
                }
            }

            SetForGrid<size_t> set;
            for (size_t i = 0; i < innerAreas.size(); i++) {
                GridArea* inner = innerAreas[i];

                for (size_t lineNumber = inner->columnStart() + 1;
                     lineNumber <= inner->columnEnd(); lineNumber++) {
                    set.insert(lineNumber);
                }
            }

            std::vector<GridTrack*> lines;

            for (size_t i = bigAreaStart; i < bigAreaEnd; i++) {
                if (i >= area.columnStart() && i < area.columnEnd()) {
                    continue;
                }

                if (!set.find(i + 1) && !m_gridTemplateColumns[i].isFixed()) {
                    lines.push_back(&m_gridTemplateColumns[i]);
                }
            }

            if (lines.size()) {
                LayoutUnit diff = (sumWidth - contentWidth) / lines.size();
                for (size_t i = 0; i < lines.size(); i++) {
                    GridTrack* line = lines[i];
                    line->setOffset(line->offset() + diff, true);
                }

                diff = contentWidth / (end - start);
                for (size_t i = start; i <= end - 1; i++) {
                    GridTrack& line = m_gridTemplateColumns[i];
                    line.setOffset(diff, true);
                }
            } else {
                std::vector<GridTrack*> noneFixed;
                LayoutUnit sumOfFixed(0);
                for (size_t i = start; i <= end - 1; i++) {
                    if (m_gridTemplateColumns[i].isFixed() &&
                        !m_gridTemplateColumns[i].isFr()) {
                        sumOfFixed += m_gridTemplateColumns[i].offset();
                    } else {
                        noneFixed.push_back(&m_gridTemplateColumns[i]);
                    }
                }
                if (noneFixed.size()) {
                    LayoutUnit diff = contentWidth / noneFixed.size();

                    for (size_t i = 0; i < noneFixed.size(); i++) {
                        GridTrack* line = noneFixed[i];
                        line->setOffset(diff, true);
                    }

                    noneFixed.clear();
                    sumOfFixed = 0;

                    size_t startForTarget = bigAreaStart;
                    size_t endForTarget = bigAreaEnd;

                    for (size_t i = startForTarget; i <= endForTarget - 1;
                         i++) {
                        if (m_gridTemplateColumns[i].isFixed() &&
                            !m_gridTemplateColumns[i].isFr()) {
                            sumOfFixed += m_gridTemplateColumns[i].offset();
                        } else {
                            noneFixed.push_back(&m_gridTemplateColumns[i]);
                        }
                    }

                    if (noneFixed.size()) {
                        diff = (sumWidth - contentWidth) / noneFixed.size();

                        for (size_t i = 0; i < noneFixed.size(); i++) {
                            GridTrack* line = noneFixed[i];
                            line->setOffset(line->offset() + diff, true);
                        }
                    }
                }
            }
        }
    } else if (sumWidth < contentWidth) {
        if (hasBigAreas) {
            std::vector<GridArea*> innerAreas;
            for (size_t i = 0; i < m_orderedGridArea.size(); i++) {
                GridArea preArea = m_orderedGridArea[i];
                if (std::find(bigAreaIds.begin(), bigAreaIds.end(),
                              preArea.index()) != bigAreaIds.end()) {
                    continue;
                }

                if (preArea.index() > area.index()) {
                    break;
                }

                if (preArea.rowStart() <= area.rowStart() &&
                    area.index() != preArea.index()) {
                    if (bigAreaStart <= preArea.columnStart() &&
                        bigAreaEnd >= preArea.columnEnd()) {
                        innerAreas.push_back(&m_orderedGridArea[i]);
                    }
                }
            }

            SetForGrid<size_t> set;

            for (size_t i = 0; i < innerAreas.size(); i++) {
                GridArea* inner = innerAreas[i];

                for (size_t lineNumber = inner->columnStart() + 1;
                     lineNumber <= inner->columnEnd(); lineNumber++) {
                    set.insert(lineNumber);
                }
            }

            std::vector<GridTrack*> lines;

            // Filter fixed lines.
            for (size_t i = bigAreaStart; i < bigAreaEnd; i++) {
                if (i >= area.columnStart() && i < area.columnEnd()) {
                    continue;
                }

                if (!set.find(i + 1) && !m_gridTemplateColumns[i].isFixed()) {
                    lines.push_back(&m_gridTemplateColumns[i]);
                }
            }

            if (lines.size()) {
                LayoutUnit diff = contentWidth - sumWidth;

                LayoutUnit sumOfLines(0);
                for (size_t i = 0; i < lines.size(); i++) {
                    sumOfLines += lines[i]->offset();
                }

                LayoutUnit remaining(0);
                for (size_t i = 0; i < lines.size(); i++) {
                    LayoutUnit offset =
                        diff * (lines[i]->offset() / sumOfLines);

                    if (lines[i]->offset() - offset > 0) {
                        lines[i]->setOffset(lines[i]->offset() - offset, true);
                    } else {
                        remaining += offset - lines[i]->offset();
                        lines[i]->setOffset(0, true);
                    }
                }

                // FIXME: If remaining is not '0', we have to distribute width.

                LayoutUnit dividedWidth = contentWidth / (end - start);

                for (size_t i = start; i <= end - 1; i++) {
                    GridTrack& line = m_gridTemplateColumns[i];
                    line.setOffset(dividedWidth, true);
                }
            } else {
                std::vector<GridTrack*> noneFixed;
                LayoutUnit sumOfFixed(0);

                for (size_t i = start; i <= end - 1; i++) {
                    if (m_gridTemplateColumns[i].isFixed() &&
                        !m_gridTemplateColumns[i].isFr()) {
                        sumOfFixed += m_gridTemplateColumns[i].offset();
                    } else {
                        noneFixed.push_back(&m_gridTemplateColumns[i]);
                    }
                }

                if (noneFixed.size()) {
                    LayoutUnit dividedWidth =
                        (contentWidth - (sumWidth - sumOfFixed)) /
                        noneFixed.size();
                    for (size_t i = 0; i < noneFixed.size(); i++) {
                        GridTrack* line = noneFixed[i];
                        line->setOffset(line->offset() + dividedWidth, true);
                    }
                }
            }
        } else {
            size_t count = 0;
            for (size_t i = start; i <= end - 1; i++) {
                if (!m_gridTemplateColumns[i].offset()) {
                    count++;
                }
            }

            if (count) {
                LayoutUnit dividedWidth = (contentWidth - sumWidth) / count;
                for (size_t i = start; i <= end - 1; i++) {
                    if (!m_gridTemplateColumns[i].offset()) {
                        GridTrack& line = m_gridTemplateColumns[i];
                        line.setOffset(dividedWidth, true);
                    }
                }
            } else {
                std::vector<GridTrack*> noneFixed; // fr grid line for column
                LayoutUnit sumOfFixed(0);

                for (size_t i = start; i <= end - 1; i++) {
                    if (m_gridTemplateColumns[i].isLength() &&
                        m_gridTemplateColumns[i].isFixed()) {
                        sumOfFixed += m_gridTemplateColumns[i].offset();
                    } else if (m_gridTemplateColumns[i].isFr()) {
                        noneFixed.push_back(&m_gridTemplateColumns[i]);
                    }
                }

                // If a grid area has multiple grid lines with fr unit,
                // each grid line should have a value of (contentWidth -
                // (gridlines' offset + fixedSum) /n-fr))
                if (noneFixed.size()) {
                    LayoutUnit dividedWidth =
                        (contentWidth - (sumWidth + sumOfFixed)) /
                        noneFixed.size();
                    for (size_t i = 0; i < noneFixed.size(); i++) {
                        GridTrack* line = noneFixed[i];
                        line->setOffset(line->offset() + dividedWidth, true);
                    }
                }
            }
        }
    }
}

void GridFormattingContext::alignGridLinesForRows(GridArea& area)
{
    FrameBox* gridItem = area.box();
    LayoutUnit sumHeight(0); // sumOfRowHeights
    size_t start = area.rowStart(); // rowStart
    size_t end = area.rowEnd(); // rowEnd
    LayoutUnit contentHeight =
        gridItem->height() +
        gridItem->style()->margin().top().specifiedValue(0, m_container) +
        gridItem->style()->margin().bottom().specifiedValue(0, m_container);

    area.setContentHeight(gridItem->height());

    for (size_t i = start; i < end; i++) {
        sumHeight += m_gridTemplateRows[i].offset();
    }

    GridArea* biggest = getBiggestAreaWithRow(m_orderedGridArea,
                                              area.columnStart(), start, end);

    if (!sumHeight) {
        LayoutUnit dividedHeight = contentHeight / (end - start);
        for (size_t i = start; i <= end - 1; i++) {
            GridTrack& line = m_gridTemplateRows[i];
            line.setOffset(dividedHeight, true);
        }
    } else if (sumHeight > contentHeight) {
        if (biggest) {
            std::vector<GridArea*> innerAreas;
            for (size_t i = 0; i < m_orderedGridArea.size(); i++) {
                GridArea preArea = m_orderedGridArea[i];
                if (preArea.index() == biggest->index()) {
                    continue;
                }

                if (preArea.index() > area.index()) {
                    break;
                }

                if (preArea.columnStart() <= area.columnStart() &&
                    area.index() != preArea.index()) {
                    if (biggest->rowStart() <= preArea.rowStart() &&
                        biggest->rowEnd() >= preArea.rowEnd()) {
                        innerAreas.push_back(&m_orderedGridArea[i]);
                    }
                }
            }

            SetForGrid<size_t> set;
            for (size_t i = 0; i < innerAreas.size(); i++) {
                GridArea* inner = innerAreas[i];

                for (size_t lineNumber = inner->rowStart() + 1;
                     lineNumber <= inner->rowEnd(); lineNumber++) {
                    set.insert(lineNumber);
                }
            }

            std::vector<GridTrack*> lines;
            for (size_t i = biggest->rowStart(); i < biggest->rowEnd(); i++) {
                if (i >= area.rowStart() && i < area.rowEnd()) {
                    continue;
                }

                if (!set.find(i + 1) && !m_gridTemplateRows[i].isFixed()) {
                    lines.push_back(&m_gridTemplateRows[i]);
                }
            }

            if (lines.size()) {
                LayoutUnit diff = (sumHeight - contentHeight) / lines.size();
                for (size_t i = 0; i < lines.size(); i++) {
                    GridTrack* line = lines[i];
                    line->setOffset(line->offset() + diff, true);
                }

                diff = contentHeight / (end - start);
                for (size_t i = start; i <= end - 1; i++) {
                    GridTrack& line = m_gridTemplateRows[i];
                    line.setOffset(diff, true);
                }
            } else {
                std::vector<GridTrack*> noneFixed;
                LayoutUnit sumOfFixed(0);

                for (size_t i = start; i <= end - 1; i++) {
                    if (m_gridTemplateRows[i].isFixed() &&
                        !m_gridTemplateRows[i].isFr()) {
                        sumOfFixed += m_gridTemplateRows[i].offset();
                    } else {
                        noneFixed.push_back(&m_gridTemplateRows[i]);
                    }
                }

                if (noneFixed.size()) {
                    LayoutUnit diff = contentHeight / noneFixed.size();

                    for (size_t i = 0; i < noneFixed.size(); i++) {
                        GridTrack* line = noneFixed[i];
                        line->setOffset(diff, true);
                    }

                    noneFixed.clear();
                    sumOfFixed = 0;

                    size_t startForTarget = biggest->rowStart();
                    size_t endForTarget = biggest->rowEnd();

                    for (size_t i = startForTarget; i <= endForTarget - 1;
                         i++) {
                        if (m_gridTemplateRows[i].isFixed() &&
                            !m_gridTemplateRows[i].isFr()) {
                            sumOfFixed += m_gridTemplateRows[i].offset();
                        } else {
                            noneFixed.push_back(&m_gridTemplateRows[i]);
                        }
                    }

                    if (noneFixed.size()) {
                        diff = (sumHeight - contentHeight) / noneFixed.size();

                        for (size_t i = 0; i < noneFixed.size(); i++) {
                            GridTrack* line = noneFixed[i];
                            line->setOffset(line->offset() + diff, true);
                        }
                    }
                }
            }
        }
    } else if (sumHeight < contentHeight) {
        if (biggest) {
            std::vector<GridArea*> innerAreas;
            for (size_t i = 0; i < m_orderedGridArea.size(); i++) {
                GridArea preArea = m_orderedGridArea[i];
                if (preArea.index() == biggest->index()) {
                    continue;
                }

                if (preArea.index() > area.index()) {
                    break;
                }

                if (preArea.columnStart() <= area.columnStart() &&
                    area.index() != preArea.index()) {
                    if (biggest->rowStart() <= preArea.rowStart() &&
                        biggest->rowEnd() >= preArea.rowEnd()) {
                        innerAreas.push_back(&m_orderedGridArea[i]);
                    }
                }
            }

            SetForGrid<size_t> set;

            for (size_t i = 0; i < innerAreas.size(); i++) {
                GridArea* inner = innerAreas[i];

                for (size_t lineNumber = inner->rowStart() + 1;
                     lineNumber <= inner->rowEnd(); lineNumber++) {
                    set.insert(lineNumber);
                }
            }

            std::vector<GridTrack*> lines;

            for (size_t i = biggest->rowStart(); i < biggest->rowEnd(); i++) {
                if (i >= area.rowStart() && i < area.rowEnd()) {
                    continue;
                }

                if (!set.find(i + 1) && !m_gridTemplateRows[i].isFixed()) {
                    lines.push_back(&m_gridTemplateRows[i]);
                }
            }

            if (lines.size()) {
                LayoutUnit diff = contentHeight - sumHeight;

                LayoutUnit sumOfLines(0);
                for (size_t i = 0; i < lines.size(); i++) {
                    sumOfLines += lines[i]->offset();
                }

                LayoutUnit remaining(0);
                for (size_t i = 0; i < lines.size(); i++) {
                    LayoutUnit offset =
                        diff * (lines[i]->offset() / sumOfLines);

                    if (lines[i]->offset() - offset > 0) {
                        lines[i]->setOffset(lines[i]->offset() - offset, true);
                    } else {
                        remaining += offset - lines[i]->offset();
                        lines[i]->setOffset(0, true);
                    }
                }

                // FIXME: If remaining is not '0', we have to distribute height.

                LayoutUnit dividedHeight = contentHeight / (end - start);

                for (size_t i = start; i <= end - 1; i++) {
                    GridTrack& line = m_gridTemplateRows[i];
                    line.setOffset(dividedHeight, true);
                }
            } else {
                std::vector<GridTrack*> noneFixed;
                LayoutUnit sumOfFixed(0);

                for (size_t i = start; i <= end - 1; i++) {
                    if (m_gridTemplateRows[i].isFixed() &&
                        !m_gridTemplateRows[i].isFr()) {
                        sumOfFixed += m_gridTemplateRows[i].offset();
                    } else {
                        noneFixed.push_back(&m_gridTemplateRows[i]);
                    }
                }

                if (noneFixed.size()) {
                    LayoutUnit dividedHeight =
                        (contentHeight - (sumHeight - sumOfFixed)) /
                        noneFixed.size();
                    for (size_t i = 0; i < noneFixed.size(); i++) {
                        GridTrack* line = noneFixed[i];
                        line->setOffset(line->offset() + dividedHeight, true);
                    }
                }
            }
        } else {
            size_t count = 0;
            for (size_t i = start; i <= end - 1; i++) {
                if (!m_gridTemplateRows[i].offset()) {
                    count++;
                }
            }

            if (count) {
                LayoutUnit dividedHeight = (contentHeight - sumHeight) / count;
                for (size_t i = start; i <= end - 1; i++) {
                    if (!m_gridTemplateRows[i].offset()) {
                        GridTrack& line = m_gridTemplateRows[i];
                        line.setOffset(dividedHeight, true);
                    }
                }
            } else {
                std::vector<GridTrack*> noneFixed;
                LayoutUnit sumOfFixed(0);

                for (size_t i = start; i <= end - 1; i++) {
                    if (m_gridTemplateRows[i].isFixed() &&
                        !m_gridTemplateRows[i].isFr()) {
                        sumOfFixed += m_gridTemplateRows[i].offset();
                    } else {
                        noneFixed.push_back(&m_gridTemplateRows[i]);
                    }
                }

                if (noneFixed.size()) {
                    LayoutUnit dividedHeight =
                        (contentHeight - sumOfFixed) / noneFixed.size();
                    for (size_t i = 0; i < noneFixed.size(); i++) {
                        GridTrack* line = noneFixed[i];
                        line->setOffset(dividedHeight, true);
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

void GridFormattingContext::assumeGridItemWidths()
{
    for (GridArea& gridArea : m_orderedGridArea) {
        FrameBox* gridItemBox = gridArea.box();
        ComputedStyle* style = gridItemBox->style();

        LayoutUnit width;
        LayoutUnit contentWidth;
        LayoutUnit preferredMinWidth;
        bool isFixed = true;

        LayoutSize mbp = fetchFixedMarginBorderPadding(m_container, style);

        if (style->width().isFixed()) {
            width = style->width().fixed() + mbp.width();
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
            isFixed = false;
        }
        gridArea.setPreferredWidth(contentWidth);
        gridArea.setPreferredMinWidth(preferredMinWidth);
        // This part relies on calculating 'width'.
        // grid lines' offset is updated.
        alignGridLinesForColumns(gridArea, width, contentWidth, isFixed);
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
    track.setOffset(maxWidthSoFar, true);
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
        sumOfAllNonAutoWidths += track.offset();
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
            track.setOffset(track.offset() + additionalAutoTrackSpace, true);
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
        yOffsetForRowsSoFar += track.offset() + m_rowGap;
    }

    for (GridArea& area : m_orderedGridArea) {
        STARFISH_ASSERT(area.rowStart() < yOffsetsForRows.size());
        area.box()->setHeight(area.contentHeight());
        LayoutUnit yOffset = yOffsetsForRows[area.rowStart()];
        GridTrack track = m_gridTemplateRows[area.rowStart()];

        LayoutUnit yPos =
            yOffset + (track.offset() / 2) - (area.box()->height() / 2);
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
                width += m_gridTemplateColumns[i].offset();
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

        alignGridLinesForRows(area);
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
                width += m_gridTemplateColumns[i].offset();
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

            AlignItemValue alignItem = m_container->style()->alignItems();
            if (alignItem == AlignItemValue::CenterAlignItemValue) {
                height = area.contentHeight();
            } else {
                for (size_t i = area.rowStart(); i < area.rowEnd(); i++) {
                    height += m_gridTemplateRows[i].offset();
                }
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

        alignGridLinesForRows(area);
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
