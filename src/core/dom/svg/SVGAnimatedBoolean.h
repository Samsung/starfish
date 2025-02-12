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

#ifndef __StarfishSVGAnimatedBoolean__
#define __StarfishSVGAnimatedBoolean__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class SVGAnimatedBoolean : public ScriptWrappable {
public:
    SVGAnimatedBoolean(Document* document, bool baseVal, bool animVal);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(SVGAnimatedBoolean)

    void setBaseVal(bool baseVal)
    {
        m_baseVal = baseVal;
    }

    bool baseVal() const
    {
        return m_baseVal;
    }

    bool animVal() const
    {
        return m_animVal;
    }

protected:
    ScriptBindingInstance* m_scriptBindingInstance;
    bool m_baseVal;
    bool m_animVal;
};
} // namespace Starfish

#endif
