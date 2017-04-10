/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "style/CSSStyleLookupTrie.h"
#include "CSSStyleDeclaration.h"

namespace StarFish {

String* CSSStyleDeclaration::getPropertyValue(String* name)
{
    const char* c = name->utf8Data();
    CSSStyleKind kind = lookupCSSStyle(c, strlen(c));
    String* val = String::emptyString;
    switch (kind) {
#define MATCH_KEY(Name, ...) \
    case CSSStyleKind::Name: \
        val = Name();        \
        break;
        FOR_EACH_STYLE_ATTRIBUTE_TOTAL(MATCH_KEY)
#undef MATCH_KEY
    default:
        break;
    }
    return val;
}

void CSSStyleDeclaration::setProperty(String* name, String* value,
                                      String* prior)
{
    bool isImportant = false;
    String* lower = prior->toLower();
    if (lower->length() > 0) {
        if (lower->equals(String::fromUTF8("important"))) {
            isImportant = true;
        } else {
            return;
        }
    }

    const char* c = name->utf8Data();
    CSSStyleKind kind = lookupCSSStyle(c, strlen(c));
    if (kind == CSSStyleKind::Unknown) {
    } else {
        if (false) {
        }
#define SET_ATTR(name, nameLower, nameCSSCase) \
    else if (kind == CSSStyleKind::name)       \
    {                                          \
        set##name(value, isImportant);         \
    }
        FOR_EACH_STYLE_ATTRIBUTE_TOTAL(SET_ATTR)
#undef SET_ATTR
    }
}
}
