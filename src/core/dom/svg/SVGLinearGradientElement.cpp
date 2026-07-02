/*
 * Copyright (c) 2020-present Samsung Electronics Co., Ltd
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

#include "core/dom/svg/SVGLinearGradientElement.h"

namespace Starfish {

void SVGLinearGradientElement::computeAttributeChangeDamage(AtomicString name)
{
    SVGGradientElement::computeAttributeChangeDamage(name);

    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_x1 == name) {
        attributeOfPaintServerLikeUpdated(false);
    } else if (ss->m_y1 == name) {
        attributeOfPaintServerLikeUpdated(false);
    } else if (ss->m_x2 == name) {
        attributeOfPaintServerLikeUpdated(false);
    } else if (ss->m_y2 == name) {
        attributeOfPaintServerLikeUpdated(false);
    }
}

void SVGLinearGradientElement::didAttributeChanged(QualifiedName name,
                                                   Optional<String*> old,
                                                   String* value,
                                                   bool attributeCreated,
                                                   bool attributeRemoved)
{
    SVGGradientElement::didAttributeChanged(name, old, value, attributeCreated,
                                            attributeRemoved);

    if (!old || !old->equals(value)) {
        StaticStrings* ss = starfish()->staticStrings();
        if (ss->m_x1 == name) {
            x1()->baseVal()->setValueAsString(value, true, false);
        } else if (ss->m_y1 == name) {
            y1()->baseVal()->setValueAsString(value, true, false);
        } else if (ss->m_x2 == name) {
            x2()->baseVal()->setValueAsString(value, true, false);
        } else if (ss->m_y2 == name) {
            y2()->baseVal()->setValueAsString(value, true, false);
        }
    }
}

void SVGLinearGradientElement::updateSVGAttributeNeeded(QualifiedName name)
{
    SVGGradientElement::updateSVGAttributeNeeded(name);

    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_x1 == name) {
        setAttribute(ss->m_x1, x1()->baseVal()->valueAsString());
    } else if (ss->m_y1 == name) {
        setAttribute(ss->m_y1, y1()->baseVal()->valueAsString());
    } else if (ss->m_x2 == name) {
        setAttribute(ss->m_x2, x2()->baseVal()->valueAsString());
    } else if (ss->m_y2 == name) {
        setAttribute(ss->m_y2, y2()->baseVal()->valueAsString());
    }
}

void* SVGLinearGradientElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGLinearGradientElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGLinearGradientElement)] = { 0 };
        fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGLinearGradientElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

} // namespace Starfish
