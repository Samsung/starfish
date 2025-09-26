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

#ifndef __StarfishFilterOffset__
#define __StarfishFilterOffset__

#include "core/modules/canvas/filter/FilterPrimitive.h"

namespace Starfish {
class FilterOffset : public FilterPrimitive {
public:
    FilterOffset(Filter* filter, SVGFilterPrimitiveStandardAttributes* element);
    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual Filter::FilterBias computeBias(
        const LayoutSize& targetSize,
        const Unit::Rect& candidateFilterFrameRect,
        const std::pair<float, float>& viewportScale) override;

    virtual void apply(const Unit::Rect& subRegionInFloat,
                       Filter::FilterApplyContext& ctx) override;
};
} // namespace Starfish
#endif
