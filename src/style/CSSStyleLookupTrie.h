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

CSSStyleKind lookupCSSStyle(const char* data, unsigned length);

} /* namespace StarFish */

#endif /* __StarFishCSSStyleLookupTrie__ */
