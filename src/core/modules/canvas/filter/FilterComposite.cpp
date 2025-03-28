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

#include <math.h>
#include "core/dom/svg/SVGFECompositeElement.h"
#include "core/dom/svg/SVGAnimatedNumberList.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/filter/Filter.h"
#include "core/modules/canvas/filter/FilterComposite.h"

namespace Starfish {

void compositeSource(Filter::FilterApplyContext& ctx,
                     const Unit::Rect& subRegionInFloat,
                     std::shared_ptr<Filter::FilterSourceBuffer> input1,
                     std::shared_ptr<Filter::FilterSourceBuffer> input2,
                     std::shared_ptr<Filter::FilterSourceBuffer> output,
                     SVGFECompositeElement::CompositeOperator oper)
{
    Canvas* c =
        Canvas::create(output->data(), ctx.width, ctx.height, ctx.stride, 1);

    if (!FilterPrimitive::subRegionCoversAll(subRegionInFloat)) {
        c->clearColor(Unit::Color(0, 0, 0, 0));
        c->clip(Unit::Rect(ctx.width * subRegionInFloat.x(),
                           ctx.height * subRegionInFloat.y(),
                           ctx.width * subRegionInFloat.width(),
                           ctx.height * subRegionInFloat.height()));
    }

    c->setCompositeOperator(CanvasCompositeOperator::Copy, BlendMode::Normal);

    switch (oper) {
    case SVGFECompositeElement::CompositeOperator::
        SVG_FECOMPOSITE_OPERATOR_OVER:
        c->drawImage(input2->data(), ctx.width, ctx.stride, ctx.height,
                     Unit::Rect(0, 0, ctx.width, ctx.height));
        c->setCompositeOperator(CanvasCompositeOperator::SourceOver,
                                BlendMode::Normal);
        c->drawImage(input1->data(), ctx.width, ctx.stride, ctx.height,
                     Unit::Rect(0, 0, ctx.width, ctx.height));
        break;

    case SVGFECompositeElement::CompositeOperator::SVG_FECOMPOSITE_OPERATOR_IN:
        // TODO : Applies only to the intersected region.

        c->drawImage(input2->data(), ctx.width, ctx.stride, ctx.height,
                     Unit::Rect(0, 0, ctx.width, ctx.height));
        c->setCompositeOperator(CanvasCompositeOperator::SourceIn,
                                BlendMode::Normal);
        c->drawImage(input1->data(), ctx.width, ctx.stride, ctx.height,
                     Unit::Rect(0, 0, ctx.width, ctx.height));
        break;

    case SVGFECompositeElement::CompositeOperator::SVG_FECOMPOSITE_OPERATOR_OUT:
        c->drawImage(input1->data(), ctx.width, ctx.stride, ctx.height,
                     Unit::Rect(0, 0, ctx.width, ctx.height));
        c->setCompositeOperator(CanvasCompositeOperator::DestinationOut,
                                BlendMode::Normal);
        c->drawImage(input2->data(), ctx.width, ctx.stride, ctx.height,
                     Unit::Rect(0, 0, ctx.width, ctx.height));
        break;

    case SVGFECompositeElement::CompositeOperator::
        SVG_FECOMPOSITE_OPERATOR_ATOP:
        c->drawImage(input2->data(), ctx.width, ctx.stride, ctx.height,
                     Unit::Rect(0, 0, ctx.width, ctx.height));
        c->setCompositeOperator(CanvasCompositeOperator::SourceAtop,
                                BlendMode::Normal);
        c->drawImage(input1->data(), ctx.width, ctx.stride, ctx.height,
                     Unit::Rect(0, 0, ctx.width, ctx.height));
        break;

    case SVGFECompositeElement::CompositeOperator::SVG_FECOMPOSITE_OPERATOR_XOR:
        c->drawImage(input2->data(), ctx.width, ctx.stride, ctx.height,
                     Unit::Rect(0, 0, ctx.width, ctx.height));
        c->setCompositeOperator(CanvasCompositeOperator::XOR,
                                BlendMode::Normal);
        c->drawImage(input1->data(), ctx.width, ctx.stride, ctx.height,
                     Unit::Rect(0, 0, ctx.width, ctx.height));
        break;

    case SVGFECompositeElement::CompositeOperator::
        SVG_FECOMPOSITE_OPERATOR_ARITHMETIC:
        STARFISH_UNSUPPORTED();
        break;

    case SVGFECompositeElement::CompositeOperator::
        SVG_FECOMPOSITE_OPERATOR_LIGHTER:
        c->drawImage(input2->data(), ctx.width, ctx.stride, ctx.height,
                     Unit::Rect(0, 0, ctx.width, ctx.height));
        c->setCompositeOperator(CanvasCompositeOperator::PlusLighter,
                                BlendMode::Normal);
        c->drawImage(input1->data(), ctx.width, ctx.stride, ctx.height,
                     Unit::Rect(0, 0, ctx.width, ctx.height));
        break;

    default:
        STARFISH_UNSUPPORTED();
    }
    delete c;
}

FilterComposite::FilterComposite(Filter* filter,
                                 SVGFilterPrimitiveStandardAttributes* element)
    : FilterPrimitive(filter, element,
                      element->asSVGFECompositeElement()->in1()->baseVal(),
                      element->asSVGFECompositeElement()->in2()->baseVal(),
                      element->output()->baseVal())
{
    STARFISH_ASSERT(filter);
    STARFISH_ASSERT(element->isSVGFECompositeElement());
}

void* FilterComposite::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FilterComposite));
    static bool typeInited = false;
    static GC_descr descr;
    if (typeInited == false) {
        GC_word desc[GC_BITMAP_SIZE(FilterComposite)] = { 0 };
        FilterPrimitive::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FilterComposite));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FilterComposite::apply(const Unit::Rect& subRegionInFloat,
                            Filter::FilterApplyContext& ctx)
{
    STARFISH_ASSERT(element()->isSVGFECompositeElement());
    SVGFECompositeElement* ele = element()->asSVGFECompositeElement();
    auto inputSource = filter()->fetchInputSource(ctx, this);
    auto inputSource2 = filter()->fetchInputSource2(ctx, this);
    SVGFECompositeElement::CompositeOperator oper =
        (SVGFECompositeElement::CompositeOperator)ele->domOperator()->baseVal();

    std::shared_ptr<Filter::FilterSourceBuffer> outputSource(
        new Filter::FilterSourceBuffer(ctx.src, ctx.stride * ctx.height, true));

    compositeSource(ctx, subRegionInFloat, inputSource, inputSource2,
                    outputSource, oper);

    filter()->registerOutput(ctx, this, outputSource);
}

} // namespace Starfish
