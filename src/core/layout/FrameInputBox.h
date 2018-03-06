/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishFrameInputBox__
#define __StarFishFrameInputBox__

#include "core/layout/FrameBlockBox.h"

namespace StarFish {

class FrameTreeBuilderContext;
class ComputedStyle;

class FrameInputBox : public FrameBlockBox {
public:
    FrameInputBox(Node* node, ComputedStyle* style);
    static FrameInputBox* buildFrameTree(Node* current,
                                         FrameTreeBuilderContext& ctx,
                                         bool force);

    virtual const char* name()
    {
        return "FrameInputBox";
    }

    virtual bool isFrameInputBox()
    {
        return true;
    }

    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat) override;
    virtual void paintContent(PaintingContext& ctx) override;

    void layoutForTextEditable(LayoutContext& ctx,
                               Frame::LayoutWantToResolve resolveWhat);

protected:
    void paintCaret(Canvas* canvas);
    virtual void paintInlineContent(Canvas* canvas) override;
    static ComputedStyle* createInputElementStyleFrom(Node* parent);
};
}

#endif
