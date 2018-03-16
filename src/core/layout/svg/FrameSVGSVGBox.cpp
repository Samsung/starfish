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

#include "StarFishConfig.h"
#include "FrameSVGSVGBox.h"
#include "StarFish.h"
#include "core/dom/svg/SVGSVGElement.h"

namespace StarFish {

void* FrameSVGSVGBox::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FrameSVGSVGBox)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameSVGSVGBox, m_surface));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameSVGSVGBox, m_node));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameSVGSVGBox, m_layoutParent));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameSVGSVGBox, m_surface));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameSVGSVGBox, m_treeItemModel.m_parent));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameSVGSVGBox, m_treeItemModel.m_previous));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameSVGSVGBox, m_treeItemModel.m_next));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameSVGSVGBox,
                                              m_treeItemModel.m_firstChild));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameSVGSVGBox, m_treeItemModel.m_lastChild));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FrameSVGSVGBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

IntrinsicSize FrameSVGSVGBox::intrinsicSize()
{
    IntrinsicSize result;
    result.m_isContentExists = true;
    result.m_hasAspectRatio = false;

    LayoutUnit width = STARFISH_DEFAULT_SVG_WIDTH;
    LayoutUnit height = STARFISH_DEFAULT_SVG_HEIGHT;

    if (style()->width().isFixed() && style()->height().isFixed()) {
        width = style()->width().fixed();
        height = style()->height().fixed();
        result.m_hasAspectRatio = true;
    } else if (style()->width().isFixed()) {
        width = style()->width().fixed();
        height = style()->width().fixed();
        result.m_hasAspectRatio = true;
    } else if (style()->height().isFixed()) {
        width = style()->height().fixed();
        height = style()->height().fixed();
        result.m_hasAspectRatio = true;
    } else {
        if (node()->asSVGSVGElement()->hasViewBox()) {
            width = STARFISH_DEFAULT_SVG_WIDTH;
            height = STARFISH_DEFAULT_SVG_WIDTH *
                     node()->asSVGSVGElement()->viewBox().height() /
                     node()->asSVGSVGElement()->viewBox().width();
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
            float sx = rt.width() / contentWidth();
            float sy = rt.height() / contentHeight();
            float s = std::min(sx, sy);
            if (s == 0 || std::isnan(s)) {
            } else {
                setWidth(s * contentWidth());
                setHeight(s * contentHeight());
                m_svgScale = s;
                m_viewBox = Nullable<Unit::Rect>(rt);
            }
        }

        Frame* f = firstChild();
        while (f) {
            f->asFrameSVGBox()->resolvePosition(ctx);
            f->asFrameSVGBox()->moveX(borderLeft() + paddingLeft());
            f->asFrameSVGBox()->moveY(borderTop() + paddingTop());
            f->layout(ctx, Frame::LayoutWantToResolve::ResolveAll);

            f = f->next();
        }

        setContentWidth(orgWidth);
        setContentHeight(orgHeight);
    }
}

void FrameSVGSVGBox::paintReplaced(Canvas* canvas)
{
    canvas->setNeedsGoodQualityAntialias();
#if defined(PORT_CANVAS_BACKEND_EFL)
    Canvas* outerCanvas = canvas;
    outerCanvas->save();
    outerCanvas->translate(borderLeft() + paddingLeft(),
                           borderTop() + paddingTop());

    if (!m_surface || (float)contentWidth() != m_surface->width() ||
        (float)contentHeight() != m_surface->height()) {
        m_surface = NativeImageData::create((float)contentWidth(),
                                            (float)contentHeight());
    }
    m_surface->clear();

    canvas =
        Canvas::createGenericCanvas(node()->starFish(), m_surface->data(),
                                    m_surface->width(), m_surface->height());

    if (node()->asSVGSVGElement()->hasViewBox()) {
        Unit::Rect rt = node()->asSVGSVGElement()->viewBox();
        float sx = contentWidth() / rt.width();
        float sy = contentHeight() / rt.height();
        float s = std::min(sx, sy);
        float tx = rt.x();
        float ty = rt.y();
        canvas->scale(s, s);
        canvas->translate(-tx, -ty);
    }

    PaintingContext ctx(canvas);
    Frame* child = firstChild();
    while (child) {
        ctx.m_canvas->save();
        ctx.m_canvas->translate(
            child->asFrameBox()->x() - borderLeft() - paddingLeft(),
            child->asFrameBox()->y() - borderTop() - paddingTop());
        child->asFrameSVGBox()->paintContent(ctx);
        ctx.m_canvas->restore();
        child = child->next();
    }
    delete canvas;

    outerCanvas->drawImage(m_surface,
                           Unit::Rect(0, 0, contentWidth(), contentHeight()));
    outerCanvas->restore();
#else
    canvas->save();
    canvas->translate(borderLeft() + paddingLeft(), borderTop() + paddingTop());
    canvas->clip(Unit::Rect(0, 0, contentWidth(), contentHeight()));

    if (node()->asSVGSVGElement()->hasViewBox()) {
        Unit::Rect rt = node()->asSVGSVGElement()->viewBox();
        float sx = contentWidth() / rt.width();
        float sy = contentHeight() / rt.height();
        float s = std::min(sx, sy);
        if (s == 0 || std::isnan(s)) {
            canvas->restore();
            return;
        }
        float tx = rt.x();
        float ty = rt.y();
        if (std::isnan(tx) || std::isnan(ty)) {
            canvas->restore();
            return;
        }
        canvas->scale(s, s);
        canvas->translate(-tx, -ty);

        tx = contentWidth() - s * rt.width();
        if (tx > 0) {
            tx = contentWidth() / 2 - (s * rt.width()) / 2;
        } else {
            tx = 0;
        }

        ty = contentHeight() - s * rt.height();
        if (ty > 0) {
            ty = contentHeight() / 2 - (s * rt.height()) / 2;
        } else {
            ty = 0;
        }

        canvas->translate(tx / s, ty / s);
    }

    PaintingContext ctx(canvas);
    Frame* child = firstChild();
    while (child) {
        ctx.m_canvas->save();
        ctx.m_canvas->translate(
            child->asFrameBox()->x() - borderLeft() - paddingLeft(),
            child->asFrameBox()->y() - borderTop() - paddingTop());
        child->asFrameSVGBox()->paintContent(ctx);
        ctx.m_canvas->restore();
        child = child->next();
    }

    canvas->restore();
#endif
    canvas->setNeedsFastAntialias();
}
}
