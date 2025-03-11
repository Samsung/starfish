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

#ifndef __StarfishSVGAnimatedString__
#define __StarfishSVGAnimatedString__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class SVGElement;
class SVGAnimatedString : public ScriptWrappable {
public:
    SVGAnimatedString(SVGElement* targetElement,
                      const QualifiedName& targetAttribute, String* baseVal,
                      String* animVal);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(SVGAnimatedString)

    void setBaseVal(String* baseVal, bool fromSetAttribute = false);
    String* baseVal() const
    {
        return m_baseVal;
    }

    String* animVal() const
    {
        return m_animVal;
    }

protected:
    SVGElement* m_targetElement;
    QualifiedName m_targetAttribute;
    String* m_baseVal;
    String* m_animVal;
};
} // namespace Starfish

#endif
