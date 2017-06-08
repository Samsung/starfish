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

#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSStyleLookupTrie.h"

namespace StarFish {

static ESValue readCallbackFunction(const ESValue& key, ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    CSSStyleDeclaration* self = (CSSStyleDeclaration*)obj->extraPointerData();
    STARFISH_ASSERT(self->isCSSStyleDeclaration());
    uint32_t idx = key.toIndex();
    if (idx < self->length()) {
        return ESString::create(self->item(idx)->utf8Data());
    }

    if (idx == ESValue::ESInvalidIndexValue) {
        const char* str = toBrowserString(key)->utf8Data();
        CSSStyleKind kind = lookupCSSStyleCamelCase(str, strlen(str));

        if (kind == CSSStyleKind::Unknown) {
            kind = lookupCSSStyle(str, strlen(str));
        }
        if (kind == CSSStyleKind::Unknown) {
            return ESValue(ESValue::ESDeletedValue);
        } else {
            if (false) {
            }
#define GET_ATTR(name, nameLower, nameCSSCase)   \
    else if (kind == CSSStyleKind::name)         \
    {                                            \
        return createScriptString(self->name()); \
    }
            FOR_EACH_STYLE_ATTRIBUTE_TOTAL(GET_ATTR)
#undef GET_ATTR
        }
    }

    return ESString::create("");
}

static bool writeCallbackFunction(const ESValue& key, const ESValue& val,
                                  ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    CSSStyleDeclaration* self = (CSSStyleDeclaration*)obj->extraPointerData();
    STARFISH_ASSERT(self->isCSSStyleDeclaration());
    const char* str = toBrowserString(key)->utf8Data();
    CSSStyleKind kind = lookupCSSStyleCamelCase(str, strlen(str));

    if (kind == CSSStyleKind::Unknown) {
        kind = lookupCSSStyle(str, strlen(str));
    }
    if (kind == CSSStyleKind::Unknown) {
        return false;
    } else {
        if (false) {
        }
#define SET_ATTR(name, nameLower, nameCSSCase)        \
    else if (kind == CSSStyleKind::name)              \
    {                                                 \
        self->set##name(toBrowserString(val), false); \
        return true;                                  \
    }
        FOR_EACH_STYLE_ATTRIBUTE_TOTAL(SET_ATTR)
#undef SET_ATTR
    }

    return false;
}

static ESValueVector enumerateCallbackFunction(ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    CSSStyleDeclaration* self = (CSSStyleDeclaration*)obj->extraPointerData();
    STARFISH_ASSERT(self->isCSSStyleDeclaration());
    size_t len = self->length();
    ESValueVector v(len);
    for (size_t i = 0; i < len; i++) {
        v[i] = ESValue(i);
    }

#define ENUM_ATTR(name, nameLower, nameCSSCase) \
    v.push_back(ESString::create(#nameLower));

    FOR_EACH_STYLE_ATTRIBUTE_TOTAL(ENUM_ATTR)
#undef ENUM_ATTR
    return v;
}

void CSSStyleDeclaration::postInit(ScriptBindingInstance* instance)
{
    scriptObject()->setPropertyInterceptor(readCallbackFunction,
                                           writeCallbackFunction,
                                           enumerateCallbackFunction, true);
}
}
