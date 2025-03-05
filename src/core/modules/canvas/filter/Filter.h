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

    struct FilterApplyContext {
        uint8_t* src;
        size_t width, stride, height;
        float viewportScaleX;
        float viewportScaleY;
        bool isAlphaImage;

        enum FixedSourcePlace {
            SourceGraphic,
        };

        class FilterSourceBuffer {
        public:
            FilterSourceBuffer(uint8_t* src, size_t size,
                               bool needsAllocateNewBuffer)
            {
                m_size = size;
                if (needsAllocateNewBuffer) {
                    m_externalBuffer.resize(size);
                } else {
                    m_buffer = src;
                }
            }

            uint8_t* data()
            {
                if (m_buffer) {
                    return m_buffer.value();
                }
                return m_externalBuffer.data();
            }

            size_t size()
            {
                return m_size;
            }

        private:
            Optional<uint8_t*> m_buffer;
            size_t m_size;
            std::vector<uint8_t> m_externalBuffer;
        };
        std::vector<std::pair<std::string, std::shared_ptr<FilterSourceBuffer>>>
            sources;

        FilterApplyContext(size_t w, size_t s, size_t h, uint8_t* srcData,
                           float scaleX, float scaleY)
        {
            src = srcData;
            width = w;
            stride = s;
            height = h;
            viewportScaleX = scaleX;
            viewportScaleY = scaleY;
            isAlphaImage = false;

            // first slot is always SourceGraphic
            sources.push_back(std::make_pair(
                "SourceGraphic",
                std::shared_ptr<FilterSourceBuffer>(
                    new FilterSourceBuffer(src, stride * height, false))));
            STARFISH_ASSERT(sources[0].first == "SourceGraphic");
        }

        std::shared_ptr<FilterSourceBuffer> sourceGraphic()
        {
            STARFISH_ASSERT(sources[0].first == "SourceGraphic");
            return sources[FixedSourcePlace::SourceGraphic].second;
        }

        void updateSourceGraphic(const std::shared_ptr<FilterSourceBuffer>& s)
        {
            STARFISH_ASSERT(sources[0].first == "SourceGraphic");
            if (s) {
                STARFISH_ASSERT(s->size() == stride * height);
            }
            sources[FixedSourcePlace::SourceGraphic].second = s;
        }
    };

    void applyFilter(FilterApplyContext& ctx);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    void updateIfNeeds();
    void setNeedsUpdate()
    {
        m_needsUpdate = true;
    }

    float biasX()
    {
        updateIfNeeds();
        return m_filterBiasX;
    }

    float biasY()
    {
        updateIfNeeds();
        return m_filterBiasY;
    }

    float biasWidth()
    {
        updateIfNeeds();
        return m_filterBiasWidth;
    }

    float biasHeight()
    {
        updateIfNeeds();
        return m_filterBiasHeight;
    }

    void setBias(float x, float y, float width, float height);

private:
    void rebuildFiter();
    Optional<FilterPrimitive*> createFilterPrimitive(
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
};
} // namespace Starfish
#endif
