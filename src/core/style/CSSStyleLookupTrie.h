/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishCSSStyleLookupTrie__
#define __StarFishCSSStyleLookupTrie__

#include "core/style/Style.h"

namespace StarFish {

// z-index, font-size...
CSSStyleValuePair::KeyKind lookupCSSStyle(const char* data, unsigned length);

// zIndex, fontSize...
CSSStyleValuePair::KeyKind lookupCSSStyleCamelCase(const char* data,
                                                   unsigned length);

UnitType lookupUnitType(const char* data, unsigned length);

} /* namespace StarFish */

#endif /* __StarFishCSSStyleLookupTrie__ */
