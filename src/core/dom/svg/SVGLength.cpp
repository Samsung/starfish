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
#include "SVGLength.h"
#include "core/style/ComputedStyle.h"
#include "core/style/CalcData.h"
#include "core/style/CSSParser.h"
#include "core/layout/FrameBox.h"
#include "core/layout/svg/FrameSVGSVGBox.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/dom/svg/SVGDocument.h"
#include "core/dom/DOMException.h"
#include "core/dom/svg/SVGTextElement.h"

namespace Starfish {

SVGLength::SVGLength(SVGElement* sourceElement, QualifiedName targetAttribute,
                     unsigned short unitType, float value)
    : ScriptWrappable(this)
    , m_sourceElement(sourceElement)
    , m_targetAttribute(targetAttribute)
    , m_unitType(unitType)
    , m_valueInSpecifiedUnits(value)
    , m_readOnly(false)
{
}

ScriptBindingInstance* SVGLength::scriptBindingInstance()
{
    if (m_sourceElement == nullptr) {
        return nullptr;
    }
    return m_sourceElement->scriptBindingInstance();
}

unsigned short SVGLength::unitType()
{
    return m_unitType;
}

void SVGLength::setUnitType(unsigned short unitType)
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return;
    }

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
        CalcValueType type = data.m_calc->calcValueType();
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
    if (m_unitType == SVG_LENGTHTYPE_PERCENTAGE) {
        Length len = Length(Length::Percent, m_valueInSpecifiedUnits / 100.0);
        FrameBox* cb =
            m_sourceElement->frame()
                ? m_sourceElement->frame()->layoutParent()->asFrameBox()
                : nullptr;
        FrameBox* svgBox = nullptr;
        if (cb) {
            svgBox = cb;
            while (svgBox != nullptr) {
                if (svgBox->isFrameReplaced() &&
                    svgBox->asFrameReplaced()->isFrameSVGSVGBox()) {
                    break;
                }
                if (svgBox->layoutParent() != nullptr) {
                    svgBox = svgBox->layoutParent()->asFrameBox();
                } else {
                    break;
                }
            }
        }

        LayoutUnit result = len.specifiedValue(
            cb ? (float)cb->contentWidth() : 0.f, m_sourceElement);
        if (svgBox) {
            result = result * ((FrameSVGSVGBox*)svgBox)->svgScale();
        }
        return result;
    } else {
        // unimplemented EMS, EXS
        if (m_unitType == SVG_LENGTHTYPE_NUMBER) {
            return valueInSpecifiedUnits();
        } else if (m_unitType == SVG_LENGTHTYPE_PX) {
            return valueInSpecifiedUnits();
        } else if (m_unitType == SVG_LENGTHTYPE_CM) {
            return UnitHelper::convertFromCmToPx(valueInSpecifiedUnits());
        } else if (m_unitType == SVG_LENGTHTYPE_MM) {
            return UnitHelper::convertFromMmToPx(valueInSpecifiedUnits());
        } else if (m_unitType == SVG_LENGTHTYPE_IN) {
            return UnitHelper::convertFromInToPx(valueInSpecifiedUnits());
        } else if (m_unitType == SVG_LENGTHTYPE_PT) {
            return UnitHelper::convertFromPtToPx(valueInSpecifiedUnits());
        } else if (m_unitType == SVG_LENGTHTYPE_PC) {
            return UnitHelper::convertFromPcToPx(valueInSpecifiedUnits());
        } else {
            STARFISH_UNSUPPORTED("Unsupported unit type in svg (type: %d)",
                                 (int)m_unitType);
        }
    }
    return 0;
}

void SVGLength::setValue(float v)
{
    // unimplemented PERCENTAGE, EMS, EXS
    if (m_unitType == SVG_LENGTHTYPE_NUMBER) {
        setValueInSpecifiedUnits(v);
    } else if (m_unitType == SVG_LENGTHTYPE_PX) {
        setValueInSpecifiedUnits(v);
    } else if (m_unitType == SVG_LENGTHTYPE_CM) {
        setValueInSpecifiedUnits(UnitHelper::convertFromPxToCm(v));
    } else if (m_unitType == SVG_LENGTHTYPE_MM) {
        setValueInSpecifiedUnits(UnitHelper::convertFromPxToMm(v));
    } else if (m_unitType == SVG_LENGTHTYPE_IN) {
        setValueInSpecifiedUnits(UnitHelper::convertFromPxToIn(v));
    } else if (m_unitType == SVG_LENGTHTYPE_PT) {
        setValueInSpecifiedUnits(UnitHelper::convertFromPxToPt(v));
    } else if (m_unitType == SVG_LENGTHTYPE_PC) {
        setValueInSpecifiedUnits(UnitHelper::convertFromPxToPc(v));
    } else {
        STARFISH_UNSUPPORTED("Unsupported unit type in svg (type: %d)",
                             (int)m_unitType);
    }
}

float SVGLength::valueInSpecifiedUnits()
{
    m_sourceElement->document()->browsingContext()->layoutIfNeeded();
    return m_valueInSpecifiedUnits;
}

void SVGLength::setValueInSpecifiedUnits(float v)
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return;
    }

    if (std::isnan(v) || std::isinf(v)) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "The provided float value is non-finite");
        return;
    }

    m_valueInSpecifiedUnits = v;

    m_sourceElement->updateSVGAttributeNeeded(m_targetAttribute);
}

String* SVGLength::valueAsString()
{
    String* str = String::fromFloat(valueInSpecifiedUnits());

    // unimplemented EMS, EXS
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
    } else if (m_unitType == SVG_LENGTHTYPE_PERCENTAGE) {
        str = str->concat("%");
    } else {
        STARFISH_UNSUPPORTED("Unsupported unit type in svg (type: %d)",
                             (int)m_unitType);
    }

    return str;
}

void SVGLength::setValueAsString(String* valueAsString)
{
    valueAsString = valueAsString->toLower();

    if (valueAsString->length()) {
        auto s = valueAsString->toUTF8NonGCString();
        CSSStyleValuePair pair;
        float v;
        if (CSSPropertyParser::parseNumber(s.data(), 0, &v)) {
            setUnitType(SVG_LENGTHTYPE_NUMBER);
            setValueInSpecifiedUnits(v);
        } else if (CSSPropertyParser::parseLength(
                       s.data(),
                       CSSPropertyParser::AllowPercent |
                           CSSPropertyParser::AllowWithoutUnit |
                           CSSPropertyParser::AllowNegative,
                       &pair)) {
            // unimplemented EMS, EXS
            if (pair.valueKind() == CSSStyleValuePair::Length) {
                if (pair.cssLengthValue().kind() == CSSLength::PX) {
                    setUnitType(SVG_LENGTHTYPE_PX);
                } else if (pair.cssLengthValue().kind() == CSSLength::CM) {
                    setUnitType(SVG_LENGTHTYPE_CM);
                } else if (pair.cssLengthValue().kind() == CSSLength::MM) {
                    setUnitType(SVG_LENGTHTYPE_MM);
                } else if (pair.cssLengthValue().kind() == CSSLength::INCH) {
                    setUnitType(SVG_LENGTHTYPE_IN);
                } else if (pair.cssLengthValue().kind() == CSSLength::PC) {
                    setUnitType(SVG_LENGTHTYPE_PC);
                } else if (pair.cssLengthValue().kind() == CSSLength::PT) {
                    setUnitType(SVG_LENGTHTYPE_PT);
                } else {
                    STARFISH_UNSUPPORTED(
                        "Unsupported unit type in svg (type: %d)",
                        (int)pair.cssLengthValue().kind());
                    setUnitType(SVG_LENGTHTYPE_PX);
                }
                setValueInSpecifiedUnits(pair.cssLengthValue().value());
            } else if (pair.valueKind() == CSSStyleValuePair::Percentage) {
                setUnitType(SVG_LENGTHTYPE_PERCENTAGE);
                setValueInSpecifiedUnits(pair.percentageValue() * 100);
            } else {
                throw new DOMException(m_sourceElement->executionContext(),
                                       DOMException::Code::NOT_SUPPORTED_ERR,
                                       "Not Supported error");
            }
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

bool SVGLength::isReadOnly()
{
    return m_readOnly;
}

void SVGLength::setReadOnly()
{
    m_readOnly = true;
}

void SVGLength::detach()
{
    // Set the SVGLength to no longer be associated with any element.
    m_sourceElement = nullptr;
    m_targetAttribute = AtomicString::emptyAtomicString();

    // If the SVGLength is read only, set it to be no longer read only. Set the
    // SVGLength to have unspecified directionality.
    if (isReadOnly()) {
        m_readOnly = false;
    }
}

void SVGLength::attach(SVGElement* sourceElement, QualifiedName targetAttribute)
{
    // Associate the SVGLength with the element that the list interface object
    // is associated with and set its directionality to that specified by the
    // attribute being reflected.
    m_sourceElement = sourceElement;
    m_targetAttribute = targetAttribute;
}

bool SVGLength::isDetached()
{
    return m_targetAttribute.toString()->equals(
        AtomicString::emptyAtomicString());
}
} // namespace Starfish
