/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishCSSStyleLookupTrie__
#define __StarFishCSSStyleLookupTrie__

#include "style/Style.h"

namespace StarFish {

#define DEFINE_CSS_STYLE_KIND(name, nameLower, lowerCaseName) \
    name,


enum CSSStyleKind {
    Unknown,
    FOR_EACH_STYLE_ATTRIBUTE_TOTAL(DEFINE_CSS_STYLE_KIND)
};

#undef DEFINE_CSS_STYLE_KIND

// z-index, font-size...
CSSStyleKind lookupCSSStyle(const char* data, unsigned length);

// zIndex, fontSize...
CSSStyleKind lookupCSSStyleCamelCase(const char* data, unsigned length);

} /* namespace StarFish */

#endif /* __StarFishCSSStyleLookupTrie__ */
