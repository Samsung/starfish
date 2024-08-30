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

#ifndef __StarfishFrameGridBox__
#define __StarfishFrameGridBox__

#include "core/layout/FrameBlockBox.h"

#include <bitset>

namespace Starfish {

class ComputedStyle;
class GridFormattingContext;

class GridLine : public gc {
public:
    bool isDefinite()
    {
        if (m_value.hasValue() && !hasSpan()) {
            return true;
        }
        return false;
    }

    bool isAuto()
    {
        if (m_value.hasValue()) {
            return false;
        }

        if (hasSpan()) {
            return false;
        }

        return true;
    }

    bool hasValue()
    {
        return m_value.hasValue();
    }

    size_t value()
    {
        if (m_value.hasValue()) {
            return m_value.value();
        } else {
            return 0;
        }
    }

    void setValue(size_t v)
    {
        m_value = v;
    }

    bool hasSpan()
    {
        return m_spanValue > 1;
    }

    size_t spanValue()
    {
        return m_spanValue;
    }

    void setSpanValue(size_t v)
    {
        m_spanValue = v;
    }

    bool hasCustomIdent()
    {
        return !m_customIdent->equals(String::emptyString);
    }

    String* customIdent()
    {
        return m_customIdent;
    }

    void setCustomIdent(String* v)
    {
        m_customIdent = v;
    }

private:
    Nullable<size_t> m_value;
    size_t m_spanValue{ 1 };
    String* m_customIdent{ String::emptyString };
};

class GridArea : public gc {
public:
    GridArea(FrameBox* box, size_t id)
        : m_box(box)
        , m_index(id)
    {
        if (box) {
            m_isMarginLeftAuto = box->style()->margin().left().isAuto();
            m_isMarginRightAuto = box->style()->margin().right().isAuto();
        }
    }

    GridArea(FrameBox* box, size_t idx, size_t rowStart, size_t rowEnd,
             size_t columnStart, size_t columnEnd)
        : GridArea(box, idx)
    {
        m_rowStartLine->setValue(rowStart);
        m_rowEndLine->setValue(rowEnd);
        m_columnStartLine->setValue(columnStart);
        m_columnEndLine->setValue(columnEnd);
    }

    bool isDefinite()
    {
        if ((m_rowStartLine->isDefinite() || m_rowEndLine->isDefinite()) &&
            (m_columnStartLine->isDefinite() ||
             m_columnEndLine->isDefinite())) {
            return true;
        }
        return false;
    }

    DEFINE_GETTER(FrameBox*, box);
    DEFINE_GETTER(size_t, index);
    DEFINE_GETTER_SETTER(LayoutUnit, preferredWidth, PreferredWidth);
    DEFINE_GETTER_SETTER(LayoutUnit, preferredMinWidth, PreferredMinWidth);
    DEFINE_GETTER_SETTER(LayoutUnit, contentHeight, ContentHeight);

    size_t rowStart() const
    {
        return m_rowStartLine->value();
    }

    void setRowStart(size_t v) const
    {
        m_rowStartLine->setValue(v);
    }

    size_t rowEnd() const
    {
        return m_rowEndLine->value();
    }

    void setRowEnd(size_t v) const
    {
        m_rowEndLine->setValue(v);
    }

    size_t columnStart() const
    {
        return m_columnStartLine->value();
    }

    void setColumnStart(size_t v) const
    {
        m_columnStartLine->setValue(v);
    }

    size_t columnEnd() const
    {
        return m_columnEndLine->value();
    }

    void setColumnEnd(size_t v) const
    {
        m_columnEndLine->setValue(v);
    }

    bool isMarginLeftAuto()
    {
        return m_isMarginLeftAuto;
    }

    bool isMarginRightAuto()
    {
        return m_isMarginRightAuto;
    }

    bool hasRowAndColumnValues()
    {
        if (m_rowStartLine->hasValue() && m_rowEndLine->hasValue() &&
            m_columnStartLine->hasValue() && m_columnEndLine->hasValue()) {
            return true;
        }
        return false;
    }

    void moveRow(size_t v) const
    {
        size_t previous = rowEnd() - rowStart();
        setRowStart(v);
        setRowEnd(v + previous);
    }

    void moveColumn(size_t v) const
    {
        size_t previous = columnEnd() - columnStart();
        setColumnStart(v);
        setColumnEnd(v + previous);
    }

    void parseGridRowAndColumnValues(GridFormattingContext& ctx);
    GridLine* parseGridLine(String* gridLineValue);
    void resolveDefinitePositionValues();
    size_t rowSpanValue();
    size_t columnSpanValue();

private:
    FrameBox* m_box;
    size_t m_index{ 0 };
    LayoutUnit m_preferredWidth;
    LayoutUnit m_preferredMinWidth;
    LayoutUnit m_contentHeight;
    bool m_isMarginLeftAuto = false;
    bool m_isMarginRightAuto = false;

    GridLine* m_rowStartLine = new GridLine();
    GridLine* m_rowEndLine = new GridLine();
    GridLine* m_columnStartLine = new GridLine();
    GridLine* m_columnEndLine = new GridLine();
};

enum class GridTrackType {
    kAuto,
    kLength,
    kFlexibleLength,
    kMinMax,
    kMinContent,
    kMaxContent,
};

class GridTrack : public gc {
public:
    LayoutUnit size() const
    {
        return m_size; // including margin, border, and padding
    }

    LayoutUnit growthLimit() const
    {
        return m_growthLimit;
    }

    void setGrowthLimit(LayoutUnit growthLimit)
    {
        m_growthLimit = growthLimit;
    }

    LayoutUnit flexibleLength()
    {
        return m_flexibleLength;
    }

    const GridLength& min() const
    {
        return m_min;
    }

    const GridLength& max() const
    {
        return m_max;
    }

    void setSize(LayoutUnit size)
    {
        m_size = size;
    }

    void setMinMax(GridLength min, GridLength max)
    {
        m_min = min;
        m_max = max;
    }

    void setImplicitLine(bool newLine)
    {
        m_implicitLine = newLine;
    }

    bool isImplicitLine() const
    {
        return m_implicitLine;
    }

    bool isLength() const
    {
        return m_type == GridTrackType::kLength;
    }

    bool isFlexibleLength() const
    {
        return m_type == GridTrackType::kFlexibleLength;
    }

    bool isMinMax() const
    {
        return m_type == GridTrackType::kMinMax;
    }

    bool isAuto() const
    {
        return m_type == GridTrackType::kAuto;
    }

    bool isMinContent() const
    {
        return m_type == GridTrackType::kMinContent;
    }

    bool isMaxContent() const
    {
        return m_type == GridTrackType::kMaxContent;
    }

    GridTrack()
        : m_implicitLine(true)
    {
    }

    GridTrack(LayoutUnit size)
        : m_size(size)
        , m_growthLimit(size)
        , m_type(GridTrackType::kLength)
    {
    }

    // The 'computed' is for the 'fr' unit.
    GridTrack(LayoutUnit fr, bool computed)
        : m_flexibleLength(fr)
        , m_type(GridTrackType::kFlexibleLength)
    {
    }

    GridTrack(GridLength min, GridLength max)
        : m_min(min)
        , m_max(max)
        , m_type(GridTrackType::kMinMax)
    {
        if (min.isLength() && min.length().isFixed()) {
            m_size = min.length().numberData();
        } else if (min.isLength() && min.length().isAuto()) {
            m_size = 0;
        }
    }

    GridTrack(GridTrackType type)
        : m_type(type)
    {
    }

private:
    LayoutUnit m_size;
    LayoutUnit m_growthLimit{ intMaxForLayoutUnit };
    LayoutUnit m_flexibleLength;
    GridLength m_min;
    GridLength m_max;
    bool m_implicitLine{ false };
    GridTrackType m_type{ GridTrackType::kAuto };
};

class GridCellTable {
public:
    static const int kMaxTrack = 64;

    GridCellTable();
    bool hasFreeSlot(size_t row, size_t col);
    void setOccupied(size_t row, size_t col);
    std::string toString();

private:
    std::bitset<kMaxTrack * kMaxTrack> m_gridCellTable;
};

class GridFormattingContext {
public:
    static bool doesParticipateInGridFormattingContext(Frame* GridItem);

    GridFormattingContext(LayoutContext& ctx, FrameGridBox* container,
                          LayoutUnit availableWidth);

    void computeColumnsAndRows();

    bool needsGridItemLayout(FrameBox* gridItem, ComputedStyle* style,
                             bool testWidthOnly);

    LayoutUnit preferredWidth();

    LayoutContext& layoutContext()
    {
        return m_layoutContext;
    }

    GridArea* getNamedGridArea(String* name);

    const GCVector<GridTrack>& gridTemplateColumns() const;

private:
    void buildGridTrackTemplate();
    void layoutGridItems();

    void parseGridTemplateAreas();

    void initializeGridTracksFromGridTemplateColumnsAndRows(
        const GCVector<GridArea*>& areas);
    void initializeGridTracks(GCVector<GridTrack>& gridTracks,
                              const GCVector<GridTrackSize*>* gridTrackSizes,
                              bool isColumnDirection,
                              const GCVector<GridArea*>& areas);
    void initializeGridTracksWithFixedRepeat(
        GCVector<GridTrack>& gridTracks, GridTrackSizeFixedRepeat* fixedRepeat,
        bool isColumnDirection);
    void initializeGridTracksWithAutoRepeat(GCVector<GridTrack>& gridTracks,
                                            GridTrackSizeAutoRepeat* autoepeat,
                                            bool isColumnDirection,
                                            const GCVector<GridArea*>& areas);
    GridTrack gridTrackSizeToGridTrack(GridTrackSize* gridTrackSize,
                                       bool isColumnDirection);

    void placeGridItemsIntoCells();
    void placeGridAreasWithDefinitePositions(
        GCVector<GridArea*>& gridAreaDefinite);
    bool addImplicitGridLineRows(GridArea* gridArea);
    bool addImplicitGridLineColumns(GridArea* gridArea);

    void placeGridAreasLockedToRows(GCVector<GridArea*>& gridAreasAuto);
    void placeRemainingGridAreas(GCVector<GridArea*>& gridAreasAuto);
    bool hasAvailableGridCells(GridArea* gridArea, size_t row, size_t col);
    void placeGridArea(GridArea* gridArea);
    void rearrangeGridArea(GCVector<GridArea*>& gridAreasAuto,
                           GridArea* gridArea, size_t* row, size_t* col);

    void initializePreferredWidths();
    void resolveIntrinsicColumnTrackSizes();
    void increaseColumnGridTracksForSpans(
        GCVector<GridArea*>& gridAreasWithSpans);
    bool isFrPartOfTrack(GridArea* gridArea);
    bool isFrPartOfRowTrack(GridArea* gridArea);

    void maximizeColumnTracks();
    void expandFlexibleColumnTracks();
    void stretchAutoColumnTracks();
    void applyAlignItems();

    void initializeContentHeights();
    void applyImplicitTrackSizing();
    void resolveIntrinsicRowTrackSizes();
    void increaseRowGridTracksForSpans(GCVector<GridArea*>& gridAreasWithSpans);

    void expandFlexibleRowTracks();
    void layoutGridItemFrameBoxes();
    void layoutGridItemFrameBox(GridArea& gridArea, bool widthOnly);

    void resolveMinMaxContentSize(size_t gridTrackIndex);

    void layoutNonGridItems();
    void repositionFixedNonGridItem(FrameBox* nonGridItem);

    LayoutSize fetchFixedMargin(FrameGridBox* grid, ComputedStyle* style);
    LayoutSize fetchFixedMarginBorderPadding(FrameGridBox* grid,
                                             ComputedStyle* style);

    void insertNamedGridArea(const std::pair<std::string, GridArea>& pair);
    GCVector<GridArea*> createGridAreas();
    void classifyGridAreas(const GCVector<GridArea*>& areas,
                           GCVector<GridArea*>& definiteAreas,
                           GCVector<GridArea*>& autoAreas);

    LayoutContext& m_layoutContext;
    FrameGridBox* m_container;
    LayoutUnit m_availableWidth;
    GCVector<GridTrack> m_gridTemplateColumns;
    GCVector<GridTrack> m_gridTemplateRows;
    GCVector<GridArea> m_orderedGridArea;
    GCUnorderedMap<std::string, GCVector<GridArea>> m_namedAreaMap;

    LayoutUnit m_rowGap;
    LayoutUnit m_columnGap;

    GridCellTable m_gridCellTable;
    GCVector<FrameBox*> m_nonGridItems;
};

class FrameGridBox final : public FrameBlockBox {
public:
    FrameGridBox(Node* node, ComputedStyle* style);

    virtual const char* name() override
    {
        return "FrameGridBox";
    }

    virtual bool isFrameGridBox() override
    {
        return true;
    }

    virtual bool hasBlockFlow() override
    {
        return true;
    }

    virtual void computeStyleFlags() override;

    bool hasFixedStyleWidth()
    {
        return m_hasFixedStyleWidth;
    }

    bool hasFixedStyleHeight()
    {
        return m_hasFixedStyleHeight;
    }

    virtual bool shouldLayout(LayoutContext& ctx,
                              LayoutWantToResolve resolveWhat,
                              FrameBox* containingBox) override;

    void layoutGrid(LayoutContext& ctx);
    void computePreferredWidth(PreferredWidthContext& ctx) override;
    bool canStratchItem();

    LengthData insets();

    const GCVector<GridTrack>& gridTemplateColumns() const;

    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(FrameGridBox));
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(FrameGridBox)] = { 0 };
            FrameGridBox::fillGCDescriptor(desc);
            descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameGridBox));
            typeInited = true;
        }

        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }

    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        FrameBlockBox::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(FrameGridBox, m_gridTemplateColumns));
    }

    bool m_hasFixedStyleWidth{ false };
    bool m_hasFixedStyleHeight{ false };
    GCVector<GridTrack> m_gridTemplateColumns;
};

template <typename dataType>
class SetForGrid {
private:
    std::vector<dataType> dataSet;

public:
    void insert(dataType data)
    {
        for (size_t i = 0; i < dataSet.size(); i++) {
            if (data == dataSet[i]) {
                return;
            }
        }

        dataSet.push_back(data);
    }

    bool find(dataType data)
    {
        for (size_t i = 0; i < dataSet.size(); i++) {
            if (data == dataSet[i]) {
                return true;
            }
        }
        return false;
    }

    size_t size()
    {
        return dataSet.size();
    }

    void sort()
    {
        std::stable_sort(dataSet.begin(), dataSet.end());
    }

    std::vector<dataType>& set()
    {
        sort();
        return dataSet;
    }
};

} // namespace Starfish

#endif
