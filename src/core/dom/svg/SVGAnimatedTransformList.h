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

#ifndef __StarfishSVGAnimatedTransformList__
#define __StarfishSVGAnimatedTransformList__

#include "binding/ScriptWrappable.h"
#include "core/dom/svg/SVGTransformList.h"

namespace Starfish {

class SVGAnimatedTransformList : public ScriptWrappable {
public:
    SVGAnimatedTransformList(Document* document, SVGTransformList* baseVal,
                             SVGTransformList* animVal);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(SVGAnimatedTransformList)

    SVGTransformList* baseVal() const;
    SVGTransformList* animVal() const;

private:
    ScriptBindingInstance* m_scriptBindingInstance;
    SVGTransformList* m_baseVal;
    SVGTransformList* m_animVal;
};
} // namespace Starfish

#endif
