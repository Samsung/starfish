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
#ifndef __StarfishFilter__
#define __StarfishFilter__

#include "core/dom/Document.h"
#include "core/layout/Frame.h"
#include "core/dom/svg/SVGUnitTypes.h"

namespace Starfish {

class FilterPrimitive;
class SVGElement;

class Filter : public gc {
public:
    Filter(SVGElement* owner);

    void applyFilter(size_t w, size_t s, size_t h,
                     GCAtomicVector<uint8_t>* sourceGraphic);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    void registerSourceBuffer(String* sourceName,
                              GCAtomicVector<uint8_t>* sourceBuffer);
    GCAtomicVector<uint8_t>* getSourceBuffer(String* sourceName);
    void clearSourceBuffer();

    void updateIfNeeds();
    void setNeedsUpdate()
    {
        m_needsUpdate = true;
    }

    float biasX()
    {
        return m_filterBiasX;
    }

    float biasY()
    {
        return m_filterBiasY;
    }

    float biasWidth()
    {
        return m_filterBiasWidth;
    }

    float biasHeight()
    {
        return m_filterBiasHeight;
    }

    void setBias(float x, float y, float width, float height);

private:
    void rebuildFiter();
    FilterPrimitive* createFilterPrimitive(
        SVGFilterPrimitiveStandardAttributes* filterPrimitiveNode);

    bool m_needsUpdate = false;
    float m_filterBiasX = 0;
    float m_filterBiasY = 0;
    float m_filterBiasWidth = 0;
    float m_filterBiasHeight = 0;

    SVGElement* m_owner{ nullptr };
    SVGUnitTypes::UnitTypes m_filterUnits;
    SVGUnitTypes::UnitTypes m_primitiveUnits;

    GCVector<FilterPrimitive*> m_filterPrimitives;
    GCUnorderedMap<String*, GCAtomicVector<uint8_t>*> m_sources;
};
} // namespace Starfish
#endif
