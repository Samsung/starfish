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
#include "core/dom/svg/SVGElement.h"
#include "core/dom/svg/SVGAnimatedEnumeration.h"
#include "core/dom/svg/SVGMarkerElement.h"
#include "core/dom/svg/SVGGradientElement.h"
#include "core/dom/DOMException.h"

namespace Starfish {

SVGAnimatedEnumeration::SVGAnimatedEnumeration(
    SVGElement* sourceElement, QualifiedName targetAttribute,
    unsigned short baseVal, unsigned short animVal,
    unsigned short maxEnumValue /* = 2*/)
    : ScriptWrappable(this)
    , m_sourceElement(sourceElement)
    , m_targetAttribute(targetAttribute)
    , m_baseVal(baseVal)
    , m_animVal(animVal)
    , m_maxEnumValue(maxEnumValue)
    , m_updated(false)
{
}

ScriptBindingInstance* SVGAnimatedEnumeration::scriptBindingInstance()
{
    return m_sourceElement->scriptBindingInstance();
}

unsigned short SVGAnimatedEnumeration::baseVal()
{
    return m_baseVal;
}

void SVGAnimatedEnumeration::setBaseVal(unsigned short baseVal)
{
    if (baseVal == 0 || baseVal > m_maxEnumValue) {
        throw new DOMException(
            m_sourceElement->executionContext(),
            DOMException::Code::SCRIPT_TYPE_ERR,
            "The provided enumeration value is not settable.");
    }

    m_baseVal = baseVal;
    m_animVal = baseVal;

    updateAttribute();
}

void SVGAnimatedEnumeration::setBaseValWithoutUpdateAttribute(
    unsigned short baseVal)
{
    m_baseVal = baseVal;
    m_animVal = baseVal;
}

unsigned short SVGAnimatedEnumeration::animVal()
{
    return m_animVal;
}

bool SVGAnimatedEnumeration::isUpdated()
{
    return m_updated;
}

void SVGAnimatedEnumeration::unsetUpdated()
{
    m_updated = false;
}

void SVGAnimatedEnumeration::updateAttribute()
{
    m_updated = true;

    if (m_targetAttribute.localName()->equals("orient")) {
        if (m_baseVal == SVGMarkerElement::SVG_MARKER_ORIENT_ANGLE) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("0"));
        } else if (m_baseVal == SVGMarkerElement::SVG_MARKER_ORIENT_AUTO) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("auto"));
        }
    } else if (m_targetAttribute.localName()->equals("markerUnits")) {
        if (m_baseVal == SVGMarkerElement::SVG_MARKERUNITS_USERSPACEONUSE) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("userSpaceOnUse"));
        } else if (m_baseVal == SVGMarkerElement::SVG_MARKERUNITS_STROKEWIDTH) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("strokeWidth"));
        }
    } else if (m_targetAttribute.localName()->equals("spreadMethod")) {
        if (m_baseVal == SVGGradientElement::SVG_SPREADMETHOD_PAD) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("pad"));
        } else if (m_baseVal == SVGGradientElement::SVG_SPREADMETHOD_REFLECT) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("reflect"));
        } else if (m_baseVal == SVGGradientElement::SVG_SPREADMETHOD_REPEAT) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("repeat"));
        }
    } else {
        if (m_targetAttribute.localName()->equals(String::emptyString) ==
            false) {
            if (m_baseVal ==
                SVGUnitTypes::UnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE) {
                m_sourceElement->setAttribute(
                    m_targetAttribute, String::fromUTF8("userSpaceOnUse"));
            } else {
                m_sourceElement->setAttribute(
                    m_targetAttribute, String::fromUTF8("objectBoundingBox"));
            }
        }
    }
}

} // namespace Starfish
