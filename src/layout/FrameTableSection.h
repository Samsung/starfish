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

#ifndef __StarFishFrameTableSection__
#define __StarFishFrameTableSection__

#include "layout/FrameBlockBox.h"

namespace StarFish {

class FrameTreeBuilderContext;
class FrameTable;
class FrameTableRow;
class FrameTableCell;
class ColStruct;

struct CellStruct {
    CellStruct() : cell(nullptr) {}
    CellStruct(FrameTableCell* cell_) : cell(cell_) {}

    FrameTableCell* cell;
};

struct RowStruct {
    RowStruct() : tableRow(nullptr) {}
    RowStruct(FrameTableRow* tableRow);

    FrameTableRow* tableRow;
    GCVector<CellStruct> cells;
};

class FrameTableSection : public FrameBlockBox {
public:
    FrameTableSection(Node* node, ComputedStyle* style);

    static FrameTableSection* buildFrameTableSection(Node* sectionNode,
                                                     FrameTreeBuilderContext& ctx,
                                                     bool force = false);
    void calContentWidth(LayoutContext& ctx);
    void layoutWidth(LayoutContext& ctx);
    void layoutHeight(LayoutContext& ctx);

    FrameTableRow* addChild(Node* child, FrameTreeBuilderContext& ctx, bool force);

    virtual const char* name()
    {
        return "FrameTableSection";
    }

    virtual bool isFrameTableSection()
    {
        return true;
    }

    virtual bool hasBlockFlow()
    {
        // TODO: Fix it after finishing table context properly
        // FrameTableSection always contains blockflow
        Frame* child = firstChild();
        if (!child) {
            STARFISH_ASSERT(child->isNormalFlow());
            // Only TableRow can exist as children of TableSection
            STARFISH_ASSERT(child->isFrameTableRow());
        }

        return true;
    }

    FrameTable* table()
    {
        return parent()->asFrameTable();
    }

    GCVector<RowStruct>& grid()
    {
        return m_grid;
    }

    GCVector<ColStruct>& columnWidths()
    {
        return m_columnWidths;
    }

private:
    void layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat);

    // represent the logical table structure
    GCVector<RowStruct> m_grid;
    GCVector<ColStruct> m_columnWidths;
};

}

#endif
