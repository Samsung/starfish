/*
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishFrameViewportContextBox__
#define __StarfishFrameViewportContextBox__

#include "core/layout/svg/FrameSVGBox.h"
#include "core/layout/svg/FrameSVGSVGBox.h"

namespace Starfish {

class FrameSVGViewportContextBox final : public FrameSVGBox {
public:
    FrameSVGViewportContextBox(Node* node)
        : FrameSVGBox(node)
    {
    }

    virtual bool isFrameSVGViewportContextBox() override
    {
        return true;
    }

    virtual const char* name() override
    {
        return "FrameSVGViewportContextBox";
    }

    LayoutSize viewport() const
    {
        return m_viewport;
    }

    LayoutUnit normalizedDiagonalViewportLength()
    {
        float w = m_viewport.width();
        float h = m_viewport.height();
        return sqrt(w * w + h * h) / sqrt(2);
    }

    Unit::Rect viewBox();
    std::pair<bool, SkMatrix> computeTranlateScaleOnPaint();
    virtual void layoutChildren(SVGLayoutContext& ctx,
                                SkMatrix matrix) override;
    virtual bool prepareChildPainting(Canvas* canvas) override;
    virtual Optional<Path*> path() override;

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        FrameSVGBox::fillGCDescriptor(desc);
    }

    LayoutSize m_viewport;
};
} // namespace Starfish

#endif
