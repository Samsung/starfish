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
#include "FrameSVGSVGBox.h"
#include "core/dom/svg/SVGSVGElement.h"

namespace Starfish {

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

IntrinsicSize FrameSVGSVGBox::intrinsicSize()
{
    IntrinsicSize result;
    result.m_isContentExists = true;
    result.m_hasAspectRatio = false;
    result.m_hasViewport = false;

    LayoutUnit width = STARFISH_DEFAULT_SVG_WIDTH;
    LayoutUnit height = STARFISH_DEFAULT_SVG_HEIGHT;

    if (style()->width().isFixed() && style()->height().isFixed()) {
        width = style()->width().fixed();
        height = style()->height().fixed();
        result.m_hasAspectRatio = true;
        result.m_hasViewport = true;
    } else if (style()->width().isFixed()) {
        width = style()->width().fixed();
        height = style()->width().fixed();
        result.m_hasAspectRatio = true;
        result.m_hasViewport = true;
    } else if (style()->height().isFixed()) {
        width = style()->height().fixed();
        height = style()->height().fixed();
        result.m_hasAspectRatio = true;
        result.m_hasViewport = true;
    } else {
        if (node()->asSVGSVGElement()->hasViewBox()) {
            Unit::Rect viewBox = node()->asSVGSVGElement()->viewBox();
            height = STARFISH_DEFAULT_SVG_HEIGHT;
            width = STARFISH_DEFAULT_SVG_HEIGHT * viewBox.width() /
                    viewBox.height();
            result.m_hasAspectRatio = true;
        }
    }

    result.m_intrinsicContentSize = LayoutSize(width, height);
    return result;
}

void FrameSVGSVGBox::layout(LayoutContext& ctx,
                            Frame::LayoutWantToResolve resolveWhat)
{
    FrameReplaced::layout(ctx, resolveWhat);

    if (resolveWhat & Frame::LayoutWantToResolve::ResolveHeight) {
        LayoutUnit orgWidth = contentWidth();
        LayoutUnit orgHeight = contentHeight();

        m_svgScale = 1;
        m_viewBox = Nullable<Unit::Rect>();

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
                m_viewBox = Nullable<Unit::Rect>(rt);
            }
        }

        if (isInnerSVG()) {
            FrameBox* cb = layoutParent()->asFrameBox();
            auto styleX = style()->x();
            auto styleY = style()->y();
            float x = 0, y = 0;
            if (styleX.isSpecified() && styleY.isSpecified()) {
                x = styleX.specifiedValue(cb->width(), this);
                y = styleY.specifiedValue(cb->height(), this);
            } else if (styleX.isSpecified() && !styleY.isSpecified()) {
                x = styleX.specifiedValue(cb->width(), this);
            } else if (!styleX.isSpecified() && styleY.isSpecified()) {
                y = styleY.specifiedValue(cb->height(), this);
            }
            setX(x);
            setY(y);
        }

        Frame* f = firstChild();
        while (f) {
            if (f->isFrameSVGSVGBox()) {
                FrameSVGSVGBox* svg = (FrameSVGSVGBox*)f;
                f->layout(ctx, Frame::LayoutWantToResolve::ResolveAll);
            } else {
                f->asFrameSVGBox()->resolvePosition(ctx);
                f->asFrameSVGBox()->moveX(borderLeft() + paddingLeft());
                f->asFrameSVGBox()->moveY(borderTop() + paddingTop());
                f->layout(ctx, Frame::LayoutWantToResolve::ResolveAll);
            }

            f = f->next();
        }

        setContentWidth(orgWidth);
        setContentHeight(orgHeight);
    }
}

void FrameSVGSVGBox::paintReplaced(Canvas* canvas)
{
    canvas->setNeedsGoodQualityAntialias();
    canvas->save();

    LayoutUnit svgWidth = contentWidth();
    LayoutUnit svgHeight = contentHeight();
    IntrinsicSize intrinsicSizeInfo = intrinsicSize();

    Unit::Rect viewport;
    if (m_containerViewport.hasValue()) {
        viewport = m_containerViewport.value();
        svgWidth = intrinsicSizeInfo.m_intrinsicContentSize.width();
        svgHeight = intrinsicSizeInfo.m_intrinsicContentSize.height();
    } else {
        viewport.setWidth(contentWidth());
        viewport.setHeight(contentHeight());
    }

    canvas->translate(borderLeft() + paddingLeft(), borderTop() + paddingTop());
    canvas->clip(Unit::Rect(0, 0, viewport.width(), viewport.height()));

    double sxToViewport = viewport.width() / svgWidth;
    double syToViewport = viewport.height() / svgHeight;
    double sToViewport = 1;
    if (sxToViewport == 0 || syToViewport == 0 ||
        std::isnan(sxToViewport == 0) || std::isnan(syToViewport == 0)) {
        canvas->restore();
        return;
    }

    bool hasViewBox = node()->asSVGSVGElement()->hasViewBox();
    if (hasViewBox) {
        sToViewport = std::min(sxToViewport, syToViewport);
        canvas->scale(sToViewport, sToViewport);
    } else {
        if (intrinsicSizeInfo.m_hasViewport) {
            canvas->scale(sxToViewport, syToViewport);
        }
    }

    if (hasViewBox) {
        Unit::Rect viewBox = node()->asSVGSVGElement()->viewBox();
        double sx = svgWidth / viewBox.width();
        double sy = svgHeight / viewBox.height();
        double sToContentSize = std::min(sx, sy);

        if (sToContentSize == 0 || std::isnan(sToContentSize)) {
            canvas->restore();
            return;
        }

        canvas->scale(sToContentSize, sToContentSize);

        double tx = viewBox.x();
        double ty = viewBox.y();
        if (std::isnan(tx) || std::isnan(ty)) {
            canvas->restore();
            return;
        }

        canvas->translate(-tx, -ty);

        double dx = 0;
        double dy = 0;
        if (intrinsicSizeInfo.m_hasViewport) {
            dx = (svgWidth - viewBox.width() * sToContentSize) / 2;
            dy = (svgHeight - viewBox.height() * sToContentSize) / 2;
        } else {
            // scale to viewport directly
            dx = (viewport.width() - svgWidth * sToViewport) / 2;
            dy = (viewport.height() - svgHeight * sToViewport) / 2;
        }

        if (dx < 0) {
            dx = 0;
        }
        if (dy < 0) {
            dy = 0;
        }

        canvas->translate(dx / (sToContentSize * sToViewport),
                          dy / (sToContentSize * sToViewport));
    }

    PaintingContext ctx(canvas);
    Frame* child = firstChild();
    while (child) {
        ctx.m_canvas->save();
        ctx.m_canvas->translate(
            child->asFrameBox()->x() - borderLeft() - paddingLeft(),
            child->asFrameBox()->y() - borderTop() - paddingTop());
        if (child->isFrameSVGSVGBox()) {
            FrameSVGSVGBox* svg = (FrameSVGSVGBox*)child;
            svg->paintReplaced(canvas);
        } else {
            child->asFrameSVGBox()->paintContent(ctx);
        }
        ctx.m_canvas->restore();
        child = child->next();
    }

    canvas->restore();
    canvas->setNeedsFastAntialias();
}
}
