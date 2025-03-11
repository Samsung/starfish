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

#ifndef __StarfishSVGAnimatedEnumeration__
#define __StarfishSVGAnimatedEnumeration__

#include "binding/DocumentHoldable.h"
#include "binding/ScriptWrappable.h"
#include "SVGUnitTypes.h"

namespace Starfish {

class SVGElement;

class SVGAnimatedEnumeration : public ScriptWrappable {
public:
    SVGAnimatedEnumeration(SVGElement* sourceElement,
                           QualifiedName targetAttribute,
                           unsigned short baseVal, unsigned short animVal,
                           unsigned short maxEnumValue);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(SVGAnimatedEnumeration)

    unsigned short baseVal();
    void setBaseVal(unsigned short baseVal);
    void setBaseValWithoutUpdateAttribute(unsigned short baseVal);
    unsigned short animVal();

    bool isUpdated();
    void unsetUpdated();
    void updateAttribute();

private:
    SVGElement* m_sourceElement;
    QualifiedName m_targetAttribute;

    unsigned short m_baseVal;
    unsigned short m_animVal;

    unsigned short m_maxEnumValue;

    bool m_updated;
};

} // namespace Starfish

#endif
