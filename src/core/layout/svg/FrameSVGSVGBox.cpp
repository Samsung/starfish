/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
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
    result.m_hasAspectRatio = true;

    LayoutUnit width = STARFISH_DEFAULT_SVG_WIDTH;
    LayoutUnit height = STARFISH_DEFAULT_SVG_HEIGHT;

    if (style()->width().isFixed() && style()->height().isFixed()) {
        width = style()->width().fixed();
        height = style()->height().fixed();
    } else if (style()->width().isFixed()) {
        width = style()->width().fixed();
        height = style()->width().fixed();
    } else if (style()->height().isFixed()) {
        width = style()->height().fixed();
        height = style()->height().fixed();
    } else {
        if (node()->asSVGSVGElement()->hasViewBox()) {
            width = STARFISH_DEFAULT_SVG_WIDTH;
            height = STARFISH_DEFAULT_SVG_WIDTH *
                     node()->asSVGSVGElement()->viewBox().width() /
                     node()->asSVGSVGElement()->viewBox().height();
        }
    }

    result.m_intrinsicContentSize = LayoutSize(width, height);
    return result;
}

void FrameSVGSVGBox::paintReplaced(Canvas* canvas)
{
#if defined(PORT_GRAPHIC_BACKEND_EFL)
    Canvas* outerCanvas = canvas;
    outerCanvas->save();
    outerCanvas->translate(borderLeft() + paddingLeft(),
                           borderTop() + paddingTop());

    if (!m_surface || (float)contentWidth() != m_surface->width() ||
        (float)contentHeight() != m_surface->height()) {
        m_surface =
            ImageData::create((float)contentWidth(), (float)contentHeight());
    }
    m_surface->clear();

    canvas = Canvas::createGenericCanvas(m_surface);

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
        child->asFrameSVGBox()->paint(ctx);
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
        float tx = rt.x();
        float ty = rt.y();
        canvas->scale(sx, sy);
        canvas->translate(-tx, -ty);
    }

    PaintingContext ctx(canvas);
    Frame* child = firstChild();
    while (child) {
        ctx.m_canvas->save();
        ctx.m_canvas->translate(
            child->asFrameBox()->x() - borderLeft() - paddingLeft(),
            child->asFrameBox()->y() - borderTop() - paddingTop());
        child->asFrameSVGBox()->paintSVG(ctx);
        ctx.m_canvas->restore();
        child = child->next();
    }

    canvas->restore();
#endif
}
}
