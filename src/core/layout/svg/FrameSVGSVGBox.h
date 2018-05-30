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

#ifndef __StarFishFrameSVGSVGBox__
#define __StarFishFrameSVGSVGBox__

#include "core/layout/FrameReplaced.h"
#include "core/layout/svg/FrameSVGBox.h"

namespace StarFish {

class FrameSVGSVGBox final : public FrameReplaced {
public:
    FrameSVGSVGBox(Node* node)
        : FrameReplaced(node, nullptr)
        , m_svgScale(1)
        , m_surface(nullptr)
        , m_isInnerSVG(false)
    {
    }

    virtual bool isFrameSVGSVGBox() override
    {
        return true;
    }

    virtual const char* name() override
    {
        return "FrameSVGSVGBox";
    }

    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat) override;
    virtual IntrinsicSize intrinsicSize() override;
    virtual void paintReplaced(Canvas* canvas) override;
    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    Nullable<Unit::Rect> viewBox()
    {
        return m_viewBox;
    }

    float svgScale()
    {
        return m_svgScale;
    }

    bool isInnerSVG()
    {
        return m_isInnerSVG;
    }

    void setInnerSVG(bool v)
    {
        m_isInnerSVG = v;
    }

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        FrameReplaced::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(FrameSVGSVGBox, m_surface));
    }

    Nullable<Unit::Rect> m_viewBox;
    float m_svgScale;
    NativeImageData* m_surface;
    bool m_isInnerSVG;
};
}

#endif
