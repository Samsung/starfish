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

    virtual void apply(const Unit::Rect& subRegionInFloat,
                       Filter::FilterApplyContext& ctx) = 0;

    virtual bool shouldMaintainSourceBuffer(bool isFirstFilter)
    {
        return !isFirstFilter && (m_input->equals("SourceGraphic") ||
                                  m_input2->equals("SourceGraphic"));
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

    struct ComputeBiasContext {
        const LayoutSize& targetSize;
        const LayoutRect& unadjustedFrameRectByFilter;
        const Unit::Rect& candidateFilterFrameRect;
        const std::pair<float, float>& viewportScale;
        LayoutRect currentVisibleRect;
    };

    virtual Filter::FilterBias computeBias(ComputeBiasContext& ctx)
    {
        return Filter::FilterBias();
    }

    virtual bool canShrinkPreviousResult() const
    {
        return false;
    }

    static bool subRegionCoversAll(const Unit::Rect& subRegionInFloat)
    {
        return subRegionInFloat.x() <= 0 && subRegionInFloat.y() <= 0 &&
               (subRegionInFloat.maxX()) >= 1 && (subRegionInFloat.maxY()) >= 1;
    }

    static Unit::Rect normalizeSubRegion(const Unit::Rect& subRegion);

    virtual bool canSubRegionExpandFrameRect()
    {
        return false;
    }

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(FilterPrimitive, m_input));
        GC_set_bit(desc, GC_WORD_OFFSET(FilterPrimitive, m_input2));
        GC_set_bit(desc, GC_WORD_OFFSET(FilterPrimitive, m_output));
        GC_set_bit(desc, GC_WORD_OFFSET(FilterPrimitive, m_filter));
        GC_set_bit(desc, GC_WORD_OFFSET(FilterPrimitive, m_domElement));
    }

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
