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

#ifndef __StarfishFilterColorMatrix__
#define __StarfishFilterColorMatrix__

#include "core/modules/canvas/filter/FilterPrimitive.h"

namespace Starfish {
class FilterColorMatrix : public FilterPrimitive {
public:
    FilterColorMatrix(Filter* filter,
                      SVGFilterPrimitiveStandardAttributes* element);
    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void apply(size_t x, size_t y, size_t width, size_t height,
                       Filter::FilterApplyContext& ctx) override;
};
} // namespace Starfish
#endif
