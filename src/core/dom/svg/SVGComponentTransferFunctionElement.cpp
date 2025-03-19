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
#include "Starfish.h"

#include "core/dom/svg/SVGComponentTransferFunctionElement.h"
#include "core/dom/svg/SVGFilterElement.h"
#include "core/dom/svg/SVGAnimatedNumber.h"
#include "core/dom/svg/SVGAnimatedNumberList.h"
#include "core/dom/svg/SVGAnimatedEnumeration.h"

namespace Starfish {

void SVGComponentTransferFunctionElement::didAttributeChanged(
    QualifiedName name, Optional<String*> old, String* value,
    bool attributeCreated, bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);

    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_type == name) {
        notifyAttributeOfPaintServerLikeUpdated();
        if (type()->isUpdated() == false) {
            if (value->equals("identity")) {
                m_type->setBaseValWithoutUpdateAttribute(
                    ComponentTransferType::
                        SVG_FECOMPONENTTRANSFER_TYPE_IDENTITY);
            } else if (value->equals("table")) {
                m_type->setBaseValWithoutUpdateAttribute(
                    ComponentTransferType::SVG_FECOMPONENTTRANSFER_TYPE_TABLE);
            } else if (value->equals("discrete")) {
                m_type->setBaseValWithoutUpdateAttribute(
                    ComponentTransferType::
                        SVG_FECOMPONENTTRANSFER_TYPE_DISCRETE);
            } else if (value->equals("linear")) {
                m_type->setBaseValWithoutUpdateAttribute(
                    ComponentTransferType::SVG_FECOMPONENTTRANSFER_TYPE_LINEAR);
            } else if (value->equals("gamma")) {
                m_type->setBaseValWithoutUpdateAttribute(
                    ComponentTransferType::SVG_FECOMPONENTTRANSFER_TYPE_GAMMA);
            } else {
                m_type->setBaseValWithoutUpdateAttribute(
                    ComponentTransferType::
                        SVG_FECOMPONENTTRANSFER_TYPE_UNKNOWN);
            }
        }
    } else if (ss->m_tableValues == name) {
        notifyAttributeOfPaintServerLikeUpdated();
        SVGNumberList* valueList = tableValues()->baseVal();
        valueList->setValueByString(value);
    } else if (ss->m_slope == name) {
        notifyAttributeOfPaintServerLikeUpdated();
        slope()->setBaseVal(String::parseFloat(value), true);
    } else if (ss->m_intercept == name) {
        notifyAttributeOfPaintServerLikeUpdated();
        intercept()->setBaseVal(String::parseFloat(value), true);
    } else if (ss->m_amplitude == name) {
        notifyAttributeOfPaintServerLikeUpdated();
        amplitude()->setBaseVal(String::parseFloat(value), true);
    } else if (ss->m_exponent == name) {
        notifyAttributeOfPaintServerLikeUpdated();
        exponent()->setBaseVal(String::parseFloat(value), true);
    } else if (ss->m_offset == name) {
        notifyAttributeOfPaintServerLikeUpdated();
        offset()->setBaseVal(String::parseFloat(value), true);
    }
}

void SVGComponentTransferFunctionElement::updateSVGAttributeNeeded(
    QualifiedName name)
{
    SVGElement::updateSVGAttributeNeeded(name);

    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_type == name) {
        switch (type()->baseVal()) {
        case ComponentTransferType::SVG_FECOMPONENTTRANSFER_TYPE_IDENTITY:
            setAttribute(ss->m_type, String::fromUTF8("identity"));
            break;
        case ComponentTransferType::SVG_FECOMPONENTTRANSFER_TYPE_TABLE:
            setAttribute(ss->m_type, String::fromUTF8("table"));
            break;
        case ComponentTransferType::SVG_FECOMPONENTTRANSFER_TYPE_DISCRETE:
            setAttribute(ss->m_type, String::fromUTF8("discrete"));
            break;
        case ComponentTransferType::SVG_FECOMPONENTTRANSFER_TYPE_LINEAR:
            setAttribute(ss->m_type, String::fromUTF8("linear"));
            break;
        case ComponentTransferType::SVG_FECOMPONENTTRANSFER_TYPE_GAMMA:
            setAttribute(ss->m_type, String::fromUTF8("gamma"));
            break;
        case ComponentTransferType::SVG_FECOMPONENTTRANSFER_TYPE_UNKNOWN:
            setAttribute(ss->m_type, String::emptyString);
            break;
        default:
            STARFISH_ASSERT_NOT_REACHED();
        }
    } else if (ss->m_tableValues == name) {
        setAttribute(ss->m_tableValues, tableValues()->baseVal()->toString());
    } else if (ss->m_slope == name) {
        setAttribute(ss->m_slope, String::fromFloat(slope()->baseVal()));
    } else if (ss->m_intercept == name) {
        setAttribute(ss->m_intercept,
                     String::fromFloat(intercept()->baseVal()));
    } else if (ss->m_amplitude == name) {
        setAttribute(ss->m_amplitude,
                     String::fromFloat(amplitude()->baseVal()));
    } else if (ss->m_exponent == name) {
        setAttribute(ss->m_exponent, String::fromFloat(exponent()->baseVal()));
    } else if (ss->m_offset == name) {
        setAttribute(ss->m_offset, String::fromFloat(offset()->baseVal()));
    }
}

SVGAnimatedEnumeration* SVGComponentTransferFunctionElement::type()
{
    if (!m_type.hasValue()) {
        m_type = new SVGAnimatedEnumeration(
            this, staticStrings()->m_type,
            ComponentTransferType::SVG_FECOMPONENTTRANSFER_TYPE_IDENTITY,
            ComponentTransferType::SVG_FECOMPONENTTRANSFER_TYPE_IDENTITY,
            ComponentTransferType::SVG_FECOMPONENTTRANSFER_TYPE_GAMMA);
    }
    return m_type.value();
}

SVGAnimatedNumberList* SVGComponentTransferFunctionElement::tableValues()
{
    if (!m_tableValues.hasValue()) {
        m_tableValues = new SVGAnimatedNumberList(
            document(),
            new SVGNumberList(this, AtomicString::emptyAtomicString()),
            new SVGNumberList(this, AtomicString::emptyAtomicString()));
    }
    return m_tableValues.value();
}

SVGAnimatedNumber* SVGComponentTransferFunctionElement::slope()
{
    if (!m_slope.hasValue()) {
        m_slope = new SVGAnimatedNumber(
            this, starfish()->staticStrings()->m_slope, 1, 1);
    }
    return m_slope.value();
}

SVGAnimatedNumber* SVGComponentTransferFunctionElement::intercept()
{
    if (!m_intercept.hasValue()) {
        m_intercept = new SVGAnimatedNumber(
            this, starfish()->staticStrings()->m_intercept, 0, 0);
    }
    return m_intercept.value();
}

SVGAnimatedNumber* SVGComponentTransferFunctionElement::amplitude()
{
    if (!m_amplitude.hasValue()) {
        m_amplitude = new SVGAnimatedNumber(
            this, starfish()->staticStrings()->m_amplitude, 1, 1);
    }
    return m_amplitude.value();
}

SVGAnimatedNumber* SVGComponentTransferFunctionElement::exponent()
{
    if (!m_exponent.hasValue()) {
        m_exponent = new SVGAnimatedNumber(
            this, starfish()->staticStrings()->m_exponent, 1, 1);
    }
    return m_exponent.value();
}

SVGAnimatedNumber* SVGComponentTransferFunctionElement::offset()
{
    if (!m_offset.hasValue()) {
        m_offset = new SVGAnimatedNumber(
            this, starfish()->staticStrings()->m_offset, 0, 0);
    }
    return m_offset.value();
}

void SVGComponentTransferFunctionElement::
    notifyAttributeOfPaintServerLikeUpdated()
{
    if (parentElement() && parentElement()->isSVGFEComponentTransferElement() &&
        parentElement()->parentElement() &&
        parentElement()->parentElement()->isSVGFilterElement()) {
        parentElement()
            ->parentElement()
            ->asSVGFilterElement()
            ->attributeOfPaintServerLikeUpdated(false);
    }
}

void* SVGComponentTransferFunctionElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGComponentTransferFunctionElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGComponentTransferFunctionElement)] = {
            0
        };
        fillGCDescriptor(desc);
        descr = GC_make_descriptor(
            desc, GC_WORD_LEN(SVGComponentTransferFunctionElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

} // namespace Starfish
