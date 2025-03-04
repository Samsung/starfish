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
#include "core/modules/canvas/filter/FilterPrimitive.h"

namespace Starfish {

FilterPrimitive::FilterPrimitive(Filter* filter,
                                 SVGFilterPrimitiveStandardAttributes* element)
    : m_filter(filter)
    , m_domElement(element)
{
}

void FilterPrimitive::apply(size_t x, size_t y, size_t width, size_t height,
                            Filter::FilterApplyContext& ctx)
{
    STARFISH_ASSERT_NOT_REACHED();
}

void* FilterPrimitive::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FilterPrimitive));
    static bool typeInited = false;
    static GC_descr descr;
    if (typeInited == false) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FilterPrimitive)] = { 0 };
        STARFISH_ASSERT(obj_bitmap != nullptr);
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FilterPrimitive, m_filter));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FilterPrimitive, m_domElement));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FilterPrimitive));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}
} // namespace Starfish
