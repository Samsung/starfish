/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishFrameRangeBox__
#define __StarfishFrameRangeBox__

#include "core/layout/FrameInputBox.h"

namespace Starfish {

// A range control draws a track and a thumb instead of a text value, so it
// takes none of FrameInputBox's caret or line box handling.
//
// It stays a block box rather than becoming a replaced one: a range control is
// semi-replaced, meaning it has a default size but still stretches when a flex
// container or a pair of opposing insets ask it to, which a replaced box would
// not do.
// https://html.spec.whatwg.org/multipage/input.html#range-state-(type=range)
class FrameRangeBox final : public FrameInputBox {
public:
    FrameRangeBox(Node* node, ComputedStyle* style)
        : FrameInputBox(node, style)
    {
    }

    virtual const char* name() override
    {
        return "FrameRangeBox";
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
        FrameInputBox::fillGCDescriptor(desc);
    }

public:
    virtual void paintInlineContentBlock(Canvas* canvas,
                                         PaintPassMemos* memos) override;

private:
    void paintSlider(Canvas* canvas);
};
} // namespace Starfish

#endif
