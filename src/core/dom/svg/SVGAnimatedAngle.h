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

#ifndef __StarfishSVGAnimatedAngle__
#define __StarfishSVGAnimatedAngle__

#include "binding/ScriptWrappable.h"
#include "core/dom/svg/SVGAngle.h"

namespace Starfish {

class SVGAnimatedAngle : public ScriptWrappable {
public:
    SVGAnimatedAngle(Document* document, SVGAngle* baseVal, SVGAngle* animVal);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(SVGAnimatedAngle)

    SVGAngle* baseVal() const
    {
        return m_baseVal;
    }

    SVGAngle* animVal() const
    {
        return m_animVal;
    }

protected:
    ScriptBindingInstance* m_scriptBindingInstance;
    SVGAngle* m_baseVal;
    SVGAngle* m_animVal;
};
} // namespace Starfish

#endif
