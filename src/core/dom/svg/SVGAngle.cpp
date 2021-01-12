/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
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
#include "SVGAngle.h"
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

SVGAngle::SVGAngle(SVGElement* sourceElement, QualifiedName targetAttribute)
    : ScriptWrappable(this)
    , m_sourceElement(sourceElement)
    , m_targetAttribute(targetAttribute)
    , m_unitType(SVG_ANGLETYPE_UNSPECIFIED)
{
}

ScriptBindingInstance* SVGAngle::scriptBindingInstance()
{
    return m_sourceElement->scriptBindingInstance();
}

static Nullable<Angle> valueToAngle(CSSStyleValuePair::ValueKind kind,
                                    CSSStyleValuePair::ValueData data)
{
    if (kind == CSSStyleValuePair::ValueKind::Auto) {
        return Nullable<Angle>();
    } else if (kind == CSSStyleValuePair::ValueKind::Angle) {
        return data.m_angle.toAngle();
    } else if (kind == CSSStyleValuePair::ValueKind::Number) {
        return Angle(Angle::Fixed, data.m_floatValue);
    } else if (kind == CSSStyleValuePair::ValueKind::CalcValueKind) {
        CalcValueType type = data.m_calc->type();
        if (type.isAngle() || type.isPercentage()) {
            return Angle(data.m_calc);
        } else {
            return Nullable<Angle>();
        }
    } else {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return Nullable<Angle>();
    }
}

unsigned short SVGAngle::unitType()
{
    return m_unitType;
}

void SVGAngle::setUnitType(unsigned short unitType)
{
    float v = value();
    m_unitType = unitType;
    setValue(v);
}

float SVGAngle::value()
{
    if (m_unitType == SVG_ANGLETYPE_UNSPECIFIED) {
        return valueInSpecifiedUnits();
    } else if (m_unitType == SVG_ANGLETYPE_DEG) {
        return valueInSpecifiedUnits();
    } else if (m_unitType == SVG_ANGLETYPE_RAD) {
        return UnitHelper::convertFromRadToDeg(valueInSpecifiedUnits());
    } else if (m_unitType == SVG_ANGLETYPE_GRAD) {
        return UnitHelper::convertFromGradToDeg(valueInSpecifiedUnits());
    }
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return 0;
}

void SVGAngle::setValue(float v)
{
    if (m_unitType == SVG_ANGLETYPE_UNSPECIFIED) {
        setValueInSpecifiedUnits(v);
    } else if (m_unitType == SVG_ANGLETYPE_DEG) {
        setValueInSpecifiedUnits(v);
    } else if (m_unitType == SVG_ANGLETYPE_RAD) {
        setValueInSpecifiedUnits(UnitHelper::convertFromDegToRad(v));
    } else if (m_unitType == SVG_ANGLETYPE_GRAD) {
        setValueInSpecifiedUnits(UnitHelper::convertFromDegToGrad(v));
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
}

float SVGAngle::valueInSpecifiedUnits()
{
    m_sourceElement->document()->browsingContext()->layoutIfNeeded();

    String* attrValue = m_sourceElement->getAttributeOrEmpty(m_targetAttribute);
    Angle ang;
    if (attrValue->length()) {
        auto s = attrValue->toUTF8NonGCString();
        CSSStyleValuePair pair;
        if (CSSPropertyParser::parseAngle(
                s.data(),
                CSSPropertyParser::AllowNegative |
                    CSSPropertyParser::AllowWithoutUnit,
                &pair)) {
            Nullable<Angle> value =
                valueToAngle(pair.valueKind(), pair.value());
            if (value.hasValue()) {
                ang = value.getValue();
            }
        }
    }

    if (ang.isSpecified()) {
        return ang.specifiedValue();
    }
    return 0;
}

void SVGAngle::setValueInSpecifiedUnits(float v)
{
    if (std::isnan(v) || std::isinf(v)) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "The provided float value is non-finite");
    }

    m_sourceElement->setAttribute(m_targetAttribute, String::fromFloat(v));
}

String* SVGAngle::valueAsString()
{
    String* str = String::fromFloat(valueInSpecifiedUnits());

    if (m_unitType == SVG_ANGLETYPE_UNSPECIFIED) {
    } else if (m_unitType == SVG_ANGLETYPE_DEG) {
        str = str->concat("deg");
    } else if (m_unitType == SVG_ANGLETYPE_RAD) {
        str = str->concat("rad");
    } else if (m_unitType == SVG_ANGLETYPE_GRAD) {
        str = str->concat("grad");
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    return str;
}

void SVGAngle::setValueAsString(String* valueAsString)
{
    if (valueAsString->length()) {
        auto s = valueAsString->toUTF8NonGCString();
        CSSStyleValuePair pair;
        if (CSSPropertyParser::parseAngle(
                s.data(),
                CSSPropertyParser::AllowNegative |
                    CSSPropertyParser::AllowWithoutUnit,
                &pair)) {
            if (pair.angleValue().kind() == CSSAngle::UNSPECIFIED) {
                setUnitType(SVG_ANGLETYPE_UNSPECIFIED);
            } else if (pair.angleValue().kind() == CSSAngle::DEG) {
                setUnitType(SVG_ANGLETYPE_DEG);
            } else if (pair.angleValue().kind() == CSSAngle::RAD) {
                setUnitType(SVG_ANGLETYPE_RAD);
            } else if (pair.angleValue().kind() == CSSAngle::GRAD) {
                setUnitType(SVG_ANGLETYPE_GRAD);
            } else {
                throw new DOMException(m_sourceElement->executionContext(),
                                       DOMException::Code::NOT_SUPPORTED_ERR,
                                       "Not Supported error");
            }
            setValueInSpecifiedUnits(pair.angleValue().value());
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

void SVGAngle::newValueSpecifiedUnits(unsigned short unitType,
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

void SVGAngle::convertToSpecifiedUnits(unsigned short unitType)
{
    if (unitType < SVG_ANGLETYPE_UNSPECIFIED || unitType > SVG_ANGLETYPE_GRAD) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::Code::NOT_SUPPORTED_ERR,
                               "NotSupportedError");
    }

    setUnitType(unitType);
}
} // namespace Starfish
