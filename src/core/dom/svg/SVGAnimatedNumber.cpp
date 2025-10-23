/*
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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
#include "SVGAnimatedNumber.h"
#include "core/dom/svg/SVGElement.h"

namespace Starfish {

SVGAnimatedNumber::SVGAnimatedNumber(SVGElement* targetElement,
                                     const QualifiedName& targetAttribute,
                                     float baseVal)
    : ScriptWrappable(this)
    , m_targetElement(targetElement)
    , m_targetAttribute(targetAttribute)
    , m_baseVal(baseVal)
{
}

float SVGAnimatedNumber::animVal() const
{
    auto val = m_targetElement->animatedLengthAttribute(
        m_targetAttribute.localNameAtomic());
    if (val) {
        return val.value().fixed();
    } else {
        return m_baseVal;
    }
}

void SVGAnimatedNumber::updateTargetElementAttribute()
{
    m_targetElement->updateSVGAttributeNeeded(m_targetAttribute);
}

ScriptBindingInstance* SVGAnimatedNumber::scriptBindingInstance()
{
    return m_targetElement->scriptBindingInstance();
}

float SVGAnimatedNumberWithFallbackAttribute::animVal() const
{
    auto val = m_targetElement->animatedLengthAttribute(
        m_targetAttribute.localNameAtomic());
    if (val) {
        return val.value().fixed();
    } else {
        val = m_targetElement->animatedLengthAttribute(
            m_fallbackAttribute.localNameAtomic());
        if (val) {
            return val.value().fixed();
        }
        return m_baseVal;
    }
}
} // namespace Starfish
