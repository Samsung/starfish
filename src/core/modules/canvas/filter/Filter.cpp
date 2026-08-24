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
#include "core/modules/canvas/filter/FilterFlood.h"
#include "core/modules/canvas/filter/FilterTurbulence.h"
#include "core/modules/canvas/filter/FilterDisplacementMap.h"
#include "core/dom/svg/SVGElement.h"
#include "core/page/WebView.h"
#include "core/dom/svg/SVGFilterElement.h"
#include "core/dom/svg/SVGFilterPrimitiveStandardAttributes.h"
#include "core/layout/svg/FrameSVGBox.h"
#include "core/layout/svg/FrameSVGSVGBox.h"

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
    const std::shared_ptr<FilterSourceBuffer>& input,
    const Unit::Rect& subRegionInFloat)
{
    bool isSubRegionCoversAll =
        FilterPrimitive::subRegionCoversAll(subRegionInFloat);
    if ((!isSubRegionCoversAll || m_shouldMaintainSourceBuffer) &&
        input->data() == ctx.src) {
        auto o = std::shared_ptr<Filter::FilterSourceBuffer>(
            new FilterSourceBuffer(ctx.src, ctx.stride * ctx.height, true));
        if (!isSubRegionCoversAll) {
            memset(o->data(), 0, o->size());
        }
        return o;
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

static float resolveBoundingBoxUnitSVGLength(SVGLength* l, float parentLength)
{
    if (l->unitType() == SVGLength::UnitType::SVG_LENGTHTYPE_NUMBER) {
        return l->valueInSpecifiedUnits(false);
    } else if (l->unitType() ==
               SVGLength::UnitType::SVG_LENGTHTYPE_PERCENTAGE) {
        return l->valueInSpecifiedUnits(false) / 100;
    } else {
        return l->value() / parentLength;
    }
}

static float resolveUserspaceUnitSVGLength(SVGLength* l, float viewportScale,
                                           const LayoutUnit& viewportLength)
{
    if (l->unitType() == SVGLength::UnitType::SVG_LENGTHTYPE_NUMBER) {
        return l->valueInSpecifiedUnits(false) * viewportScale;
    } else if (l->unitType() ==
               SVGLength::UnitType::SVG_LENGTHTYPE_PERCENTAGE) {
        float p = l->valueInSpecifiedUnits(false) / 100;
        p *= viewportLength;
        p *= viewportScale;
        return p;
    } else {
        return l->value(false);
    }
}

Unit::Rect Filter::computeSubRegion(
    FilterPrimitive* primitive, SVGFilterElement* filterElement,
    FrameSVGBox* targetBox, const std::pair<float, float>& viewportScale)
{
    auto x = primitive->element()->x()->baseVal();
    auto y = primitive->element()->y()->baseVal();
    auto width = primitive->element()->width()->baseVal();
    auto height = primitive->element()->height()->baseVal();

    bool isObjectBoundingBox = filterElement->primitiveUnits()->baseVal() ==
                               SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX;
    auto unAdjustedFrameRect = targetBox->unadjustedFrameRectByFilter();
    auto vp = targetBox->viewport();
    Unit::Rect subRegionInFloat(0, 0, 1, 1);
    if (isObjectBoundingBox) {
        subRegionInFloat.setX(resolveBoundingBoxUnitSVGLength(
            x, unAdjustedFrameRect->width().toFloat()));
        subRegionInFloat.setY(resolveBoundingBoxUnitSVGLength(
            y, unAdjustedFrameRect->height().toFloat()));
        subRegionInFloat.setWidth(resolveBoundingBoxUnitSVGLength(
            width, unAdjustedFrameRect->width().toFloat()));
        subRegionInFloat.setHeight(resolveBoundingBoxUnitSVGLength(
            height, unAdjustedFrameRect->height().toFloat()));
    } else {
        auto vp = targetBox->viewport();
        LayoutRect viewportRect(0, 0, viewportScale.first * vp.width(),
                                viewportScale.second * vp.height());
        LayoutRect absoluteRect(
            resolveUserspaceUnitSVGLength(x, viewportScale.first, vp.width()),
            resolveUserspaceUnitSVGLength(y, viewportScale.second, vp.height()),
            resolveUserspaceUnitSVGLength(width, viewportScale.first,
                                          vp.width()),
            resolveUserspaceUnitSVGLength(height, viewportScale.second,
                                          vp.height()));

        if (absoluteRect.containsInVisual(viewportRect)) {
            subRegionInFloat = Unit::Rect(0, 0, 1, 1);
        } else if (!unAdjustedFrameRect->isEmpty()) {
            LayoutRect rt = targetBox->parent()->asFrameBox()->absoluteRect(
                targetBox->outmostSVGViewportBox());
            rt.setX(rt.x() + unAdjustedFrameRect->x());
            rt.setY(rt.y() + unAdjustedFrameRect->y());
            rt.setWidth(unAdjustedFrameRect->width());
            rt.setHeight(unAdjustedFrameRect->height());
            LayoutRect ort = LayoutRect::overlappedRect(absoluteRect, rt);
            subRegionInFloat = Unit::Rect((ort.x() - rt.x()) / rt.width(),
                                          (ort.y() - rt.y()) / rt.height(),
                                          ort.width() / rt.width(),
                                          ort.height() / rt.height());
        }
    }

    return subRegionInFloat;
}

// A chain that ends in a wide Gaussian blur can run every primitive on a
// downscaled copy: whatever detail the reduced resolution loses is detail the
// final blur would have erased anyway. This is the same trade
// FilterGaussianBlur already makes internally, extended to the primitives
// before it — for the Bixby glow (feTurbulence -> feDisplacementMap ->
// feGaussianBlur) the turbulence and displacement passes are the bulk of the
// per-frame filter cost and they scale with the pixel count.
int Filter::chainDownsampleFactor(FilterApplyContext& ctx)
{
    if (m_filterPrimitives.empty()) {
        return 1;
    }

    for (auto* primitive : m_filterPrimitives) {
        // Every op here is resolution-independent given the scaled
        // FilterApplyContext (their geometry all derives from
        // viewportScale). feMorphology is excluded: a structuring element
        // eroded at half resolution is not half an erosion.
        auto* e = primitive->element();
        if (!(e->isSVGFEGaussianBlurElement() ||
              e->isSVGFEColorMatrixElement() ||
              e->isSVGFEComponentTransferElement() ||
              e->isSVGFEMergeElement() || e->isSVGFECompositeElement() ||
              e->isSVGFEOffsetElement() || e->isSVGFEFloodElement() ||
              e->isSVGFETurbulenceElement() ||
              e->isSVGFEDisplacementMapElement())) {
            return 1;
        }
    }

    auto* last = m_filterPrimitives.back();
    if (!last->element()->isSVGFEGaussianBlurElement()) {
        return 1;
    }

    FilterGaussianBlur* blur = static_cast<FilterGaussianBlur*>(last);
    auto vm = ctx.target->outmostSVGViewportBox()
                  ->computeTranlateScaleOnPaint()
                  .second;
    auto stdXY =
        blur->computeStdXY(ctx.target->unadjustedFrameRectByFilter()->size(),
                           std::make_pair(vm.getScaleX(), vm.getScaleY()));
    if (stdXY.first <= 0 || stdXY.second <= 0) {
        return 1;
    }
    auto kernel = FilterGaussianBlur::computeKernelSize(
        stdXY.first * ctx.viewportScaleX, stdXY.second * ctx.viewportScaleY);
    float dpr = owner()->webView()->screenInfo().devicePixelRatio;
    unsigned kernelX = (unsigned)(kernel.first * dpr);
    unsigned kernelY = (unsigned)(kernel.second * dpr);
    unsigned kernelMin =
        std::min(kernelX ? kernelX : kernelY, kernelY ? kernelY : kernelX);

    // Mirror FilterGaussianBlur::computeDownsampleFactor's guards: keep >= 16
    // samples across the kernel and >= 16 px per reduced dimension.
    const int factor = 2;
    if (kernelMin / (unsigned)factor < 16 || (int)ctx.width / factor < 16 ||
        (int)ctx.height / factor < 16) {
        return 1;
    }
    return factor;
}

void Filter::applyFilter(FilterApplyContext& ctx)
{
    updateIfNeeds();

    FrameSVGSVGBox* viewportBox = ctx.target->outmostSVGViewportBox();
    auto transScale = viewportBox->computeTranlateScaleOnPaint();
    auto subRegionScale = std::make_pair(transScale.second.getScaleX(),
                                         transScale.second.getScaleY());

    int factor = chainDownsampleFactor(ctx);
    if (factor > 1) {
        int reducedWidth = ((int)ctx.width + factor - 1) / factor;
        int reducedHeight = ((int)ctx.height + factor - 1) / factor;
        size_t reducedStride = (size_t)reducedWidth * 4;
        FilterSourceBuffer reduced(nullptr, reducedStride * reducedHeight,
                                   true);
        filterDownsampleRGBA(ctx.src, ctx.width, ctx.height, ctx.stride,
                             reduced.data(), reducedWidth, reducedHeight,
                             reducedStride, factor);

        FilterApplyContext reducedCtx(
            ctx.target, reducedWidth, reducedStride, reducedHeight,
            reduced.data(), ctx.viewportScaleX / factor,
            ctx.viewportScaleY / factor, ctx.isAlphaImage);
        for (auto* primitive : m_filterPrimitives) {
            primitive->apply(computeSubRegion(primitive, owner(), ctx.target,
                                              subRegionScale),
                             reducedCtx);
        }

        std::shared_ptr<FilterSourceBuffer> full(
            new FilterSourceBuffer(nullptr, ctx.stride * ctx.height, true));
        filterUpsampleRGBA(reducedCtx.output->data(), reducedWidth,
                           reducedHeight, reducedStride, full->data(),
                           ctx.width, ctx.height, ctx.stride, factor);
        ctx.output = full;
        return;
    }

    for (auto* primitive : m_filterPrimitives) {
        primitive->apply(
            computeSubRegion(primitive, owner(), ctx.target, subRegionScale),
            ctx);
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
    FrameSVGBox* target, const LayoutRect& unadjustedFrameRectByFilter,
    const Unit::Rect& candidateFilterFrameRect,
    const std::pair<float, float>& viewportScale)
{
    updateIfNeeds();

    FilterBias ret;
    auto siz = target->frameRect().size();
    for (auto* f : m_filterPrimitives) {
        if (f->canShrinkPreviousResult()) {
            ret.maximumBias = NullOption;
            ret.minimumBias = NullOption;
        }

        FilterPrimitive::ComputeBiasContext ctx{
            siz, unadjustedFrameRectByFilter, candidateFilterFrameRect,
            viewportScale, unadjustedFrameRectByFilter
        };
        auto subResult = f->computeBias(ctx);

        if (subResult.maximumBias) {
            if (!ret.maximumBias) {
                ret.maximumBias = subResult.maximumBias.value();
            } else {
                subResult.maximumBias.value().unite(ret.maximumBias.value());
                ret.maximumBias = subResult.maximumBias.value();
            }
        }

        if (subResult.minimumBias) {
            if (!ret.minimumBias) {
                ret.minimumBias = subResult.minimumBias.value();
            } else {
                subResult.minimumBias.value().unite(ret.minimumBias.value());
                ret.minimumBias = subResult.minimumBias.value();
            }
        }
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
    } else if (filterPrimitiveNode->isSVGFEFloodElement()) {
        primitive = new FilterFlood(this, filterPrimitiveNode);
    } else if (filterPrimitiveNode->isSVGFETurbulenceElement()) {
        primitive = new FilterTurbulence(this, filterPrimitiveNode);
    } else if (filterPrimitiveNode->isSVGFEDisplacementMapElement()) {
        primitive = new FilterDisplacementMap(this, filterPrimitiveNode);
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
