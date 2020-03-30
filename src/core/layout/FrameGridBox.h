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

namespace Starfish {

class ComputedStyle;
class FrameBox;
class FrameGridBox;
class LineBox;

class GridLineValue : public gc {
public:
    bool isDefinite()
    {
        if (m_value.hasValue() && !m_hasSpan) {
            return true;
        }
        return false;
    }

    bool isAuto()
    {
        if (m_value.hasValue()) {
            return false;
        }

        if (m_hasSpan) {
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
        return m_hasSpan;
    }

    void setHasSpan(bool v)
    {
        m_hasSpan = v;
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
    bool m_hasSpan{ false };
    Nullable<size_t> m_value;
    String* m_customIdent{ String::emptyString };
};

class GridArea : public gc {
public:
    GridArea(FrameBox* box, size_t id)
        : m_box(box)
        , m_index(id)
    {
    }

    GridArea(FrameBox* box, size_t idx, size_t rowStart, size_t rowEnd,
             size_t columnStart, size_t columnEnd)
        : m_box(box)
        , m_index(idx)
        , m_rowStart(rowStart)
        , m_rowEnd(rowEnd)
        , m_columnStart(columnStart)
        , m_columnEnd(columnEnd)
    {
    }

    bool isDefinite()
    {
        if ((m_gridRowStart->isDefinite() || m_gridRowEnd->isDefinite()) &&
            (m_gridColumnStart->isDefinite() ||
             m_gridColumnEnd->isDefinite())) {
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
        if (m_rowStart.hasValue()) {
            return m_rowStart.value();
        } else {
            return 0;
        }
    }

    size_t rowEnd() const
    {
        if (m_rowEnd.hasValue()) {
            return m_rowEnd.value();
        } else {
            return 0;
        }
    }

    size_t columnStart() const
    {
        if (m_columnStart.hasValue()) {
            return m_columnStart.value();
        } else {
            return 0;
        }
    }

    size_t columnEnd() const
    {
        if (m_columnEnd.hasValue()) {
            return m_columnEnd.value();
        } else {
            return 0;
        }
    }

    DEFINE_SETTER(size_t, rowStart, RowStart);
    DEFINE_SETTER(size_t, rowEnd, RowEnd);
    DEFINE_SETTER(size_t, columnStart, ColumnStart);
    DEFINE_SETTER(size_t, columnEnd, ColumnEnd);

    DEFINE_GETTER_SETTER(GridLineValue*, gridRowStart, GridRowStart);
    DEFINE_GETTER_SETTER(GridLineValue*, gridRowEnd, GridRowEnd);
    DEFINE_GETTER_SETTER(GridLineValue*, gridColumnStart, GridColumnStart);
    DEFINE_GETTER_SETTER(GridLineValue*, gridColumnEnd, GridColumnEnd);

    bool hasRowAndColumnValues()
    {
        if (m_rowStart.hasValue() && m_rowEnd.hasValue() &&
            m_columnStart.hasValue() && m_columnEnd.hasValue()) {
            return true;
        }
        return false;
    }

private:
    FrameBox* m_box;
    size_t m_index{ 0 };
    LayoutUnit m_preferredWidth;
    LayoutUnit m_preferredMinWidth;
    LayoutUnit m_contentHeight;

    Nullable<size_t> m_rowStart;
    Nullable<size_t> m_rowEnd;
    Nullable<size_t> m_columnStart;
    Nullable<size_t> m_columnEnd;

    GridLineValue* m_gridRowStart{ nullptr };
    GridLineValue* m_gridRowEnd{ nullptr };
    GridLineValue* m_gridColumnStart{ nullptr };
    GridLineValue* m_gridColumnEnd{ nullptr };
};

class GridTrack : public gc {
public:
    enum GridLineType {
        Length,
        Fr,
        MinMax,
        MinContent,
        MaxContent,
    };

    LayoutUnit offset() const
    {
        return m_offset;
    }

    LayoutUnit fr()
    {
        return m_fr;
    }

    const GridLength& min() const
    {
        return m_min;
    }

    const GridLength& max() const
    {
        return m_max;
    }

    void setOffset(LayoutUnit offset, bool computed)
    {
        m_computed = computed;
        m_offset = offset;
    }

    void setFixed(bool fixed)
    {
        m_fixed = fixed;
    }

    void setFr(LayoutUnit fr)
    {
        m_fr = fr;
    }

    void setMinMax(GridLength min, GridLength max)
    {
        m_min = min;
        m_max = max;
    }

    void setComputed(bool computed)
    {
        m_computed = computed;
    }

    void setNewLine(bool newLine)
    {
        m_newLine = newLine;
    }

    void setContaining(bool contain)
    {
        m_containing = contain;
    }

    void setAuto(bool isAuto)
    {
        m_isAuto = isAuto;
    }

    bool isFixed()
    {
        return m_fixed;
    }

    bool isNewLine()
    {
        return m_newLine;
    }

    bool isComputed()
    {
        return m_computed;
    }

    bool isLength()
    {
        return m_type == GridLineType::Length;
    }

    bool isFr()
    {
        return m_type == GridLineType::Fr;
    }

    bool isMinMax()
    {
        return m_type == GridLineType::MinMax;
    }

    bool isAuto()
    {
        return m_isAuto;
    }

    bool isMinContent()
    {
        return m_type == GridLineType::MinContent;
    }

    bool isMaxContent()
    {
        return m_type == GridLineType::MaxContent;
    }

    bool isContaining()
    {
        return m_containing;
    }

    GridTrack(LayoutUnit offset)
        : m_offset(offset)
        , m_fr(0)
        , m_min(GridLength())
        , m_max(GridLength())
        , m_computed(true)
        , m_isAuto(false)
        , m_fixed(true)
        , m_newLine(false)
        , m_type(GridLineType::Length)
        , m_containing(false)
    {
    }

    // The 'computed' is for the 'fr' unit.
    GridTrack(LayoutUnit fr, bool computed)
        : m_offset(0)
        , m_fr(fr)
        , m_min(GridLength())
        , m_max(GridLength())
        , m_computed(computed)
        , m_isAuto(false)
        , m_fixed(true)
        , m_newLine(false)
        , m_type(GridLineType::Fr)
        , m_containing(false)
    {
    }

    GridTrack(GridLength min, GridLength max)
        : m_fr(0)
        , m_min(min)
        , m_max(max)
        , m_computed(false)
        , m_isAuto(false)
        , m_fixed(true)
        , m_newLine(false)
        , m_type(GridLineType::MinMax)
        , m_containing(false)
    {
        if (min.isLength() && min.length().isFixed()) {
            m_offset = min.length().numberData();
        } else if (min.isAuto()) {
            m_offset = 0;
        }
    }

    GridTrack(GridLineType type)
        : m_type(type)
    {
    }

private:
    LayoutUnit m_offset; // width
    LayoutUnit m_fr;
    GridLength m_min;
    GridLength m_max;
    bool m_computed{ false };
    bool m_isAuto{ false };
    bool m_fixed{ true };
    bool m_newLine{ false };
    GridLineType m_type;
    bool m_containing{ false };
};

#define GRID_MAX_TRACK 50
class GridFormattingContext {
public:
    enum ConvertType { ROWSTART, COLUMNSTART, ROWEND, COLUMNEND };

    GridFormattingContext(LayoutContext& ctx, FrameGridBox* container,
                          LayoutUnit availableWidth);

    void computeColumnsAndRows();
    void applyFrUnitsWithColumns();
    void applyFrUnitsWithRows();
    void buildGridLineTemplate();
    void layoutGridItems();
    void alignGridLinesForColumns(GridArea&, LayoutUnit&, LayoutUnit&, bool);
    void alignGridLinesForRows(GridArea&);
    void assumeGridItemWidths();
    void resolveIntrinsicTrackSizes();
    void stretchAutoTracks();
    void applyAlignItems();
    void layoutGridLinesWithGridAreas();
    void relayoutGridLinesWithGridAreasIfNeeded();

    bool needsGridItemLayout(FrameBox* gridItem, ComputedStyle* style,
                             bool testWidthOnly);

    bool fixGridAreaWithDefine(GridArea*, size_t);
    bool fixGridAreaWithUndefine(GridArea**, GridArea*, size_t);
    void parseGridTemplateAreas();
    void buildGridAreaAndOrdering();
    void placeGridItemsIntoCells();

    size_t convertToRealLine(String*, size_t, ConvertType);
    void convertToStartEndForRow(ComputedStyle*, size_t&, size_t&);
    void convertToStartEndForColumn(ComputedStyle*, size_t&, size_t&);

    void initializeGridLineColumns(const GCVector<GridTrackSize>* columns);
    void initializeGridLineRows(const GCVector<GridTrackSize>* rows);
    void applyMinMaxGridLineColumns();

    LayoutUnit preferredWidth();

    GCVector<GridTrack>& gridTemplateColumns()
    {
        return m_gridTemplateColumns;
    }

    GCVector<GridTrack>& gridTemplateRows()
    {
        return m_gridTemplateRows;
    }

    LayoutContext& layoutContext()
    {
        return m_layoutContext;
    }

    bool existColumnTemplate();
    bool existRowTemplate();

    GridArea* getNamedGridArea(String* name);

    static bool doesParticipateInGridFormattingContext(Frame* GridItem);

private:
    LayoutContext& m_layoutContext;
    FrameGridBox* m_container;
    LayoutUnit m_availableWidth;
    GCVector<GridTrack> m_gridTemplateColumns;
    GCVector<GridTrack> m_gridTemplateRows;
    GCVector<FrameBox*> m_orderedGridItems;
    GCVector<GridArea> m_orderedGridArea;
    GCUnorderedMultiMap<std::string, GridArea> m_namedAreaMap;

    LayoutUnit m_rowGap;
    LayoutUnit m_columnGap;
    // FIXME(#1286): This checker is poor.
    bool m_areaChecker[GRID_MAX_TRACK][GRID_MAX_TRACK];

    void parseGridRowAndColumnValues(GridArea* gridArea);
    void resolveDefinitePositionValues(GridArea* gridArea);
    GridLineValue* parseGridLineValue(String* gridLineValue);
    void placeGridAreasWithDefinitePositions(
        GCVector<GridArea*>& gridAreaDefinite);
    bool expandGridLineRows(GridArea* gridArea);
    bool expandGridLineColumns(GridArea* gridArea);

    void placeGridAreasLockedToRows(GCVector<GridArea*>& gridAreasAuto);
    void placeRemainingGridAreas(GCVector<GridArea*>& gridAreasAuto);
    bool hasAvailableGridCells(GridArea* gridArea, size_t row, size_t col);
    void placeGridArea(GridArea* gridArea);

    void applyImplicitTrackSizing();
    void resolveMinMaxContentSize(size_t gridTrackIndex);
    void applyAlignItemsCenter();
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
    }

    bool m_hasFixedStyleWidth{ false };
    bool m_hasFixedStyleHeight{ false };
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
}
#endif
