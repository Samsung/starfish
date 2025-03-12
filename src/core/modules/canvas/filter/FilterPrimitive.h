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

#ifndef __StarfishFilterPrimitive__
#define __StarfishFilterPrimitive__

#include "core/modules/canvas/filter/Filter.h"

namespace Starfish {
class SVGFilterPrimitiveStandardAttributes;
class FilterPrimitive : public gc {
public:
    FilterPrimitive(Filter* filter,
                    SVGFilterPrimitiveStandardAttributes* element,
                    String* input, String* result);
    FilterPrimitive(Filter* filter,
                    SVGFilterPrimitiveStandardAttributes* element,
                    String* input, String* input2, String* result);
    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void apply(size_t x, size_t y, size_t width, size_t height,
                       Filter::FilterApplyContext& ctx) = 0;

    virtual bool shouldMaintainSourceBuffer(bool isFirstFilter)
    {
        return !isFirstFilter && m_input->equals("SourceGraphic");
    }

    SVGFilterPrimitiveStandardAttributes* element()
    {
        return m_domElement;
    }

    String* input() const
    {
        return m_input;
    }

    String* input2() const
    {
        return m_input2;
    }

    String* output() const
    {
        return m_output;
    }

protected:
    Filter* filter()
    {
        return m_filter;
    }

private:
    Filter* m_filter;
    String* m_input;
    String* m_input2;
    String* m_output;
    SVGFilterPrimitiveStandardAttributes* m_domElement;
};
} // namespace Starfish
#endif
