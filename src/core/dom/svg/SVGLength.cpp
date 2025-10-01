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
                     Optional<SVGLength*> sourceObject)
    : ScriptWrappable(this)
    , m_sourceElement(sourceElement)
    , m_targetAttribute(targetAttribute)
    , m_sourceObject(sourceObject)
    , m_unitType(SVG_LENGTHTYPE_NUMBER)
    , m_valueInSpecifiedUnits(0)
    , m_isReadOnly(!!sourceObject)
    , m_hasSpecificValue(false)
{
}

SVGLength::SVGLength(SVGElement* sourceElement, QualifiedName targetAttribute,
                     unsigned short unitType, float value)
    : ScriptWrappable(this)
    , m_sourceElement(sourceElement)
    , m_targetAttribute(targetAttribute)
    , m_unitType(unitType)
    , m_valueInSpecifiedUnits(value)
    , m_isReadOnly(false)
    , m_hasSpecificValue(true)
{
}

void SVGLength::throwIfReadOnly()
{
    if (isReadOnly()) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::NO_MODIFICATION_ALLOWED_ERR,
                               "NoModificationAllowedError");
        return;
    }
}

void SVGLength::updateByAttribute()
{
    // set fromElementDidAttributeChanged for prevent update of attribute
    String* value = m_sourceElement->getAttributeOrEmpty(m_targetAttribute);
    setValueAsString(value, true, false);
}

ScriptBindingInstance* SVGLength::scriptBindingInstance()
{
    return m_sourceElement->scriptBindingInstance();
}

unsigned short SVGLength::unitType()
{
    if (m_sourceObject) {
        auto s = m_sourceElement->animatedAttribute(
            m_targetAttribute.localNameAtomic());
        if (s) {
            SVGLength len(m_sourceElement, m_targetAttribute);
            len.setValueAsString(s.value().toString(), true, false);
            return len.unitType();
        }
        return m_sourceObject->unitType();
    }
    return m_unitType;
}

bool SVGLength::hasSpecificValue()
{
    if (m_sourceObject) {
        return m_sourceObject->hasSpecificValue();
    }
    return m_hasSpecificValue;
}

void SVGLength::setUnitType(unsigned short unitType)
{
    throwIfReadOnly();
    m_unitType = unitType;
}

float SVGLength::value(bool layoutIfNeeded)
{
    if (m_sourceObject) {
        auto s = m_sourceElement->animatedAttribute(
            m_targetAttribute.localNameAtomic());
        if (s) {
            SVGLength len(m_sourceElement, m_targetAttribute);
            len.setValueAsString(s.value().toString(), true, false);
            return value(len.unitType(), len.valueInSpecifiedUnits(false));
        }
        return m_sourceObject->value(layoutIfNeeded);
    }
    updateByAttribute();

    if (layoutIfNeeded) {
        m_sourceElement->document()->browsingContext()->layoutIfNeeded();
    }

    return value(m_unitType, m_valueInSpecifiedUnits);
}

void SVGLength::setValue(float v)
{
    throwIfReadOnly();

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

float SVGLength::value(unsigned short unitType, float rawValue)
{
    if (unitType == SVG_LENGTHTYPE_PERCENTAGE) {
        Length len = Length(Length::Percent, rawValue / 100.0);
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
        return rawValue;
        // unimplemented EMS, EXS
        if (unitType == SVG_LENGTHTYPE_NUMBER) {
            return rawValue;
        } else if (unitType == SVG_LENGTHTYPE_PX) {
            return rawValue;
        } else if (unitType == SVG_LENGTHTYPE_CM) {
            return UnitHelper::convertFromCmToPx(rawValue);
        } else if (unitType == SVG_LENGTHTYPE_MM) {
            return UnitHelper::convertFromMmToPx(rawValue);
        } else if (unitType == SVG_LENGTHTYPE_IN) {
            return UnitHelper::convertFromInToPx(rawValue);
        } else if (unitType == SVG_LENGTHTYPE_PT) {
            return UnitHelper::convertFromPtToPx(rawValue);
        } else if (unitType == SVG_LENGTHTYPE_PC) {
            return UnitHelper::convertFromPcToPx(rawValue);
        } else {
            STARFISH_UNSUPPORTED("Unsupported unit type in svg (type: %d)",
                                 (int)unitType);
        }
    }
    return 0;
}

float SVGLength::valueInSpecifiedUnits(bool layoutIfNeeded)
{
    if (m_sourceObject) {
        auto s = m_sourceElement->animatedAttribute(
            m_targetAttribute.localNameAtomic());
        if (s) {
            SVGLength len(m_sourceElement, m_targetAttribute);
            len.setValueAsString(s.value().toString(), true, false);
            return len.valueInSpecifiedUnits(false);
        }
        return m_sourceObject->valueInSpecifiedUnits(layoutIfNeeded);
    }
    if (layoutIfNeeded) {
        m_sourceElement->document()->browsingContext()->layoutIfNeeded();
    }
    return m_valueInSpecifiedUnits;
}

void SVGLength::setValueInSpecifiedUnits(float v,
                                         bool fromElementDidAttributeChanged)
{
    throwIfReadOnly();

    if (std::isnan(v) || std::isinf(v)) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "The provided float value is non-finite");
        return;
    }
    m_valueInSpecifiedUnits = v;
    m_hasSpecificValue = true;

    if (!fromElementDidAttributeChanged) {
        m_sourceElement->updateSVGAttributeNeeded(m_targetAttribute);
    }
}

String* SVGLength::valueAsString(bool layoutIfNeeded)
{
    String* str = String::fromFloat(valueInSpecifiedUnits(layoutIfNeeded));
    auto unitType = this->unitType();

    // unimplemented EMS, EXS
    if (unitType == SVG_LENGTHTYPE_NUMBER) {
    } else if (unitType == SVG_LENGTHTYPE_PX) {
        str = str->concat("px");
    } else if (unitType == SVG_LENGTHTYPE_CM) {
        str = str->concat("cm");
    } else if (unitType == SVG_LENGTHTYPE_MM) {
        str = str->concat("mm");
    } else if (unitType == SVG_LENGTHTYPE_IN) {
        str = str->concat("in");
    } else if (unitType == SVG_LENGTHTYPE_PT) {
        str = str->concat("pt");
    } else if (unitType == SVG_LENGTHTYPE_PC) {
        str = str->concat("pc");
    } else if (unitType == SVG_LENGTHTYPE_PERCENTAGE) {
        str = str->concat("%");
    } else {
        STARFISH_UNSUPPORTED("Unsupported unit type in svg (type: %d)",
                             (int)m_unitType);
    }

    return str;
}

void SVGLength::setValueAsString(String* valueAsString,
                                 bool fromElementDidAttributeChanged,
                                 bool throwDOMExceptionOnFailure)
{
    throwIfReadOnly();

    valueAsString = valueAsString->toLower();

    if (valueAsString->length()) {
        auto s = valueAsString->toUTF8NonGCString();
        CSSStyleValuePair pair;
        float v;
        if (CSSPropertyParser::parseNumber(s.data(), s.length(), 0, &v)) {
            setUnitType(SVG_LENGTHTYPE_NUMBER);
            setValueInSpecifiedUnits(v, fromElementDidAttributeChanged);
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
                setValueInSpecifiedUnits(pair.cssLengthValue().value(),
                                         fromElementDidAttributeChanged);
            } else if (pair.valueKind() == CSSStyleValuePair::Percentage) {
                setUnitType(SVG_LENGTHTYPE_PERCENTAGE);
                setValueInSpecifiedUnits(pair.percentageValue() * 100,
                                         fromElementDidAttributeChanged);
            } else {
                if (throwDOMExceptionOnFailure) {
                    throw new DOMException(
                        m_sourceElement->executionContext(),
                        DOMException::Code::NOT_SUPPORTED_ERR,
                        "Not Supported error");
                }
            }
        } else {
            if (throwDOMExceptionOnFailure) {
                throw new DOMException(m_sourceElement->executionContext(),
                                       DOMException::Code::SYNTAX_ERR,
                                       "SyntaxError");
            }
        }
    } else {
        if (throwDOMExceptionOnFailure) {
            throw new DOMException(m_sourceElement->executionContext(),
                                   DOMException::Code::SYNTAX_ERR,
                                   "SyntaxError");
        }
        setUnitType(SVG_LENGTHTYPE_NUMBER);
        setValueInSpecifiedUnits(0, fromElementDidAttributeChanged);
    }
}

void SVGLength::newValueSpecifiedUnits(unsigned short unitType,
                                       float valueInSpecifiedUnits)
{
    throwIfReadOnly();

    if (std::isnan(valueInSpecifiedUnits) ||
        std::isinf(valueInSpecifiedUnits)) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::Code::SCRIPT_TYPE_ERR,
                               "The provided float value is non-finite");
    }

    convertToSpecifiedUnits(unitType);
    setValueInSpecifiedUnits(valueInSpecifiedUnits, false);
}

void SVGLength::convertToSpecifiedUnits(unsigned short unitType)
{
    throwIfReadOnly();

    if (unitType < SVG_LENGTHTYPE_NUMBER || unitType > SVG_LENGTHTYPE_PC) {
        throw new DOMException(m_sourceElement->executionContext(),
                               DOMException::Code::NOT_SUPPORTED_ERR,
                               "NotSupportedError");
    }

    setUnitType(unitType);
}

bool SVGLength::isReadOnly()
{
    return m_isReadOnly;
}

void SVGLength::setReadOnly()
{
    m_isReadOnly = true;
}

void SVGLength::detach()
{
    throwIfReadOnly();

    // Set the SVGLength to no longer be associated with any element.
    m_sourceElement = nullptr;
    m_targetAttribute = AtomicString::emptyAtomicString();

    // If the SVGLength is read only, set it to be no longer read only. Set the
    // SVGLength to have unspecified directionality.
    if (isReadOnly()) {
        m_isReadOnly = false;
    }
}

void SVGLength::attach(SVGElement* sourceElement, QualifiedName targetAttribute)
{
    throwIfReadOnly();
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
