/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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

#include "core/dom/svg/SVGRadialGradientElement.h"

#include "core/dom/svg/SVGStopElement.h"
#include "core/style/GradientData.h"
#include "core/style/CSSParser.h"

namespace Starfish {

GCVector<ColorStop*> SVGRadialGradientElement::colorStops()
{
    GCVector<ColorStop*> colorStops;
    for (Node* c = firstChild(); c; c = c->nextSibling()) {
        if (c->isSVGStopElement()) {
            ColorStop* colorStop = c->asSVGStopElement()->colorStop();
            colorStops.push_back(colorStop);
        }
    }

    return colorStops;
}

void SVGRadialGradientElement::didAttributeChanged(QualifiedName name,
                                                   Optional<String*> old,
                                                   String* value,
                                                   bool attributeCreated,
                                                   bool attributeRemoved)
{
    SVGGradientElement::didAttributeChanged(name, old, value, attributeCreated,
                                            attributeRemoved);
    if (!old || !old->equals(value)) {
        StaticStrings* ss = starfish()->staticStrings();
        if (ss->m_cx == name) {
            cx()->baseVal()->setValueAsString(value, true, false);
            paintingAttributesUpdated();
        } else if (ss->m_cy == name) {
            cy()->baseVal()->setValueAsString(value, true, false);
            paintingAttributesUpdated();
        } else if (ss->m_r == name) {
            r()->baseVal()->setValueAsString(value, true, false);
            paintingAttributesUpdated();
        } else if (ss->m_fx == name) {
            fx()->baseVal()->setValueAsString(value, true, false);
            paintingAttributesUpdated();
        } else if (ss->m_fy == name) {
            fy()->baseVal()->setValueAsString(value, true, false);
            paintingAttributesUpdated();
        } else if (ss->m_fr == name) {
            fr()->baseVal()->setValueAsString(value, true, false);
            paintingAttributesUpdated();
        }
    }
}

void SVGRadialGradientElement::updateSVGAttributeNeeded(QualifiedName name)
{
    SVGGradientElement::updateSVGAttributeNeeded(name);

    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_cx == name) {
        setAttribute(ss->m_cx, cx()->baseVal()->valueAsString());
    } else if (ss->m_cy == name) {
        setAttribute(ss->m_cy, cy()->baseVal()->valueAsString());
    } else if (ss->m_r == name) {
        setAttribute(ss->m_r, r()->baseVal()->valueAsString());
    } else if (ss->m_fx == name) {
        setAttribute(ss->m_fx, fx()->baseVal()->valueAsString());
    } else if (ss->m_fy == name) {
        setAttribute(ss->m_fy, fy()->baseVal()->valueAsString());
    } else if (ss->m_fr == name) {
        setAttribute(ss->m_fr, fr()->baseVal()->valueAsString());
    }
}

void SVGRadialGradientElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    SVGElement::styleForPresentationAttribute(cssValues, cssCustomValues);
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(cx, CX, cssCustomValues);
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(cy, CY, cssCustomValues);
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(r, R, cssCustomValues);
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(fx, FX, cssCustomValues);
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(fy, FY, cssCustomValues);
    STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(fr, FR, cssCustomValues);
}

void* SVGRadialGradientElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGRadialGradientElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGRadialGradientElement)] = { 0 };
        fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGRadialGradientElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

} // namespace Starfish
