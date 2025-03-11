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

#include "core/dom/svg/SVGMarkerElement.h"
#include "StarfishBase.h"
namespace Starfish {

SVGMarkerElement::SVGMarkerElement(Document* document,
                                   const QualifiedName& qname)
    : SVGElement(document, qname)
    , m_orientAngle(nullptr)
{
    m_markerUnits = new SVGAnimatedEnumeration(
        this, staticStrings()->m_markerUnits, SVG_MARKERUNITS_STROKEWIDTH,
        SVG_MARKERUNITS_STROKEWIDTH, SVG_MARKERUNITS_STROKEWIDTH);

    SVGAngle* baseVal = new SVGAngle(this, staticStrings()->m_orient);
    m_orientAngle = new SVGAnimatedAngle(document, baseVal, nullptr);
    m_orientType = new SVGAnimatedEnumeration(
        this, staticStrings()->m_orient, SVG_MARKER_ORIENT_ANGLE,
        SVG_MARKER_ORIENT_ANGLE, SVG_MARKER_ORIENT_ANGLE);
}

void SVGMarkerElement::didAttributeChanged(QualifiedName name,
                                           Optional<String*> old, String* value,
                                           bool attributeCreated,
                                           bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);

    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_orient == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
        if (m_orientType->isUpdated() == true) {
            m_orientAngle->baseVal()
                ->newValueSpecifiedUnitsWithoutUpdateAttribute(
                    SVGAngle::SVG_ANGLETYPE_UNSPECIFIED, 0);
            m_orientType->unsetUpdated();
        } else if (m_orientAngle->baseVal()->isUpdated() == true) {
            m_orientAngle->baseVal()->unsetIsUpdated();
        } else {
            auto str = this->getAttributeOrEmpty(staticStrings()->m_orient);
            if (str->equals("auto")) {
                m_orientType->setBaseValWithoutUpdateAttribute(
                    SVG_MARKER_ORIENT_AUTO);
            } else {
                m_orientType->setBaseValWithoutUpdateAttribute(
                    SVG_MARKER_ORIENT_ANGLE);
                m_orientAngle->baseVal()->setValueAsString(str, false);
            }
        }
    } else if (ss->m_markerUnits == name) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
        setNeedsPainting();
        if (m_markerUnits->isUpdated() == true) {
            m_markerUnits->unsetUpdated();
        } else {
            auto str =
                this->getAttributeOrEmpty(staticStrings()->m_markerUnits);
            if (str->equals("userSpaceOnUse")) {
                m_markerUnits->setBaseValWithoutUpdateAttribute(
                    SVG_MARKERUNITS_USERSPACEONUSE);
            } else {
                m_markerUnits->setBaseValWithoutUpdateAttribute(
                    SVG_MARKERUNITS_STROKEWIDTH);
            }
        }
    }
}

void* SVGMarkerElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGMarkerElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGMarkerElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(SVGMarkerElement, m_markerUnits));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGMarkerElement, m_orientAngle));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGMarkerElement, m_orientType));
        SVGElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGMarkerElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

SVGAnimatedEnumeration* SVGMarkerElement::markerUnits()
{
    return m_markerUnits;
}

SVGAnimatedAngle* SVGMarkerElement::orientAngle()
{
    return m_orientAngle;
}

SVGAnimatedEnumeration* SVGMarkerElement::orientType()
{
    return m_orientType;
}

void SVGMarkerElement::setOrientToAuto()
{
    m_orientType->setBaseVal(SVG_MARKER_ORIENT_AUTO);
}

void SVGMarkerElement::setOrientToAngle(SVGAngle* angle)
{
    m_orientType->setBaseValWithoutUpdateAttribute(SVG_MARKER_ORIENT_ANGLE);
    m_orientAngle->baseVal()->newValueSpecifiedUnits(
        angle->unitType(), angle->valueInSpecifiedUnits());
}
} // namespace Starfish
