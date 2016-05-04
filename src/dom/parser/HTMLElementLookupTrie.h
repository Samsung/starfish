#ifndef __StarFishHTMLElementLookupTrie__
#define __StarFishHTMLElementLookupTrie__

#include "util/String.h"

namespace StarFish {

QualifiedName lookupHTMLTag(StaticStrings& s, const char32_t* data, unsigned length);

}

#endif
