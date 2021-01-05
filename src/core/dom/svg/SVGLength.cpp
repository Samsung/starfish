/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "SVGLength.h"
#include "SVGElement.h"
#include "core/style/ComputedStyle.h"
#include "core/style/CalcData.h"
#include "core/style/CSSParser.h"
#include "core/layout/FrameBox.h"
#include "core/layout/svg/FrameSVGSVGBox.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"

namespace Starfish {

SVGLength::SVGLength(SVGElement* sourceElement, QualifiedName targetAttribute)
    : ScriptWrappable(this)
    , m_sourceElement(sourceElement)
    , m_targetAttribute(targetAttribute)
    , m_unitType(SVG_LENGTHTYPE_NUMBER)
{
}

ScriptBindingInstance* SVGLength::scriptBindingInstance()
{
    return m_sourceElement->scriptBindingInstance();
}

unsigned short SVGLength::unitType()
{
    return m_unitType;
}

void SVGLength::setUnitType(unsigned short unitType)
{
    m_unitType = unitType;
}

static Nullable<Length> valueToLength(CSSStyleValuePair::ValueKind kind,
                                      CSSStyleValuePair::ValueData data)
{
    if (kind == CSSStyleValuePair::ValueKind::Auto) {
        return Length();
    } else if (kind == CSSStyleValuePair::ValueKind::Length) {
        return data.m_length.toLength();
    } else if (kind == CSSStyleValuePair::ValueKind::Percentage) {
        return Length(Length::Percent, data.m_floatValue);
    } else if (kind == CSSStyleValuePair::ValueKind::Number) {
        return Length(Length::Fixed, data.m_floatValue);
    } else if (kind == CSSStyleValuePair::ValueKind::CalcValueKind) {
        CalcValueType type = data.m_calc->type();
        if (type.isLength() || type.isPercentage()) {
            return Length(data.m_calc);
        } else {
            return Nullable<Length>();
        }
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return Nullable<Length>();
    }
}

float SVGLength::value()
{
    m_sourceElement->document()->browsingContext()->layoutIfNeeded();

    String* attrValue = m_sourceElement->getAttributeOrEmpty(m_targetAttribute);
    Length len;
    if (attrValue->length()) {
        auto s = attrValue->toUTF8NonGCString();
        CSSStyleValuePair pair;
        if (CSSPropertyParser::parseLength(
                s.data(), CSSPropertyParser::AllowPercent |
                              CSSPropertyParser::AllowWithoutUnit,
                &pair)) {
            Nullable<Length> value =
                valueToLength(pair.valueKind(), pair.value());
            if (value.hasValue()) {
                len = value.getValue();
            }
        }
    }

    if (len.isSpecified()) {
        FrameBox* cb =
            m_sourceElement->frame()
                ? m_sourceElement->frame()->layoutParent()->asFrameBox()
                : nullptr;
        FrameBox* svgBox = nullptr;
        if (cb) {
            svgBox = cb;
            while (true) {
                if (svgBox->isFrameReplaced() &&
                    svgBox->asFrameReplaced()->isFrameSVGSVGBox()) {
                    break;
                }
                svgBox = svgBox->layoutParent()->asFrameBox();
            }
        }

        LayoutUnit result = len.specifiedValue(
            cb ? (float)cb->contentWidth() : 0.f, m_sourceElement);
        if (svgBox) {
            result = result * ((FrameSVGSVGBox*)svgBox)->svgScale();
        }
        return result;
    }
    return 0;
}

void SVGLength::setValue(float v)
{
    if (std::isnan(v) || std::isinf(v)) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "The provided float value is non-finite");
    }

    m_sourceElement->setAttribute(m_targetAttribute, String::fromFloat(v));
}

float SVGLength::valueInSpecifiedUnits()
{
    // unimplemented PERCENTAGE, EMS, EXS
    if (m_unitType == SVG_LENGTHTYPE_NUMBER) {
        return value();
    } else if (m_unitType == SVG_LENGTHTYPE_PX) {
        return value();
    } else if (m_unitType == SVG_LENGTHTYPE_CM) {
        return value() / UnitHelper::UNIT_PX_PER_CM;
    } else if (m_unitType == SVG_LENGTHTYPE_MM) {
        return value() / UnitHelper::UNIT_PX_PER_MM;
    } else if (m_unitType == SVG_LENGTHTYPE_IN) {
        return value() / UnitHelper::UNIT_PX_PER_IN;
    } else if (m_unitType == SVG_LENGTHTYPE_PT) {
        return value() / UnitHelper::UNIT_PX_PER_PT;
    } else if (m_unitType == SVG_LENGTHTYPE_PC) {
        return value() / UnitHelper::UNIT_PX_PER_PC;
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    return 0;
}

void SVGLength::setValueInSpecifiedUnits(float v)
{
    // unimplemented PERCENTAGE, EMS, EXS
    if (m_unitType == SVG_LENGTHTYPE_NUMBER) {
        setValue(v);
    } else if (m_unitType == SVG_LENGTHTYPE_PX) {
        setValue(v);
    } else if (m_unitType == SVG_LENGTHTYPE_CM) {
        setValue(UnitHelper::convertFromCmToPx(v));
    } else if (m_unitType == SVG_LENGTHTYPE_MM) {
        setValue(UnitHelper::convertFromMmToPx(v));
    } else if (m_unitType == SVG_LENGTHTYPE_IN) {
        setValue(UnitHelper::convertFromInToPx(v));
    } else if (m_unitType == SVG_LENGTHTYPE_PT) {
        setValue(UnitHelper::convertFromPtToPx(v));
    } else if (m_unitType == SVG_LENGTHTYPE_PC) {
        setValue(UnitHelper::convertFromPcToPx(v));
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
}

String* SVGLength::valueAsString()
{
    String* str = String::fromFloat(valueInSpecifiedUnits());

    // unimplemented PERCENTAGE, EMS, EXS
    if (m_unitType == SVG_LENGTHTYPE_NUMBER) {
    } else if (m_unitType == SVG_LENGTHTYPE_PX) {
        str = str->concat("px");
    } else if (m_unitType == SVG_LENGTHTYPE_CM) {
        str = str->concat("cm");
    } else if (m_unitType == SVG_LENGTHTYPE_MM) {
        str = str->concat("mm");
    } else if (m_unitType == SVG_LENGTHTYPE_IN) {
        str = str->concat("in");
    } else if (m_unitType == SVG_LENGTHTYPE_PT) {
        str = str->concat("pt");
    } else if (m_unitType == SVG_LENGTHTYPE_PC) {
        str = str->concat("pc");
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    return str;
}

void SVGLength::setValueAsString(String* valueAsString)
{
    valueAsString = valueAsString->toLower();

    if (valueAsString->length()) {
        auto s = valueAsString->toUTF8NonGCString();
        CSSStyleValuePair pair;
        if (CSSPropertyParser::parseLength(
                s.data(), CSSPropertyParser::AllowPercent |
                              CSSPropertyParser::AllowWithoutUnit,
                &pair)) {
            if (pair.cssLengthValue().kind() == CSSLength::PX) {
                setUnitType(SVG_LENGTHTYPE_PX);
            } else if (pair.cssLengthValue().kind() == CSSLength::CM) {
                setUnitType(SVG_LENGTHTYPE_CM);
            } else if (pair.cssLengthValue().kind() == CSSLength::MM) {
                setUnitType(SVG_LENGTHTYPE_MM);
            } else if (pair.cssLengthValue().kind() == CSSLength::INCH) {
                setUnitType(SVG_LENGTHTYPE_IN);
            } else if (pair.cssLengthValue().kind() == CSSLength::PT) {
                setUnitType(SVG_LENGTHTYPE_PC);
            } else if (pair.cssLengthValue().kind() == CSSLength::EM) {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            } else if (pair.cssLengthValue().kind() == CSSLength::EX) {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            } else if (pair.cssLengthValue().kind() == CSSLength::PERCENT) {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            } else {
                throw new DOMException(m_sourceElement->executionContext(),
                                       DOMException::Code::NOT_SUPPORTED_ERR,
                                       "Not Supported error");
            }
            setValueInSpecifiedUnits(pair.cssLengthValue().value());
        } else {
            throw new DOMException(m_sourceElement->executionContext(),
                                   DOMException::Code::SYNTAX_ERR,
                                   "SyntaxError");
        }
    } else {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::Code::SYNTAX_ERR, "SyntaxError");
    }
}

void SVGLength::newValueSpecifiedUnits(unsigned short unitType,
                                       float valueInSpecifiedUnits)
{
    if (std::isnan(valueInSpecifiedUnits) ||
        std::isinf(valueInSpecifiedUnits)) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "The provided float value is non-finite");
    }

    convertToSpecifiedUnits(unitType);
    setValueInSpecifiedUnits(valueInSpecifiedUnits);
}

void SVGLength::convertToSpecifiedUnits(unsigned short unitType)
{
    if (unitType < SVG_LENGTHTYPE_NUMBER || unitType > SVG_LENGTHTYPE_PC) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::Code::NOT_SUPPORTED_ERR,
                               "NotSupportedError");
    }

    setUnitType(unitType);
}
}
