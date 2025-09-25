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
#include "SVGAnimatedInteger.h"
#include "core/dom/Document.h"
#include "core/dom/svg/SVGElement.h"

namespace Starfish {

SVGAnimatedInteger::SVGAnimatedInteger(SVGElement* targetElement,
                                       const QualifiedName& targetAttribute,
                                       long baseVal)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(targetElement->scriptBindingInstance())
    , m_targetElement(targetElement)
    , m_targetAttribute(targetAttribute)
    , m_baseVal(baseVal)
{
}

void SVGAnimatedInteger::updateTargetElementAttribute()
{
    m_targetElement->updateSVGAttributeNeeded(m_targetAttribute);
}

ScriptBindingInstance* SVGAnimatedInteger::scriptBindingInstance()
{
    return m_scriptBindingInstance;
}

long SVGAnimatedInteger::animVal() const
{
    auto val = m_targetElement->getAnimatedAttribute(
        m_targetAttribute.localNameAtomic());
    if (val) {
        return String::parseFloat(val.value());
    } else {
        return m_baseVal;
    }
}
} // namespace Starfish
