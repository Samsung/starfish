/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishFrameInputBox__
#define __StarfishFrameInputBox__

#include "core/layout/FrameBlockBox.h"

namespace Starfish {

class FrameTreeBuilderContext;
class ComputedStyle;

class FrameInputBox final : public FrameBlockBox {
public:
    FrameInputBox(Node* node, ComputedStyle* style);
    static FrameInputBox* buildFrameTree(Node* current,
                                         FrameTreeBuilderContext& ctx,
                                         bool force);

    virtual const char* name() override
    {
        return "FrameInputBox";
    }

    virtual bool isFrameInputBox() override
    {
        return true;
    }

    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat) override;
    virtual void paintBackgroundAndBorders(Canvas* canvas) override;
    virtual void paintContent(PaintingContext& ctx) override;

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        FrameBlockBox::fillGCDescriptor(desc);
    }

    void paintCaret(Canvas* canvas);
    virtual void paintInlineContentBlock(Canvas* canvas) override;
    static ComputedStyle* createInputElementStyleFrom(Node* parent);

private:
    FrameText* firstFrameTextChild();
};
} // namespace Starfish

#endif
