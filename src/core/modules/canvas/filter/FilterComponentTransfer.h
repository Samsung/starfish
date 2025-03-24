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

#ifndef __StarfishFilterComponentTransfer__
#define __StarfishFilterComponentTransfer__

#include "core/modules/canvas/filter/FilterPrimitive.h"
#include "core/dom/svg/SVGComponentTransferFunctionElement.h"

namespace Starfish {

struct ComponentTransferFunction {
    ComponentTransferFunction()
        : type(SVGComponentTransferFunctionElement::ComponentTransferType::
                   SVG_FECOMPONENTTRANSFER_TYPE_UNKNOWN)
        , slope(0)
        , intercept(0)
        , amplitude(0)
        , exponent(0)
        , offset(0)
    {
    }

    SVGComponentTransferFunctionElement::ComponentTransferType type;
    float slope;
    float intercept;
    float amplitude;
    float exponent;
    float offset;
    std::vector<float> tableValues;
};

class FilterComponentTransfer : public FilterPrimitive {
public:
    FilterComponentTransfer(Filter* filter,
                            SVGFilterPrimitiveStandardAttributes* element);
    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void apply(const Unit::Rect& subRegionInFloat,
                       Filter::FilterApplyContext& ctx) override;

private:
    std::array<uint8_t, 256> m_aTable;
    std::array<uint8_t, 256> m_rTable;
    std::array<uint8_t, 256> m_gTable;
    std::array<uint8_t, 256> m_bTable;
};
} // namespace Starfish
#endif
