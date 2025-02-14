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
#include "core/modules/canvas/filter/Filter.h"
#include "core/modules/canvas/filter/FilterPrimitive.h"
#include "core/modules/canvas/filter/FilterGaussianBlur.h"
#include "core/dom/svg/SVGElement.h"
#include "core/dom/svg/SVGFilterElement.h"
#include "core/dom/svg/SVGFilterPrimitiveStandardAttributes.h"
#include "core/layout/svg/FrameSVGBox.h"

namespace Starfish {
Filter::Filter(SVGElement* owner)
    : m_owner(owner)
{
    STARFISH_ASSERT(m_owner);
    STARFISH_ASSERT(m_owner->isSVGFilterElement());
    m_filterUnits = (SVGUnitTypes::UnitTypes)m_owner->asSVGFilterElement()
                        ->filterUnits()
                        ->baseVal();
    m_primitiveUnits = (SVGUnitTypes::UnitTypes)m_owner->asSVGFilterElement()
                           ->primitiveUnits()
                           ->baseVal();
}

void Filter::applyFilter(PaintingContext& ctx, FrameSVGBox* targetBox,
                         GCAtomicVector<uint8_t>* sourceGraphic)
{
    String* defaultSourceStr = String::createASCIIString("SourceGraphic");
    updateIfNeeds();
    clearSourceBuffer();
    registerSourceBuffer(defaultSourceStr, sourceGraphic);

    for (auto primitive : m_filterPrimitives) {
        primitive->apply(targetBox->x().toInt(), targetBox->y().toInt(),
                         targetBox->width().toInt(),
                         targetBox->height().toInt(),
                         targetBox->width().toInt() * 4);
    }
}

void Filter::clearSourceBuffer()
{
    m_sources.clear();
}

void Filter::setBias(float x, float y, float width, float height)
{
    m_filterBiasX = std::min(m_filterBiasX, x);
    m_filterBiasY = std::min(m_filterBiasY, y);
    m_filterBiasWidth = std::max(m_filterBiasWidth, width);
    m_filterBiasHeight = std::max(m_filterBiasHeight, height);
}

void Filter::registerSourceBuffer(String* sourceName,
                                  GCAtomicVector<uint8_t>* sourceBuffer)
{
    auto iter = m_sources.find(sourceName);
    if (iter != m_sources.end()) {
        iter.value() = sourceBuffer;
        return;
    }
    m_sources.insert(std::make_pair(sourceName, sourceBuffer));
}

GCAtomicVector<uint8_t>* Filter::getSourceBuffer(String* sourceName)
{
    auto iter = m_sources.find(sourceName);
    if (iter != m_sources.end()) {
        return iter->second;
    }
    return nullptr;
}

void Filter::updateIfNeeds()
{
    if (m_needsUpdate) {
        rebuildFiter();
        m_needsUpdate = false;
    }
}

void Filter::rebuildFiter()
{
    m_filterPrimitives.clear();
    if (m_owner && m_owner->hasChildNodes()) {
        Node* current = m_owner->firstChild();
        while (current) {
            if (current->isSVGFilterPrimitiveStandardAttributes()) {
                FilterPrimitive* filterPrimitive = createFilterPrimitive(
                    current->asSVGFilterPrimitiveStandardAttributes());
                if (filterPrimitive) {
                    m_filterPrimitives.push_back(filterPrimitive);
                }
            }
            current = current->nextSibling();
        }
    }
}

FilterPrimitive* Filter::createFilterPrimitive(
    SVGFilterPrimitiveStandardAttributes* filterPrimitiveNode)
{
    FilterPrimitive* primitive = nullptr;
    if (filterPrimitiveNode->isSVGFEGaussianBlurElement()) {
        primitive = new FilterGaussianBlur(this, filterPrimitiveNode);
    }
    return primitive;
}

void* Filter::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(Filter));
    static bool typeInited = false;
    static GC_descr descr;
    if (typeInited == false) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(Filter)] = { 0 };
        STARFISH_ASSERT(obj_bitmap != nullptr);
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(Filter, m_owner));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(Filter, m_filterUnits));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(Filter, m_primitiveUnits));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(Filter, m_filterPrimitives));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(Filter, m_sources));

        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(Filter));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}
} // namespace Starfish
