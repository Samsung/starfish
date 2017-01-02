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

#ifndef __StarFishFrameTableCell__
#define __StarFishFrameTableCell__

#include "layout/FrameBlockBox.h"

namespace StarFish {

class FrameTreeBuilderContext;

class FrameTableCell : public FrameBlockBox {
public:
    FrameTableCell(Node* node, ComputedStyle* style);

    static FrameTableCell* buildFrameTableCell(Node* cellNode,
                                               FrameTreeBuilderContext& ctx,
                                               bool force = false);
    virtual void layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat);

    virtual const char* name()
    {
        return "FrameTableCell";
    }

    virtual bool isFrameTableCell()
    {
        return true;
    }

    void setAbsoluteColumnIndex(unsigned column)
    {
        m_absoluteColumnIndex = column;
    }

    unsigned absoluteColumnIndex()
    {
        return m_absoluteColumnIndex;
    }

private:
    LayoutUnit minimumCellWidth(LayoutContext& ctx);
    LayoutUnit maximumCellWidth(LayoutContext& ctx);

    unsigned m_absoluteColumnIndex;
};

}

#endif
