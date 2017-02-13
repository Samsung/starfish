/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

struct CellStruct {
    CellStruct()
        : cell(nullptr)
    {
    }

    CellStruct(FrameTableCellBox* cell_)
        : cell(cell_)
    {
    }

    FrameTableCellBox* cell;
};

struct RowStruct {
    RowStruct() : tableRow(nullptr)
    {
    }
    RowStruct(FrameTableRowBox* tableRow);

    FrameTableRowBox* tableRow;
    GCVector<CellStruct> cells;
};

class FrameTableSectionBox : public FrameTableObjectBox {
public:
    FrameTableSectionBox(Node* node, ComputedStyle* style);

    static FrameTableSectionBox* buildFrameTableSectionBox(
        Node* current, FrameTreeBuilderContext& ctx, bool force = false);
    static FrameTableSectionBox* createAnonymousWithParent(
        FrameBlockBox* parent, Node* node);

    void calCellWidth(LayoutContext& ctx);
    void layoutWidth(LayoutContext& ctx);
    void layoutHeight(LayoutContext& ctx);

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
    bool isFirstTableSection();
    // represent the logical table structure
    GCVector<RowStruct> m_grid;
    GCVector<ColSizeStruct> m_columnWidths;
};
}

#endif
