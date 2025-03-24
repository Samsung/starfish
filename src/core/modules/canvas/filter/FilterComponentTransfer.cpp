/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Igalia, S.L.
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2015-2016 Apple, Inc. All rights reserved.
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

#include "core/dom/svg/SVGFEComponentTransferElement.h"
#include "core/dom/svg/SVGComponentTransferFunctionElement.h"
#include "core/dom/svg/SVGAnimatedNumber.h"
#include "core/dom/svg/SVGAnimatedNumberList.h"
#include "core/dom/svg/SVGAnimatedEnumeration.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/filter/FilterComponentTransfer.h"

namespace Starfish {

static void importValuesFromElement(
    SVGComponentTransferFunctionElement* element, ComponentTransferFunction& fn)
{
    fn.type =
        static_cast<SVGComponentTransferFunctionElement::ComponentTransferType>(
            element->type()->baseVal());
    fn.slope = element->slope()->baseVal();
    fn.intercept = element->intercept()->baseVal();
    fn.amplitude = element->amplitude()->baseVal();
    fn.exponent = element->exponent()->baseVal();
    fn.offset = element->offset()->baseVal();

    auto* input = element->tableValues()->baseVal();
    for (size_t i = 0; i < input->length(); i++) {
        fn.tableValues.push_back(input->getItem(i)->value());
    }
}

static void computeTableFromData(std::array<uint8_t, 256>& table,
                                 ComponentTransferFunction& fn)
{
    if (fn.type <= SVGComponentTransferFunctionElement::ComponentTransferType::
                       SVG_FECOMPONENTTRANSFER_TYPE_IDENTITY) {
        return;
    }

    const auto& tableValues = fn.tableValues;
    const uint32_t tableValuesSize = tableValues.size();

    if (fn.type == SVGComponentTransferFunctionElement::ComponentTransferType::
                       SVG_FECOMPONENTTRANSFER_TYPE_TABLE) {
        if (tableValuesSize < 1) {
            return;
        }

        for (size_t i = 0; i < table.size(); i++) {
            double c = i / 255.0;
            uint32_t k = static_cast<uint32_t>(c * (tableValuesSize - 1));
            double v1 = tableValues[k];
            double v2 = tableValues[std::min((k + 1), (tableValuesSize - 1))];
            double val =
                255.0 * (v1 + (c * (tableValuesSize - 1) - k) * (v2 - v1));
            val = std::max(0.0, std::min(255.0, val));
            table[i] = static_cast<uint8_t>(val);
        }
    } else if (fn.type ==
               SVGComponentTransferFunctionElement::ComponentTransferType::
                   SVG_FECOMPONENTTRANSFER_TYPE_DISCRETE) {
        if (tableValuesSize < 1) {
            return;
        }

        for (size_t i = 0; i < table.size(); i++) {
            uint32_t k = static_cast<uint32_t>((i * tableValuesSize) / 255.0);
            k = std::min(k, tableValuesSize - 1);
            double val = 255 * tableValues[k];
            val = std::max(0.0, std::min(255.0, val));
            table[i] = static_cast<uint8_t>(val);
        }
    } else if (fn.type ==
               SVGComponentTransferFunctionElement::ComponentTransferType::
                   SVG_FECOMPONENTTRANSFER_TYPE_LINEAR) {
        for (size_t i = 0; i < table.size(); i++) {
            double val = fn.slope * i + 255 * fn.intercept;
            val = std::max(0.0, std::min(255.0, val));
            table[i] = static_cast<uint8_t>(val);
        }
    } else if (fn.type ==
               SVGComponentTransferFunctionElement::ComponentTransferType::
                   SVG_FECOMPONENTTRANSFER_TYPE_GAMMA) {
        for (size_t i = 0; i < table.size(); i++) {
            double exponent = fn.exponent;
            double val =
                255.0 * (fn.amplitude * pow((i / 255.0), exponent) + fn.offset);
            val = std::max(0.0, std::min(255.0, val));
            table[i] = static_cast<uint8_t>(val);
        }
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }
}

FilterComponentTransfer::FilterComponentTransfer(
    Filter* filter, SVGFilterPrimitiveStandardAttributes* element)
    : FilterPrimitive(
          filter, element,
          element->asSVGFEComponentTransferElement()->in1()->baseVal(),
          element->output()->baseVal())
{
    STARFISH_ASSERT(filter);
    STARFISH_ASSERT(element->isSVGFEComponentTransferElement());

    for (size_t i = 0; i < m_aTable.size(); ++i) {
        m_aTable[i] = m_rTable[i] = m_gTable[i] = m_bTable[i] = i;
    }

    ComponentTransferFunction a, r, g, b;
    auto e = element->firstElementChild();
    while (e) {
        if (e->isSVGFEFuncAElement()) {
            importValuesFromElement(e->asSVGComponentTransferFunctionElement(),
                                    a);
        } else if (e->isSVGFEFuncRElement()) {
            importValuesFromElement(e->asSVGComponentTransferFunctionElement(),
                                    r);
        } else if (e->isSVGFEFuncGElement()) {
            importValuesFromElement(e->asSVGComponentTransferFunctionElement(),
                                    g);
        } else if (e->isSVGFEFuncBElement()) {
            importValuesFromElement(e->asSVGComponentTransferFunctionElement(),
                                    b);
        }
        e = e->nextElementSibling();
    }

    computeTableFromData(m_aTable, a);
    computeTableFromData(m_rTable, r);
    computeTableFromData(m_gTable, g);
    computeTableFromData(m_bTable, b);
}

void* FilterComponentTransfer::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FilterComponentTransfer));
    static bool typeInited = false;
    static GC_descr descr;
    if (typeInited == false) {
        GC_word desc[GC_BITMAP_SIZE(FilterComponentTransfer)] = { 0 };
        FilterPrimitive::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FilterComponentTransfer));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FilterComponentTransfer::apply(const Unit::Rect& subRegionInFloat,
                                    Filter::FilterApplyContext& ctx)
{
    SVGFEComponentTransferElement* ele =
        element()->asSVGFEComponentTransferElement();
    String* sourceNameStr = ele->in1()->baseVal();

    auto inputSource = filter()->fetchInputSource(ctx, this);
    auto outputSource = filter()->fetchOutputSource(ctx, this, inputSource);
    uint8_t* inputBuffer = inputSource->data();
    uint8_t* outputBuffer = outputSource->data();

    convertImageBufferAsPremultipliedAlphaIfNeeds(inputBuffer, ctx.width,
                                                  ctx.stride, ctx.height);

    for (size_t bY = 0; bY < ctx.height; bY++) {
        uint8_t* p = inputBuffer;
        uint8_t* p2 = outputBuffer;
        for (size_t bX = 0; bX < ctx.width; bX++) {
            p2[STARFISH_PIXEL_R_INDEX] = m_rTable[p[STARFISH_PIXEL_R_INDEX]];
            p2[STARFISH_PIXEL_G_INDEX] = m_gTable[p[STARFISH_PIXEL_G_INDEX]];
            p2[STARFISH_PIXEL_B_INDEX] = m_bTable[p[STARFISH_PIXEL_B_INDEX]];
            p2[STARFISH_PIXEL_A_INDEX] = m_aTable[p[STARFISH_PIXEL_A_INDEX]];
            p += 4;
            p2 += 4;
        }
        inputBuffer += ctx.stride;
        outputBuffer += ctx.stride;
    }

    convertImageBufferAsPremultipliedAlphaIfNeeds(
        outputSource->data(), ctx.width, ctx.stride, ctx.height);

    filter()->registerOutput(ctx, this, outputSource);
}

} // namespace Starfish
