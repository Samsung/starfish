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

class GridFormattingContext {
public:
    GridFormattingContext(LayoutContext& ctx, FrameGridBox* container,
                          LayoutUnit availableWidth);
    void computeColumnsAndRows();

    static bool doesParticipateInGridFormattingContext(Frame* GridItem);

private:
    LayoutContext& m_layoutContext;
    FrameGridBox* m_container;
    LayoutUnit m_availableWidth;
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
};
}
#endif
