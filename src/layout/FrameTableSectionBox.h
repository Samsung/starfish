/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#ifndef __StarFishFrameTableSectionBox__
#define __StarFishFrameTableSectionBox__

#include "layout/FrameTableObjectBox.h"

namespace StarFish {

class FrameTreeBuilderContext;
class FrameTableBox;
class FrameTableRowBox;
class FrameTableCellBox;
class ColSizeStruct;

class CellStruct {
public:
    CellStruct()
        : m_cell(nullptr)
    {
    }

    CellStruct(FrameTableCellBox* cell, unsigned id)
        : m_cell(cell)
        , m_id(id)
    {
    }

    FrameTableCellBox* cell()
    {
        return m_cell;
    }

    unsigned id()
    {
        return m_id;
    }

private:
    FrameTableCellBox* m_cell;
    unsigned m_id; // logical Id
};

class RowStruct {
public:
    RowStruct()
        : m_tableRow(nullptr)
    {
    }
    RowStruct(FrameTableRowBox* tableRow);

    CellStruct* lastCell()
    {
        return m_cells.size() > 0 ? &m_cells[m_cells.size() - 1] : nullptr;
    }

    unsigned logicalColumnSize();
    CellStruct* logicalCellStructAt(size_t id);

    FrameTableCellBox* logicalCellAt(size_t id)
    {
        CellStruct* cell = logicalCellStructAt(id);
        if (cell) {
            return cell->cell();
        }

        return nullptr;
    }

    FrameTableRowBox* tableRow()
    {
        return m_tableRow;
    }

    GCVector<CellStruct>& cells()
    {
        return m_cells;
    }

private:
    FrameTableRowBox* m_tableRow;
    GCVector<CellStruct> m_cells;
};

class FrameTableSectionBox : public FrameTableObjectBox {
public:
    FrameTableSectionBox(Node* node, ComputedStyle* style);

    static FrameTableSectionBox* buildFrameTableSectionBox(
        Node* current, FrameTreeBuilderContext& ctx, bool force = false);
    static FrameTableSectionBox* createAnonymousWithParent(
        FrameBlockBox* parent, Node* node);

    void calCellWidth(LayoutContext& ctx);
    void calCellWidthsWithColspans();
    void layoutWidth(LayoutContext& ctx);
    void layoutHeight(LayoutContext& ctx);

    void increaseRowHeightBy(LayoutUnit rowHeightOffset);
    void applyVerticalAlign();

    virtual void paintBackgroundAndBorders(Canvas* canvas);

    void addChild(Node* child, FrameTreeBuilderContext& ctx, bool force);

    virtual const char* name()
    {
        return "FrameTableSection";
    }

    virtual bool isFrameTableSectionBox()
    {
        return true;
    }

    virtual bool hasBlockFlow()
    {
        // FrameTableSection always contains blockflow
        return true;
    }

    FrameTableBox* tableBox()
    {
        return parent()->asFrameTableBox();
    }

    GCVector<RowStruct>& grid()
    {
        return m_grid;
    }

    GCVector<ColSizeStruct>& columnWidths()
    {
        return m_columnWidths;
    }

private:
    void layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat);
    // represent the logical table structure
    GCVector<RowStruct> m_grid;
    GCVector<ColSizeStruct> m_columnWidths;
};
}

#endif
