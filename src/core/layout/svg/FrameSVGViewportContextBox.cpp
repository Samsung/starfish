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

#include "StarfishConfig.h"
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Element.h"
#include "core/dom/svg/SVGSVGElement.h"
#include "FrameSVGViewportContextBox.h"
#include "FrameSVGSVGBox.h"
#include "Starfish.h"

namespace Starfish {

void* FrameSVGViewportContextBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameSVGViewportContextBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameSVGViewportContextBox)] = { 0 };
        FrameSVGViewportContextBox::fillGCDescriptor(desc);
        descr =
            GC_make_descriptor(desc, GC_WORD_LEN(FrameSVGViewportContextBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

Optional<Path*> FrameSVGViewportContextBox::path()
{
    Optional<Path*> path = nullptr;
    Frame* child = firstChild();

    while (child) {
        if (child && child->isFrameSVGBox()) {
            FrameSVGBox* childBox = child->asFrameSVGBox();
            auto childPath = childBox->path();
            if (childPath) {
                if (path) {
                    path->append(childPath.value());
                } else {
                    path = childPath;
                }
            }
        }
        child = child->next();
    }
    return path;
}

std::pair<int, SkMatrix>
FrameSVGViewportContextBox::computeTranlateScaleOnPaint()
{
    auto nearestViewport = FrameSVGBox::viewport();
    auto intrinsicSize =
        FrameSVGSVGBox::intrinsicSize(node()->asSVGElement(), nearestViewport);

    LayoutSize viewport = nearestViewport;
    LayoutSize svgSize = nearestViewport;
    if (intrinsicSize.m_hasViewport) {
        svgSize = intrinsicSize.m_intrinsicContentSize;
    }

    auto cs = FrameSVGSVGBox::computeTranlateScaleOnPaint(
        node()->asSVGElement(), svgSize, viewport, intrinsicSize);

    if (!intrinsicSize.m_hasViewport && node()->asSVGElement()->hasViewBox() &&
        cs.first < 2) {
        Unit::Rect viewBox = node()->asSVGElement()->viewBox();
        float dx =
            (viewport.width() - viewBox.width() * cs.second.getScaleX()) / 2;
        float dy =
            (viewport.height() - viewBox.height() * cs.second.getScaleY()) / 2;
        cs.second.postTranslate(dx, dy);
    }

    return cs;
}

bool FrameSVGViewportContextBox::prepareChildPainting(Canvas* canvas)
{
    auto stylePos = resolveStylePosition(viewport());
    canvas->translate(stylePos.x(), stylePos.y());

    auto cs = computeTranlateScaleOnPaint();

    if (!cs.first) {
        return false;
    }
    canvas->postMatrix(cs.second);
    return true;
}

void FrameSVGViewportContextBox::layoutChildren(SVGLayoutContext& ctx,
                                                SkMatrix matrix)
{
    SVGLayoutContext childCtx = { ctx.layoutContext, viewport(),
                                  normalizedDiagonalViewportLength(),
                                  ctx.clippedRects, ctx.m_fillRects };

    m_viewport = ctx.viewport;
    if (node()->asSVGElement()->hasViewBox()) {
        auto e = node()->asSVGElement();
        m_viewport = LayoutSize(e->viewBox().width(), e->viewBox().height());
    }

    auto stylePos = resolveStylePosition(ctx.viewport);
    matrix.postTranslate(stylePos.x(), stylePos.y());

    auto cs = computeTranlateScaleOnPaint();
    if (!cs.first) {
        return;
    }
    matrix.postConcat(cs.second);
    Frame* f = firstChild();
    while (f) {
        if (f->isFrameSVGBox()) {
            f->asFrameSVGBox()->layout(childCtx, matrix);
        } else {
            f->layout(childCtx.layoutContext,
                      Frame::LayoutWantToResolve::ResolveAll);
        }
        f = f->next();
    }
}

} // namespace Starfish
