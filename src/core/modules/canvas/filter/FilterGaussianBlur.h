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

#ifndef __StarfishFilterGaussianBlur__
#define __StarfishFilterGaussianBlur__

#include "core/modules/canvas/filter/FilterPrimitive.h"

namespace Starfish {
class FilterGaussianBlur : public FilterPrimitive {
public:
    FilterGaussianBlur(Filter* filter,
                       SVGFilterPrimitiveStandardAttributes* element);
    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void apply(const Unit::Rect& subRegionInFloat,
                       Filter::FilterApplyContext& ctx) override;

    static std::pair<float, float> computeKernelSize(float stdDeviationX,
                                                     float stdDeviationY);

    std::pair<float, float> computeStdXY(
        const LayoutSize& targetSize,
        const std::pair<float, float>& viewportScale);
    virtual Filter::FilterBias computeBias(ComputeBiasContext& ctx);
};
} // namespace Starfish
#endif
