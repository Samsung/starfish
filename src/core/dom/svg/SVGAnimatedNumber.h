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

#ifndef __StarfishSVGAnimatedNumber__
#define __StarfishSVGAnimatedNumber__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class SVGAnimatedNumberList;
class SVGAnimatedNumber : public ScriptWrappable {
public:
    SVGAnimatedNumber(SVGElement* targetElement,
                      const QualifiedName& targetAttribute, float baseVal);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(SVGAnimatedNumber)

    void setBaseVal(float baseVal, bool fromSetAttribute = false)
    {
        m_baseVal = baseVal;
        if (!fromSetAttribute) {
            updateTargetElementAttribute();
        }
    }

    float baseVal() const
    {
        return m_baseVal;
    }

    virtual float animVal() const;

protected:
    void updateTargetElementAttribute();
    SVGElement* m_targetElement;
    QualifiedName m_targetAttribute;
    float m_baseVal;
};

class SVGAnimatedNumberWithFallbackAttribute : public SVGAnimatedNumber {
public:
    SVGAnimatedNumberWithFallbackAttribute(
        SVGElement* targetElement, const QualifiedName& targetAttribute,
        const QualifiedName& fallbackAttribute, float baseVal)
        : SVGAnimatedNumber(targetElement, targetAttribute, baseVal)
        , m_fallbackAttribute(fallbackAttribute)
    {
    }

    virtual float animVal() const override;

protected:
    QualifiedName m_fallbackAttribute;
};
} // namespace Starfish

#endif
