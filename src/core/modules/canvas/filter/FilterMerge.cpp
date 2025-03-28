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

#include "core/dom/svg/SVGFEMergeElement.h"
#include "core/dom/svg/SVGFEMergeNodeElement.h"
#include "core/dom/svg/SVGAnimatedString.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/filter/FilterMerge.h"

namespace Starfish {
FilterMerge::FilterMerge(Filter* filter,
                         SVGFilterPrimitiveStandardAttributes* element)
    : FilterPrimitive(
          filter, element,
          String::emptyString, // this filter don't in parameter itself
          element->output()->baseVal())
{
    STARFISH_ASSERT(filter);
    STARFISH_ASSERT(element->isSVGFEMergeElement());

    auto e = element->firstElementChild();
    while (e) {
        if (e->isSVGFEMergeNodeElement()) {
            String* s = e->asSVGFEMergeNodeElement()->in()->baseVal();
            m_inputs.push_back(s);
        }
        e = e->nextElementSibling();
    }
}

void* FilterMerge::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FilterMerge));
    static bool typeInited = false;
    static GC_descr descr;
    if (typeInited == false) {
        GC_word desc[GC_BITMAP_SIZE(FilterMerge)] = { 0 };
        FilterPrimitive::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(FilterMerge, m_inputs));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FilterMerge));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

bool FilterMerge::shouldMaintainSourceBuffer(bool isFirstFilter)
{
    for (size_t i = 0; i < m_inputs.size(); i++) {
        if (m_inputs[i]->equals("SourceGraphic")) {
            return true;
        }
    }
    return false;
}

void FilterMerge::apply(const Unit::Rect& subRegionInFloat,
                        Filter::FilterApplyContext& ctx)
{
    bool isSubRegionInFloat = subRegionCoversAll(subRegionInFloat);
    auto outputSource = filter()->fetchOutputSource(
        ctx, this, ctx.sourceGraphic(), subRegionInFloat);

    if (m_inputs.size() == 0) {
        // we should clear buffer there is no input
        memset(outputSource->data(), 0, outputSource->size());
    } else {
        Canvas* c = Canvas::create(outputSource->data(), ctx.width, ctx.height,
                                   ctx.stride, 1);

        if (!subRegionCoversAll(subRegionInFloat)) {
            c->clip(Unit::Rect(ctx.width * subRegionInFloat.x(),
                               ctx.height * subRegionInFloat.y(),
                               ctx.width * subRegionInFloat.width(),
                               ctx.height * subRegionInFloat.height()));
        }

        bool first = true;
        for (auto* input : m_inputs) {
            auto inputSource = ctx.findSource(input);
            if (!inputSource) {
                STARFISH_ASSERT(ctx.output);
                inputSource = ctx.output;
            }
            if (first) {
                c->setCompositeOperator(CanvasCompositeOperator::Copy,
                                        BlendMode::Normal);
                first = false;
            } else {
                c->setCompositeOperator(CanvasCompositeOperator::SourceOver,
                                        BlendMode::Normal);
            }
            c->drawImage(inputSource->data(), ctx.width, ctx.stride, ctx.height,
                         Unit::Rect(0, 0, ctx.width, ctx.height));
        }
        delete c;
    }

    filter()->registerOutput(ctx, this, outputSource);
}

} // namespace Starfish
