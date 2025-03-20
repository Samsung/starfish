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
#include "Starfish.h"

#include "core/dom/svg/SVGFEFloodElement.h"
#include "core/modules/canvas/filter/Filter.h"
#include "core/modules/canvas/filter/FilterFlood.h"
#include "core/modules/canvas/Canvas.h"
#include "core/style/CSSParser.h"

namespace Starfish {

FilterFlood::FilterFlood(Filter* filter,
                         SVGFilterPrimitiveStandardAttributes* element)
    : FilterPrimitive(filter, element, String::emptyString,
                      element->output()->baseVal())
    , m_floodColor(Unit::Color(0, 0, 0, 255))
{
    STARFISH_ASSERT(filter);
    STARFISH_ASSERT(element->isSVGFEFloodElement());

    StaticStrings* ss = element->starfish()->staticStrings();
    String* floodColor = element->getAttributeOrEmpty(ss->m_floodColor);
    String* floodOpacity = element->getAttributeOrEmpty(ss->m_floodOpacity);

    CSSTokenValue token(floodColor->toUTF8NonGCString());
    CSSStyleValuePair pair;
    if (pair.updateValueUnitColor(token)) {
        if (pair.valueKind() == CSSStyleValuePair::ValueKind::ColorValueKind) {
            m_floodColor = pair.colorValue();
        } else if (pair.valueKind() ==
                   CSSStyleValuePair::ValueKind::NamedColorValueKind) {
            m_floodColor =
                NamedColor::namedColorToColor(pair.namedColorValue());
        }
    }

    if (pair.updateValueUnitNumber(
            CSSTokenValue(floodOpacity->toUTF8NonGCString()),
            CSSPropertyParser::AllowNone)) {
        float n = pair.numberValue();
        if (n >= 0 && n < 1) {
            m_floodColor.m_a *= n;
        }
    }
}

void* FilterFlood::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FilterFlood));
    static bool typeInited = false;
    static GC_descr descr;
    if (typeInited == false) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FilterFlood)] = { 0 };
        STARFISH_ASSERT(obj_bitmap != nullptr);
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FilterFlood));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FilterFlood::apply(size_t x, size_t y, size_t width, size_t height,
                        Filter::FilterApplyContext& ctx)
{
    std::shared_ptr<Filter::FilterSourceBuffer> outputSource =
        filter()->fetchOutputSource(ctx, this, ctx.sourceGraphic());

    Canvas* c = Canvas::create(outputSource->data(), ctx.width, ctx.height,
                               ctx.stride, 1);
    c->clearColor(m_floodColor);
    delete c;

    filter()->registerOutput(ctx, this, outputSource);
}

} // namespace Starfish
