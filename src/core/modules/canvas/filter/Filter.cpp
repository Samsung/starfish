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
#include "core/modules/canvas/filter/FilterColorMatrix.h"
#include "core/modules/canvas/filter/FilterComponentTransfer.h"
#include "core/modules/canvas/filter/FilterMerge.h"
#include "core/modules/canvas/filter/FilterComposite.h"
#include "core/modules/canvas/filter/FilterMorphology.h"
#include "core/modules/canvas/filter/FilterOffset.h"
#include "core/dom/svg/SVGElement.h"
#include "core/dom/svg/SVGFilterElement.h"
#include "core/dom/svg/SVGFilterPrimitiveStandardAttributes.h"
#include "core/layout/svg/FrameSVGBox.h"

namespace Starfish {
Filter::Filter(SVGFilterElement* owner)
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

float Filter::resolveFilterPrimitiveValue(float value, float boxSize,
                                          float viewportScale)
{
    auto type = m_owner->primitiveUnits()->baseVal();
    if (type == SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
        return boxSize * value;
    } else {
        return viewportScale * value;
    }
}

std::shared_ptr<Filter::FilterSourceBuffer>
Filter::FilterApplyContext::findSource(String* s)
{
    for (auto& pair : sources) {
        if (s->equals(pair.first.data(), pair.first.length())) {
            return pair.second;
        }
    }
    return nullptr;
}

void Filter::FilterApplyContext::registerSource(
    String* s, std::shared_ptr<FilterSourceBuffer> source)
{
    if (s->equals("SourceGraphic")) {
        return;
    }

    for (auto& pair : sources) {
        if (s->equals(pair.first.data(), pair.first.length())) {
            pair.second = source;
            return;
        }
    }
    sources.push_back(std::make_pair(s->toUTF32NonGCString(), source));
}

std::shared_ptr<Filter::FilterSourceBuffer> Filter::fetchInputSource(
    FilterApplyContext& ctx, FilterPrimitive* f)
{
    if (isFirstFilter(f)) {
        return ctx.sourceGraphic();
    }
    if (f->input()->isEmpty()) {
        return ctx.output;
    }
    auto s = ctx.findSource(f->input());
    if (!s) {
        STARFISH_ASSERT(ctx.output);
        return ctx.output;
    }
    return s;
}

std::shared_ptr<Filter::FilterSourceBuffer> Filter::fetchInputSource2(
    FilterApplyContext& ctx, FilterPrimitive* f)
{
    if (isFirstFilter(f)) {
        return ctx.sourceGraphic();
    }
    if (f->input2()->isEmpty()) {
        return ctx.output;
    }
    auto s = ctx.findSource(f->input2());
    if (!s) {
        STARFISH_ASSERT(ctx.output);
        return ctx.output;
    }
    return s;
}

std::shared_ptr<Filter::FilterSourceBuffer> Filter::fetchOutputSource(
    FilterApplyContext& ctx, FilterPrimitive* f,
    const std::shared_ptr<FilterSourceBuffer>& input)
{
    if (m_shouldMaintainSourceBuffer && input->data() == ctx.src) {
        return std::shared_ptr<Filter::FilterSourceBuffer>(
            new FilterSourceBuffer(ctx.src, ctx.stride * ctx.height, true));
    }
    return input;
}

void Filter::registerOutput(FilterApplyContext& ctx, FilterPrimitive* f,
                            const std::shared_ptr<FilterSourceBuffer>& s)
{
    ctx.output = s;
    if (isLastFilter(f) || f->output()->isEmpty()) {
        return;
    }
    ctx.registerSource(f->output(), s);
}

void Filter::applyFilter(FilterApplyContext& ctx)
{
    updateIfNeeds();
    for (auto* primitive : m_filterPrimitives) {
        // TODO
        // specify x, y, width, height and use it
        primitive->apply(0, 0, 0, 0, ctx);
    }
}

bool Filter::isFirstFilter(FilterPrimitive* f)
{
    return f == m_filterPrimitives.front();
}

bool Filter::isLastFilter(FilterPrimitive* f)
{
    return f == m_filterPrimitives.back();
}

void Filter::updateIfNeeds()
{
    if (m_needsUpdate) {
        rebuildFiter();
        m_needsUpdate = false;
    }
}

Filter::FilterBias Filter::computeBias(
    FrameSVGBox* target, const std::pair<float, float>& viewportScale)
{
    updateIfNeeds();

    FilterBias ret;
    auto siz = target->frameRect().size();
    for (auto* f : m_filterPrimitives) {
        auto subResult = f->computeBias(siz, viewportScale);
        ret.maximumBias.first =
            std::max(ret.maximumBias.first, subResult.maximumBias.first);
        ret.maximumBias.second =
            std::max(ret.maximumBias.second, subResult.maximumBias.second);
        ret.minimumBias.first =
            std::max(ret.minimumBias.first, subResult.minimumBias.first);
        ret.minimumBias.second =
            std::max(ret.minimumBias.second, subResult.minimumBias.second);
    }

    return ret;
}

void Filter::rebuildFiter()
{
    m_shouldMaintainSourceBuffer = false;
    m_filterPrimitives.clear();
    if (m_owner && m_owner->hasChildNodes()) {
        Node* current = m_owner->firstChild();
        while (current) {
            if (current->isSVGFilterPrimitiveStandardAttributes()) {
                Optional<FilterPrimitive*> filterPrimitive =
                    createFilterPrimitive(
                        current->asSVGFilterPrimitiveStandardAttributes());
                if (filterPrimitive) {
                    m_shouldMaintainSourceBuffer |=
                        filterPrimitive->shouldMaintainSourceBuffer(
                            !m_filterPrimitives.size());
                    m_filterPrimitives.push_back(filterPrimitive.value());
                }
            }
            current = current->nextSibling();
        }
    }
}

Optional<FilterPrimitive*> Filter::createFilterPrimitive(
    SVGFilterPrimitiveStandardAttributes* filterPrimitiveNode)
{
    Optional<FilterPrimitive*> primitive;
    if (filterPrimitiveNode->isSVGFEGaussianBlurElement()) {
        primitive = new FilterGaussianBlur(this, filterPrimitiveNode);
    } else if (filterPrimitiveNode->isSVGFEColorMatrixElement()) {
        primitive = new FilterColorMatrix(this, filterPrimitiveNode);
    } else if (filterPrimitiveNode->isSVGFEComponentTransferElement()) {
        primitive = new FilterComponentTransfer(this, filterPrimitiveNode);
    } else if (filterPrimitiveNode->isSVGFEMergeElement()) {
        primitive = new FilterMerge(this, filterPrimitiveNode);
    } else if (filterPrimitiveNode->isSVGFECompositeElement()) {
        primitive = new FilterComposite(this, filterPrimitiveNode);
    } else if (filterPrimitiveNode->isSVGFEMorphologyElement()) {
        primitive = new FilterMorphology(this, filterPrimitiveNode);
    } else if (filterPrimitiveNode->isSVGFEOffsetElement()) {
        primitive = new FilterOffset(this, filterPrimitiveNode);
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
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(Filter, m_filterPrimitives));

        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(Filter));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}
} // namespace Starfish
