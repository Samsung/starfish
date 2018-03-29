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

#ifndef __StarFishFrameGridBox__
#define __StarFishFrameGridBox__

#include "core/layout/FrameBlockBox.h"

namespace StarFish {

class ComputedStyle;
class FrameBox;
class FrameGridBox;
class LineBox;

class GridArea : public gc {
public:
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
    FrameBox* m_box;
    size_t m_index;
    size_t m_rowStart;
    size_t m_rowEnd;
    size_t m_columnStart;
    size_t m_columnEnd;
};

class GridLine : public gc {
public:
    LayoutUnit offset()
    {
        return m_offset;
    }

    LayoutUnit fr()
    {
        return m_fr;
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

    void setComputed(bool computed)
    {
        m_computed = computed;
    }

    void setNewLine(bool newLine)
    {
        m_newLine = newLine;
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

    bool isFr()
    {
        return m_state == Fr;
    }

    // The 'computed' is for the 'fr' unit.
    GridLine(LayoutUnit fr, bool computed)
        : m_offset(0)
        , m_fr(fr)
        , m_lineName(nullptr)
        , m_computed(computed)
        , m_fixed(true)
        , m_newLine(false)
        , m_state(Fr)
    {
    }

    GridLine(LayoutUnit offset)
        : m_offset(offset)
        , m_fr(0)
        , m_lineName(nullptr)
        , m_computed(true)
        , m_fixed(true)
        , m_newLine(false)
        , m_state(Fixed)
    {
    }

private:
    LayoutUnit m_offset;
    LayoutUnit m_fr;
    String* m_lineName;
    bool m_computed;
    bool m_fixed;
    bool m_newLine;
    enum GridLineState {
        Fixed,
        Fr,
    };
    GridLineState m_state;
};

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
    }

    FrameBox* m_box;
    Length m_width;
    Length m_height;
    Length m_minWidth;
    Length m_maxWidth;
    Length m_minHeight;
    Length m_maxHeight;
};

#define GRID_MAX_TRACK 50
class GridFormattingContext {
public:
    GridFormattingContext(LayoutContext& ctx, FrameGridBox* container,
                          LayoutUnit availableWidth);

    void computeColumnsAndRows();
    void applyFrUnitsWithColumns();
    void applyFrUnitsWithRows();
    void buildGridLineTemplate();
    void layoutGridItems();
    void arrangeGridLinesWithGridAreas(bool);
    void arrangeGridColumnLine();

    bool fixGridAreaWithDefine(GridArea*, size_t);
    bool fixGridAreaWithUndefine(GridArea**, GridArea*, size_t);
    void buildGridAreaAndOrdering();
    LayoutUnit preferredWidth();

    GCVector<GridLine>& gridLineColumns()
    {
        return m_gridLineColumns;
    }

    LayoutContext& layoutContext()
    {
        return m_layoutContext;
    }

    bool existColumnTemplate();

    static bool doesParticipateInGridFormattingContext(Frame* GridItem);

private:
    LayoutContext& m_layoutContext;
    FrameGridBox* m_container;
    LayoutUnit m_availableWidth;
    GCVector<GridLine> m_gridLineColumns;
    GCVector<GridLine> m_gridLineRows;
    GCVector<FrameBox*> m_orderedGridItems;
    GCVector<GridArea> m_orderedGridArea;
    // FIXME(#1286): This checker is poor.
    bool m_areaChecker[GRID_MAX_TRACK][GRID_MAX_TRACK];
};

class FrameGridBox : public FrameBlockBox {
public:
    FrameGridBox(Node* node, ComputedStyle* style);

    virtual const char* name()
    {
        return "FrameGridBox";
    }

    virtual bool isFrameGridBox()
    {
        return true;
    }

    virtual bool hasBlockFlow()
    {
        return true;
    }

    void layoutGrid(LayoutContext& ctx);
    void computePreferredWidth(PreferredWidthContext& ctx);
};
}
#endif
