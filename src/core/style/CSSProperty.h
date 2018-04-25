/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifndef __StarFishCSSProperty__
#define __StarFishCSSProperty__

namespace StarFish {

// TODO
// Declare CSSProperty Enum here and clean up duplicated enum declarations
// (`CSSStyleValuePair::KeyKind`, `enum class CSSStyleKind`)

class CSSPropertyHelper {
public:
    static bool isAnimatable(CSSStyleValuePair::KeyKind property);
    static const char* toString(CSSStyleValuePair::KeyKind property);
    static const char* toCamelCaseString(CSSStyleValuePair::KeyKind property);
};
}

#endif
