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

#include "core/dom/svg/SVGFEOffsetElement.h"
#include "core/modules/canvas/filter/Filter.h"
#include "core/modules/canvas/filter/FilterOffset.h"
#include "core/page/WebView.h"
#include "core/layout/svg/FrameSVGBox.h"
#include "core/layout/svg/FrameSVGSVGBox.h"

namespace Starfish {

FilterOffset::FilterOffset(Filter* filter,
                           SVGFilterPrimitiveStandardAttributes* element)
    : FilterPrimitive(filter, element,
                      element->asSVGFEOffsetElement()->in1()->baseVal(),
                      element->output()->baseVal())
{
    STARFISH_ASSERT(filter);
    STARFISH_ASSERT(element->isSVGFEOffsetElement());
}

void* FilterOffset::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FilterOffset));
    static bool typeInited = false;
    static GC_descr descr;
    if (typeInited == false) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FilterOffset)] = { 0 };
        STARFISH_ASSERT(obj_bitmap != nullptr);
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FilterOffset));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

Filter::FilterBias FilterOffset::computeBias(
    const LayoutSize& targetSize, const Unit::Rect& candidateFilterFrameRect,
    const std::pair<float, float>& viewportScale)
{
    auto e = element()->asSVGFEOffsetElement();
    float dx = filter()->resolveFilterPrimitiveValue(
        e->dx()->animVal(), targetSize.width(), viewportScale.first);
    float dy = filter()->resolveFilterPrimitiveValue(
        e->dy()->animVal(), targetSize.height(), viewportScale.second);

    auto newRt = candidateFilterFrameRect;

    newRt.setX(dx + newRt.x());
    newRt.setY(dy + newRt.y());

    return Filter::FilterBias(NullOption, newRt);
}

void FilterOffset::apply(const Unit::Rect& subRegionInFloat,
                         Filter::FilterApplyContext& ctx)
{
    STARFISH_ASSERT(element()->isSVGFEOffsetElement());
    auto e = element()->asSVGFEOffsetElement();

    auto vm = ctx.target->outmostSVGViewportBox()
                  ->computeTranlateScaleOnPaint()
                  .second;
    auto targetSize = ctx.target->unadjustedFrameRectByFilter()->size();
    float dx = filter()->resolveFilterPrimitiveValue(
        e->dx()->animVal(), targetSize.width(), vm.getScaleX());
    float dy = filter()->resolveFilterPrimitiveValue(
        e->dy()->animVal(), targetSize.height(), vm.getScaleY());

    float dpr = element()->webView()->screenInfo().devicePixelRatio;
    dx *= dpr;
    dy *= dpr;

    std::shared_ptr<Filter::FilterSourceBuffer> inputSource =
        filter()->fetchInputSource(ctx, this);
    std::shared_ptr<Filter::FilterSourceBuffer> outputSource(
        new Filter::FilterSourceBuffer(ctx.src, ctx.stride * ctx.height, true));

    Canvas* c = Canvas::create(outputSource->data(), ctx.width, ctx.height,
                               ctx.stride, 1);
    c->clearColor(Unit::Color(0, 0, 0, 0));
    if (!subRegionCoversAll(subRegionInFloat)) {
        c->clip(Unit::Rect(ctx.width * subRegionInFloat.x(),
                           ctx.height * subRegionInFloat.y(),
                           ctx.width * subRegionInFloat.width(),
                           ctx.height * subRegionInFloat.height()));
    }

    c->drawImage(inputSource->data(), ctx.width, ctx.stride, ctx.height,
                 Unit::Rect(dx, dy, ctx.width, ctx.height));
    delete c;

    filter()->registerOutput(ctx, this, outputSource);
}

} // namespace Starfish
