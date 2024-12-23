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

#ifndef __StarfishFrameSVGSVGBox__
#define __StarfishFrameSVGSVGBox__

#include "core/layout/FrameReplaced.h"
#include "core/layout/svg/FrameSVGBox.h"

namespace Starfish {

class FrameSVGSVGBox final : public FrameReplaced {
public:
    FrameSVGSVGBox(Node* node)
        : FrameReplaced(node, nullptr)
        , m_isInnerSVG(false)
        , m_svgMaskPaintingDepth(0)
        , m_svgScale(1)
        , m_defaultWidth(300)
        , m_defaultHeight(150)
    {
    }

    virtual bool isFrameSVGSVGBox() override
    {
        return true;
    }

    virtual bool needsSVGGeometryAttributes() override
    {
        return true;
    }

    virtual const char* name() override
    {
        return "FrameSVGSVGBox";
    }

    virtual void computeStyleFlags() override
    {
        FrameReplaced::computeStyleFlags();
        m_flags.m_shouldApplyOverflow = true;
    }

    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat) override;
    virtual IntrinsicSize intrinsicSize() override;
    virtual void paintReplaced(Canvas* canvas) override;
    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual LayoutRect overflowRepaintRect() override;

    LayoutSize viewport()
    {
        return m_viewport;
    }

    std::pair<bool, SkMatrix> computeTranlateScaleOnPaint();

    // https://svgwg.org/svg2-draft/coords.html#Units
    // For any other length value expressed as a percentage of the SVG viewport, the percentage must be calculated as a percentage of the normalized diagonal of the ‘viewBox’ applied to that viewport. If no ‘viewBox’ is specified, then the normalized diagonal of the SVG viewport must be used.
    // The normalized diagonal length must be calculated with sqrt((width)**2 + (height)**2)/sqrt(2).
    LayoutUnit normalizedDiagonalViewportLength()
    {
        float w = m_viewport.width();
        float h = m_viewport.height();
        return sqrt(w * w + h * h) / sqrt(2);
    }

    Optional<Unit::Rect> viewBox()
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

    void setContainerViewport(Optional<Unit::Rect> containerViewport)
    {
        m_containerViewport = containerViewport;
    }

    void setDefaultWidth(size_t width)
    {
        m_defaultWidth = width;
    }

    void setDefaultHeight(size_t height)
    {
        m_defaultHeight = height;
    }

    const SkMatrix& svgPaintingMatrix()
    {
        return m_svgPaintingMatrix;
    }

    size_t& svgMaskPaintingDepth()
    {
        return m_svgMaskPaintingDepth;
    }

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        FrameReplaced::fillGCDescriptor(desc);
    }

    bool m_isInnerSVG;
    size_t m_svgMaskPaintingDepth;
    LayoutSize m_viewport;
    Optional<Unit::Rect> m_viewBox;
    float m_svgScale;
    size_t m_defaultWidth;
    size_t m_defaultHeight;
    Optional<Unit::Rect> m_containerViewport;
    SkMatrix m_svgPaintingMatrix;
};
} // namespace Starfish

#endif
