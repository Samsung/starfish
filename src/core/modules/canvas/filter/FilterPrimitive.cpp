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
                                 SVGFilterPrimitiveStandardAttributes* element,
                                 String* input, String* result)
    : FilterPrimitive(filter, element, input, String::emptyString, result)
{
}

FilterPrimitive::FilterPrimitive(Filter* filter,
                                 SVGFilterPrimitiveStandardAttributes* element,
                                 String* input, String* input2, String* result)
    : m_filter(filter)
    , m_input(input)
    , m_input2(input2)
    , m_output(result)
    , m_domElement(element)
{
}

} // namespace Starfish
