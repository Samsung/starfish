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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Node.h"
#include "core/dom/svg/SVGSVGElement.h"
#include "core/page/WebView.h"
#include "FrameSVGSVGBox.h"

namespace Starfish {

static SkMatrix SVGPreserveAspectRatioValueMatrix(
    float logicalX, float logicalY, float logicalWidth, float logicalHeight,
    float physicalWidth, float physicalHeight,
    NativeImageData::PreserveAspectRatioAlign m_align,
    NativeImageData::PreserveAspectRatioMeetOrSlice m_meetOrSlice)
{
    SkMatrix transform = SkMatrix::I();
    if (!logicalWidth || !logicalHeight || !physicalWidth || !physicalHeight) {
        return transform;
    }

    if (m_align == NativeImageData::PreserveAspectRatioAlign::None) {
        return transform;
    }

    double extendedLogicalX = logicalX;
    double extendedLogicalY = logicalY;
    double extendedLogicalWidth = logicalWidth;
    double extendedLogicalHeight = logicalHeight;
    double extendedPhysicalWidth = physicalWidth;
    double extendedPhysicalHeight = physicalHeight;
    double logicalRatio = extendedLogicalWidth / extendedLogicalHeight;
    double physicalRatio = extendedPhysicalWidth / extendedPhysicalHeight;

    if (m_align == NativeImageData::None) {
        transform.preScale(extendedPhysicalWidth / extendedLogicalWidth,
                           extendedPhysicalHeight / extendedLogicalHeight);
        transform.preTranslate(-extendedLogicalX, -extendedLogicalY);
        return transform;
    }

    if ((logicalRatio < physicalRatio &&
         (m_meetOrSlice ==
          NativeImageData::PreserveAspectRatioMeetOrSlice::Meet)) ||
        (logicalRatio >= physicalRatio &&
         (m_meetOrSlice ==
          NativeImageData::PreserveAspectRatioMeetOrSlice::Slice))) {
        transform.preScale(extendedPhysicalHeight / extendedLogicalHeight,
                           extendedPhysicalHeight / extendedLogicalHeight);

        if (m_align == NativeImageData::PreserveAspectRatioAlign::xMinYMin ||
            m_align == NativeImageData::PreserveAspectRatioAlign::xMinYMid ||
            m_align == NativeImageData::PreserveAspectRatioAlign::xMinYMax) {
            transform.preTranslate(-extendedLogicalX, -extendedLogicalY);
        } else if (m_align ==
                       NativeImageData::PreserveAspectRatioAlign::xMidYMin ||
                   m_align ==
                       NativeImageData::PreserveAspectRatioAlign::xMidYMid ||
                   m_align ==
                       NativeImageData::PreserveAspectRatioAlign::xMidYMax) {
            transform.preTranslate(
                -extendedLogicalX -
                    (extendedLogicalWidth - extendedPhysicalWidth *
                                                extendedLogicalHeight /
                                                extendedPhysicalHeight) /
                        2,
                -extendedLogicalY);

        } else {
            transform.preTranslate(
                -extendedLogicalX -
                    (extendedLogicalWidth - extendedPhysicalWidth *
                                                extendedLogicalHeight /
                                                extendedPhysicalHeight),
                -extendedLogicalY);
        }
        return transform;
    }

    transform.preScale(extendedPhysicalWidth / extendedLogicalWidth,
                       extendedPhysicalWidth / extendedLogicalWidth);

    if (m_align == NativeImageData::PreserveAspectRatioAlign::xMinYMin ||
        m_align == NativeImageData::PreserveAspectRatioAlign::xMidYMin ||
        m_align == NativeImageData::PreserveAspectRatioAlign::xMaxYMin) {
        transform.preTranslate(-extendedLogicalX, -extendedLogicalY);

    } else if (m_align == NativeImageData::PreserveAspectRatioAlign::xMinYMid ||
               m_align == NativeImageData::PreserveAspectRatioAlign::xMidYMid ||
               m_align == NativeImageData::PreserveAspectRatioAlign::xMaxYMid) {
        transform.preTranslate(
            -extendedLogicalX,
            -extendedLogicalY - (extendedLogicalHeight -
                                 extendedPhysicalHeight * extendedLogicalWidth /
                                     extendedPhysicalWidth) /
                                    2);

    } else {
        transform.preTranslate(
            -extendedLogicalX,
            -extendedLogicalY - (extendedLogicalHeight -
                                 extendedPhysicalHeight * extendedLogicalWidth /
                                     extendedPhysicalWidth));
    }
    return transform;
}

void* FrameSVGSVGBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameSVGSVGBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameSVGSVGBox)] = { 0 };
        FrameSVGSVGBox::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameSVGSVGBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

IntrinsicSize FrameSVGSVGBox::intrinsicSize(SVGElement* element,
                                            LayoutSize defaultSize)
{
    IntrinsicSize result;
    result.m_isContentExists = true;
    result.m_hasAspectRatio = false;
    result.m_hasViewport = false;

    LayoutUnit width = defaultSize.width();
    LayoutUnit height = defaultSize.height();
    auto style = element->style();

    if (style->width().isFixed() && style->height().isFixed()) {
        width = style->width().fixed();
        height = style->height().fixed();
        result.m_hasAspectRatio = true;
        result.m_hasViewport = true;
    } else if (style->width().isFixed()) {
        width = style->width().fixed();
        if (element->hasViewBox()) {
            Unit::Rect viewBox = element->viewBox();
            height = width / viewBox.width() * viewBox.height();
        } else {
            height = width;
        }
        result.m_hasAspectRatio = true;
        result.m_hasViewport = true;
    } else if (style->height().isFixed()) {
        height = style->height().fixed();
        if (element->hasViewBox()) {
            Unit::Rect viewBox = element->viewBox();
            width = height * viewBox.width() / viewBox.height();
        } else {
            width = height;
        }
        result.m_hasAspectRatio = true;
        result.m_hasViewport = true;
    } else {
        String* widthString = element->getAttributeOrEmpty(
            element->starfish()->staticStrings()->m_width);
        String* heightString = element->getAttributeOrEmpty(
            element->starfish()->staticStrings()->m_height);
        if ((widthString && !widthString->isEmpty()) ||
            (heightString && !heightString->isEmpty())) {
            result.m_hasViewport = true;
        }

        if (element->hasViewBox()) {
            Unit::Rect viewBox = element->viewBox();
            height = defaultSize.height();
            width = defaultSize.height() * viewBox.width() / viewBox.height();
            result.m_hasAspectRatio = true;
        }
    }

    result.m_intrinsicContentSize = LayoutSize(width, height);
    return result;
}

IntrinsicSize FrameSVGSVGBox::intrinsicSize()
{
    return intrinsicSize(node()->asSVGElement(),
                         LayoutSize(m_defaultWidth, m_defaultHeight));
}

void FrameSVGSVGBox::layout(LayoutContext& ctx,
                            Frame::LayoutWantToResolve resolveWhat)
{
    FrameReplaced::layout(ctx, resolveWhat);

    if (resolveWhat & Frame::LayoutWantToResolve::ResolveHeight) {
        LayoutUnit orgWidth = contentWidth();
        LayoutUnit orgHeight = contentHeight();

        m_svgScale = 1;
        m_viewBox = Optional<Unit::Rect>();

        if (node()->asSVGSVGElement()->hasViewBox()) {
            Unit::Rect rt = node()->asSVGSVGElement()->viewBox();
            float sx = contentWidth() ? rt.width() / contentWidth()
                                      : std::numeric_limits<float>::quiet_NaN();
            float sy = contentHeight()
                           ? rt.height() / contentHeight()
                           : std::numeric_limits<float>::quiet_NaN();
            float s = std::min(sx, sy);
            if (s == 0 || std::isnan(s)) {
            } else {
                setWidth(s * contentWidth());
                setHeight(s * contentHeight());
                m_svgScale = s;
                m_viewBox = Optional<Unit::Rect>(rt);
            }
        }

        // compute viewport
        if (m_viewBox.hasValue()) {
            m_viewport.setWidth(m_viewBox.value().width());
            m_viewport.setHeight(m_viewBox.value().height());
        } else {
            m_viewport.setWidth(contentWidth().toFloat());
            m_viewport.setHeight(contentHeight().toFloat());
        }

        setContentWidth(orgWidth);
        setContentHeight(orgHeight);

        std::vector<LayoutRect> clippedRects;
        std::map<void*, LayoutRect> fillRects;
        FrameSVGBox::SVGLayoutContext svgLayoutContext = {
            ctx, m_viewport, normalizedDiagonalViewportLength(), clippedRects,
            fillRects
        };

        SkMatrix matrix = SkMatrix::I();
        matrix.postConcat(computeTranlateScaleOnPaint().second);

        Frame* f = firstChild();
        while (f) {
            f->asFrameSVGBox()->layout(svgLayoutContext, matrix);
            f->asFrameSVGBox()->moveX(borderLeft() + paddingLeft());
            f->asFrameSVGBox()->moveY(borderTop() + paddingTop());
            f = f->next();
        }
    }
}

LayoutRect FrameSVGSVGBox::overflowRepaintRect()
{
    LayoutRect rt(borderLeft() + paddingLeft(), borderTop() + paddingTop(),
                  contentWidth(), contentHeight());
    SkMatrix matrix = computeTranlateScaleOnPaint().second;

    rt.setX(rt.x() + matrix.getTranslateX());
    rt.setY(rt.y() + matrix.getTranslateY());

    rt.setWidth(rt.width() - matrix.getTranslateX() * 2);
    rt.setHeight(rt.height() - matrix.getTranslateY() * 2);

    return rt;
}

std::pair<int, SkMatrix> FrameSVGSVGBox::computeTranlateScaleOnPaint(
    SVGElement* element, const LayoutSize& svgSize, const LayoutSize& viewport,
    const IntrinsicSize& intrinsicSize)
{
    SkMatrix result = SkMatrix::I();

    LayoutUnit svgWidth = svgSize.width();
    LayoutUnit svgHeight = svgSize.height();

    double sxToViewport = viewport.width().toFloat() / svgWidth;
    double syToViewport = viewport.height().toFloat() / svgHeight;

    double sToViewport = 1;
    if (sxToViewport == 0 || syToViewport == 0 || std::isnan(sxToViewport) ||
        std::isnan(syToViewport)) {
        return std::make_pair(0, result);
    }

    bool hasViewBox = element->hasViewBox();
    NativeImageData::PreserveAspectRatioAlign svgAlign = NativeImageData::None;
    if (hasViewBox) {
        sToViewport = std::min(sxToViewport, syToViewport);
        svgAlign = element->preserveAspectRatioAlign();
        if (svgAlign == NativeImageData::None) {
            result.preScale(sxToViewport, syToViewport);
        } else {
            // TODO: Should consider preserveAspectRatio meetOrSlice.
            result.preScale(sToViewport, sToViewport);
        }
    } else {
        if (intrinsicSize.m_hasViewport) {
            result.preScale(sxToViewport, syToViewport);
        }
    }

    if (hasViewBox) {
        Unit::Rect viewBox = element->viewBox();
        double sx = svgWidth / viewBox.width();
        double sy = svgHeight / viewBox.height();

        double sToContentSize = std::min(sx, sy);

        if (sToContentSize == 0 || std::isnan(sToContentSize)) {
            return std::make_pair(0, result);
        }

        if (svgAlign == NativeImageData::None) {
            result.preScale(sx, sy);
        } else {
            result.preScale(sToContentSize, sToContentSize);
        }

        float tx = viewBox.x();
        float ty = viewBox.y();
        if (std::isnan(tx) || std::isnan(ty)) {
            return std::make_pair(0, result);
        }

        result.preTranslate(-tx, -ty);

        float dx = 0;
        float dy = 0;
        if (intrinsicSize.m_hasViewport) {
            if (svgAlign == NativeImageData::None) {
                dx = (svgWidth - viewBox.width() * sx) / 2;
                dy = (svgHeight - viewBox.height() * sy) / 2;
            } else {
                dx = (svgWidth - viewBox.width() * sToContentSize) / 2;
                dy = (svgHeight - viewBox.height() * sToContentSize) / 2;
            }
        } else {
            if (svgAlign == NativeImageData::None) {
                dx = (viewport.width().toFloat() - svgWidth * sx) / 2;
                dy = (viewport.height().toFloat() - svgHeight * sy) / 2;
            } else {
                auto preserveAspectMatrix = SVGPreserveAspectRatioValueMatrix(
                    viewBox.x(), viewBox.y(), viewBox.width(), viewBox.height(),
                    viewport.width().toFloat(), viewport.height().toFloat(),
                    svgAlign, element->preserveAspectRatioMeetOrSlice());
                return std::make_pair(2, preserveAspectMatrix);
            }
        }

        if (dx < 0) {
            dx = 0;
        }
        if (dy < 0) {
            dy = 0;
        }

        if (svgAlign == NativeImageData::None) {
            result.preTranslate(dx / (sx * sxToViewport),
                                dy / (sy * syToViewport));
        } else {
            // TODO: Should consider preserveAspectRatio alignment.
            result.preTranslate(dx / (sToContentSize * sToViewport),
                                dy / (sToContentSize * sToViewport));
        }
    }
    return std::make_pair(1, result);
}

std::pair<int, SkMatrix> FrameSVGSVGBox::computeTranlateScaleOnPaint()
{
    LayoutUnit svgWidth = contentWidth();
    LayoutUnit svgHeight = contentHeight();
    IntrinsicSize intrinsicSizeInfo = intrinsicSize();
    SkMatrix result = SkMatrix::I();
    if (svgWidth == 0 || svgHeight == 0) {
        return std::make_pair(0, result);
    }

    LayoutSize viewport;
    if (m_containerViewport.hasValue() &&
        !m_containerViewport.value().isEmpty()) {
        viewport = LayoutSize(m_containerViewport.value().width(),
                              m_containerViewport.value().height());
        svgWidth = intrinsicSizeInfo.m_intrinsicContentSize.width();
        svgHeight = intrinsicSizeInfo.m_intrinsicContentSize.height();
    } else {
        viewport.setWidth(contentWidth());
        viewport.setHeight(contentHeight());
    }

    return computeTranlateScaleOnPaint(node()->asSVGElement(),
                                       LayoutSize(svgWidth, svgHeight),
                                       viewport, intrinsicSizeInfo);
}

LayoutRect FrameSVGSVGBox::topmostMaskPaintingRect()
{
    STARFISH_ASSERT(svgMaskPaintingDepth());
    return (*svgMaskPaintingStack().begin())->absoluteRect(this);
}

LayoutRect FrameSVGSVGBox::computeCanvasLayerRect(FrameSVGBox* box)
{
    // in mask painting
    // mask rect and sub-mask or sub-content rect may not overlapped
    if (svgMaskPaintingDepth()) {
        return topmostMaskPaintingRect();
    } else {
        return box->absoluteRect(this);
    }
}

void FrameSVGSVGBox::paintReplaced(Canvas* canvas)
{
    auto tranlateScaleValue = computeTranlateScaleOnPaint();
    if (!tranlateScaleValue.first) {
        return;
    }

    canvas->setNeedsGoodQualityAntialias();
    canvas->save();

    canvas->translate(borderLeft() + paddingLeft(), borderTop() + paddingTop());

    m_svgPaintingMatrix = canvas->currentTransformMatrix();

    if (!m_containerViewport) {
        bool hasBiggerViewBoxThenContentArea = false;
        if (m_viewBox) {
            hasBiggerViewBoxThenContentArea =
                contentWidth() < m_viewBox.value().width() ||
                contentHeight() < m_viewBox.value().height();
        }
        if (hasBiggerViewBoxThenContentArea) {
            canvas->clip(Unit::Rect(0, 0, contentWidth(), contentHeight()));
        }
        canvas->postMatrix(tranlateScaleValue.second);
        auto vp = viewport();

        if (!hasBiggerViewBoxThenContentArea) {
            canvas->clip(Unit::Rect(0, 0, vp.width(), vp.height()));
        }
    } else {
        canvas->postMatrix(tranlateScaleValue.second);

        m_svgPaintingMatrix.preConcat(tranlateScaleValue.second);
        m_svgPaintingMatrix.preScale(m_svgScale, m_svgScale);
    }

    PaintingContext ctx(canvas);
    Frame* child = firstChild();
    while (child) {
        ctx.m_canvas->save();
        child->asFrameSVGBox()->paintContent(ctx);
        ctx.m_canvas->restore();
        child = child->next();
    }

    canvas->restore();
    canvas->setNeedsFastAntialias();
}
} // namespace Starfish
