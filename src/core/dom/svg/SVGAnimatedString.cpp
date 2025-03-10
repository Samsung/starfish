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
#include "SVGAnimatedString.h"
#include "core/dom/svg/SVGElement.h"

namespace Starfish {

SVGAnimatedString::SVGAnimatedString(SVGElement* targetElement,
                                     const QualifiedName& targetAttribute,
                                     String* baseVal, String* animVal)
    : ScriptWrappable(this)
    , m_targetElement(targetElement)
    , m_targetAttribute(targetAttribute)
    , m_baseVal(baseVal)
    , m_animVal(animVal)
{
}

void SVGAnimatedString::setBaseVal(String* baseVal)
{
    m_baseVal = baseVal;
    m_targetElement->updateSVGAttributeNeeded(m_targetAttribute);
}

ScriptBindingInstance* SVGAnimatedString::scriptBindingInstance()
{
    return m_targetElement->scriptBindingInstance();
}
} // namespace Starfish
