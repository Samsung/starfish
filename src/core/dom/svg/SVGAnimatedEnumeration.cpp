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
#include "core/dom/svg/SVGComponentTransferFunctionElement.h"
#include "core/dom/svg/SVGFEColorMatrixElement.h"
#include "core/dom/svg/SVGFEGaussianBlurElement.h"
#include "core/dom/DOMException.h"

namespace Starfish {

SVGAnimatedEnumeration::SVGAnimatedEnumeration(SVGElement* sourceElement,
                                               QualifiedName targetAttribute,
                                               unsigned short baseVal,
                                               unsigned short animVal,
                                               unsigned short maxEnumValue)
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

    if (m_targetAttribute ==
            m_sourceElement->starfish()->staticStrings()->m_clipPathUnits ||
        m_targetAttribute ==
            m_sourceElement->starfish()->staticStrings()->m_filterUnits ||
        m_targetAttribute ==
            m_sourceElement->starfish()->staticStrings()->m_primitiveUnits) {
        if (m_baseVal ==
            SVGUnitTypes::UnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("userSpaceOnUse"));
        } else {
            STARFISH_ASSERT(m_baseVal == 2);
            m_sourceElement->setAttribute(
                m_targetAttribute, String::fromUTF8("objectBoundingBox"));
        }
    } else if (m_targetAttribute ==
               m_sourceElement->starfish()->staticStrings()->m_orient) {
        if (m_baseVal == SVGMarkerElement::SVG_MARKER_ORIENT_ANGLE) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("0"));
        } else if (m_baseVal == SVGMarkerElement::SVG_MARKER_ORIENT_AUTO) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("auto"));
        } else {
            STARFISH_ASSERT(m_baseVal == 0);
        }
    } else if (m_targetAttribute ==
               m_sourceElement->starfish()->staticStrings()->m_markerUnits) {
        if (m_baseVal == SVGMarkerElement::SVG_MARKERUNITS_USERSPACEONUSE) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("userSpaceOnUse"));
        } else if (m_baseVal == SVGMarkerElement::SVG_MARKERUNITS_STROKEWIDTH) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("strokeWidth"));
        } else {
            STARFISH_ASSERT(m_baseVal == 0);
        }
    } else if (m_targetAttribute ==
               m_sourceElement->starfish()->staticStrings()->m_spreadMethod) {
        if (m_baseVal == SVGGradientElement::SVG_SPREADMETHOD_PAD) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("pad"));
        } else if (m_baseVal == SVGGradientElement::SVG_SPREADMETHOD_REFLECT) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("reflect"));
        } else if (m_baseVal == SVGGradientElement::SVG_SPREADMETHOD_REPEAT) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("repeat"));
        } else {
            STARFISH_ASSERT(m_baseVal == 0);
        }
    } else if (m_targetAttribute ==
               m_sourceElement->starfish()->staticStrings()->m_edgeMode) {
        if (m_baseVal ==
            SVGFEGaussianBlurElement::EdgeMode::SVG_EDGEMODE_DUPLICATE) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("duplicate"));
        } else if (m_baseVal ==
                   SVGFEGaussianBlurElement::EdgeMode::SVG_EDGEMODE_WRAP) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("wrap"));
        } else if (m_baseVal ==
                   SVGFEGaussianBlurElement::EdgeMode::SVG_EDGEMODE_NONE) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("none"));
        } else if (m_baseVal ==
                   SVGFEGaussianBlurElement::EdgeMode::SVG_EDGEMODE_MIRROR) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("mirror"));
        } else {
            STARFISH_ASSERT(m_baseVal == 0);
        }
    } else if (m_sourceElement->isSVGComponentTransferFunctionElement() &&
               m_targetAttribute ==
                   m_sourceElement->starfish()->staticStrings()->m_type) {
        if (m_baseVal ==
            SVGComponentTransferFunctionElement::ComponentTransferType::
                SVG_FECOMPONENTTRANSFER_TYPE_IDENTITY) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("identity"));
        } else if (m_baseVal ==
                   SVGComponentTransferFunctionElement::ComponentTransferType::
                       SVG_FECOMPONENTTRANSFER_TYPE_TABLE) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("table"));
        } else if (m_baseVal ==
                   SVGComponentTransferFunctionElement::ComponentTransferType::
                       SVG_FECOMPONENTTRANSFER_TYPE_DISCRETE) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("discrete"));
        } else if (m_baseVal ==
                   SVGComponentTransferFunctionElement::ComponentTransferType::
                       SVG_FECOMPONENTTRANSFER_TYPE_LINEAR) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("linear"));
        } else if (m_baseVal ==
                   SVGComponentTransferFunctionElement::ComponentTransferType::
                       SVG_FECOMPONENTTRANSFER_TYPE_GAMMA) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("gamma"));
        } else {
            STARFISH_ASSERT(m_baseVal == 0);
        }
    } else if (m_sourceElement->isSVGFEColorMatrixElement() &&
               m_targetAttribute ==
                   m_sourceElement->starfish()->staticStrings()->m_type) {
        if (m_baseVal == SVGFEColorMatrixElement::MatrixTypes::
                             SVG_FECOLORMATRIX_TYPE_MATRIX) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("matrix"));
        } else if (m_baseVal == SVGFEColorMatrixElement::MatrixTypes::
                                    SVG_FECOLORMATRIX_TYPE_SATURATE) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("saturate"));
        } else if (m_baseVal == SVGFEColorMatrixElement::MatrixTypes::
                                    SVG_FECOLORMATRIX_TYPE_HUEROTATE) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("hueRotate"));
        } else if (m_baseVal == SVGFEColorMatrixElement::MatrixTypes::
                                    SVG_FECOLORMATRIX_TYPE_LUMINANCETOALPHA) {
            m_sourceElement->setAttribute(m_targetAttribute,
                                          String::fromUTF8("luminanceToAlpha"));
        } else {
            STARFISH_ASSERT(m_baseVal == 0);
        }
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }
}

} // namespace Starfish
