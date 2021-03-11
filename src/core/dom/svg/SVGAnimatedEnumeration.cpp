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
#include "SVGElement.h"
#include "SVGAnimatedEnumeration.h"
#include "SVGMarkerElement.h"

namespace Starfish {

SVGAnimatedEnumeration::SVGAnimatedEnumeration(SVGElement* sourceElement,
                                               QualifiedName targetAttribute,
                                               unsigned short baseVal,
                                               unsigned short animVal)
    : ScriptWrappable(this)
    , m_sourceElement(sourceElement)
    , m_targetAttribute(targetAttribute)
    , m_baseVal(baseVal)
    , m_animVal(animVal)
    , m_maxEnumValue(2)
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

    m_updated = true;

    if (m_targetAttribute.localName()->equals("orient")) {
        if (baseVal == SVGMarkerElement::SVG_MARKER_ORIENT_ANGLE) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("0"));
        } else if (baseVal == SVGMarkerElement::SVG_MARKER_ORIENT_AUTO) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("auto"));
        }
    } else if (m_targetAttribute.localName()->equals("markerUnits")) {
        if (baseVal == SVGMarkerElement::SVG_MARKERUNITS_USERSPACEONUSE) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("userSpaceOnUse"));
        } else if (baseVal == SVGMarkerElement::SVG_MARKERUNITS_STROKEWIDTH) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("strokeWidth"));
        }
    } else {
        if (m_targetAttribute.localName()->equals(String::emptyString) ==
            false) {
            if (baseVal ==
                SVGUnitTypes::UnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE) {
                m_sourceElement->setAttribute(
                    m_targetAttribute, String::fromUTF8("userSpaceOnUse"));
            } else {
                m_sourceElement->setAttribute(
                    m_targetAttribute, String::fromUTF8("objectBoundingBox"));
            }
        }
    }
    m_baseVal = baseVal;
}

void SVGAnimatedEnumeration::setBaseValWithoutUpdateAttribute(
    unsigned short baseVal)
{
    m_baseVal = baseVal;
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

} // namespace Starfish
