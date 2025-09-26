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

#ifndef __StarfishFrameSVGBox__
#define __StarfishFrameSVGBox__

#include "core/layout/FrameBox.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/image/NativeImageData.h"
#include "core/modules/canvas/Path.h"

namespace Starfish {

class FrameSVGSVGBox;

class FrameSVGBox : public FrameBox {
public:
    FrameSVGBox(Node* node)
        : FrameBox(node, nullptr)
    {
        computeStyleFlags();
    }

    virtual bool isFrameSVGBox() override
    {
        return true;
    }

    virtual void computeStyleFlags() override
    {
        m_flags.m_needToEstablishBlockFormattingContext = false;
        m_flags.m_needToEstablishStackingContext = false;
        m_flags.m_needsGraphicsBuffer = false;
    }

    virtual bool needsSVGGeometryAttributes() override;
    virtual bool isAlwaysInvisible() override;

    virtual const char* name() override
    {
        return "FrameSVGBox";
    }

    FrameSVGSVGBox* outmostSVGViewportBox();
    LayoutSize viewport();
    LayoutUnit normalizedDiagonalViewportLength();

    Optional<LayoutUnit> resolveStyleLength(const Length& length,
                                            const LayoutUnit& viewportLength);
    LayoutLocation resolveStylePosition(const LayoutSize& viewport);
    LayoutSize resolveStyleSize(const LayoutSize& viewport);

    // util function for FrameSVGBox and FrameSVGSVGBox
    static LayoutLocation resolveStylePosition(FrameBox* box,
                                               const LayoutSize& viewport);

    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat) override;
    struct SVGLayoutContext {
        LayoutContext& layoutContext;
        LayoutSize viewport;
        LayoutUnit normalizedDiagonalViewportLength;
        std::vector<LayoutRect>& clippedRects;
        std::map<void*, LayoutRect> m_fillRects;
    };
    virtual void layout(SVGLayoutContext& ctx, SkMatrix matrix);

    virtual void layoutSVG(SVGLayoutContext& ctx)
    {
    }

    virtual void layoutChildren(SVGLayoutContext& ctx, SkMatrix matrix);

    virtual void postLayoutSVG(SVGLayoutContext& ctx)
    {
    }

    virtual bool prepareChildPainting(Canvas* canvas)
    {
        return true;
    }

    virtual void paintContent(PaintingContext& ctx) override;
    virtual void paintSVG(PaintingContext& ctx);
    bool applyTransformTo(Canvas* canvas, const LayoutSize& vp);

    virtual Optional<Path*> path()
    {
        return nullptr;
    }

    virtual LayoutRect boundingRect();

    virtual bool isVisible() override
    {
        return true;
    }

    Optional<SkMatrix*> computedSVGTransform()
    {
        return m_computedSVGTransform;
    }

    LayoutRect* unadjustedFrameRectByFilter()
    {
        return m_unadjustedFrameRectByFilter.value();
    }

    Optional<CanvasFillStrokeSource*> makeCanvasFillStrokeSource(
        const AtomicString& id, const Unit::Rect& rect);

    static std::vector<std::pair<double, double>> parsePointsFromString(
        String* str);
    static float computeSVGLength(SVGLength* length, float fullValue,
                                  bool isObjectBoundingBoxMode);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        FrameBox::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(FrameSVGBox, m_treeItemModel.m_parent));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(FrameSVGBox, m_treeItemModel.m_previous));
        GC_set_bit(desc, GC_WORD_OFFSET(FrameSVGBox, m_treeItemModel.m_next));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(FrameSVGBox, m_treeItemModel.m_firstChild));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(FrameSVGBox, m_treeItemModel.m_lastChild));
        GC_set_bit(desc, GC_WORD_OFFSET(FrameSVGBox, m_computedSVGTransform));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(FrameSVGBox, m_unadjustedFrameRectByFilter));
    }

    virtual bool hasFrameTreeItemModel() override
    {
        return true;
    }

    virtual FrameTreeItemModel* frameTreeItemModel() override
    {
        return &m_treeItemModel;
    }

    Optional<GradientDrawingInfo*> makeGradientDrawingInfo(
        String* url, const Unit::Rect& rect);

    FrameTreeItemModel m_treeItemModel;
    Optional<SkMatrix*> m_computedSVGTransform;
    Optional<LayoutRect*> m_unadjustedFrameRectByFilter;
};
} // namespace Starfish

#endif
