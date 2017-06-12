/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSStyleLookupTrie.h"

#include <EscargotPublic.h>

namespace StarFish {

using namespace Escargot;

ExposableObjectGetOwnPropertyCallbackResult
CSSStyleDeclarationGetOwnPropertyCallback(ExecutionStateRef* state,
                                          ObjectRef* jsObj, ValueRef* key)
{
    CSSStyleDeclaration* self = (CSSStyleDeclaration*)jsObj->extraData();
    STARFISH_ASSERT(self->isCSSStyleDeclaration());

    uint32_t idx = key->toArrayIndex(state);
    if (idx < self->length()) {
        return ExposableObjectGetOwnPropertyCallbackResult(
            ValueRef::create(toJSString(self->item(idx))), true, true, false);
    }

    if (idx == ValueRef::InvalidArrayIndexValue) {
        const char* str = toBrowserString(state, key)->utf8Data();
        CSSStyleKind kind = lookupCSSStyleCamelCase(str, strlen(str));

        if (kind == CSSStyleKind::Unknown) {
            kind = lookupCSSStyle(str, strlen(str));
        }
        if (kind == CSSStyleKind::Unknown) {
            return ExposableObjectGetOwnPropertyCallbackResult();
        } else {
            if (false) {
            }
#define GET_ATTR(name, nameLower, nameCSSCase)                              \
    else if (kind == CSSStyleKind::name)                                    \
    {                                                                       \
        return ExposableObjectGetOwnPropertyCallbackResult(                 \
            ValueRef::create(createScriptString(self->name())), true, true, \
            false);                                                         \
    }
            FOR_EACH_STYLE_ATTRIBUTE_TOTAL(GET_ATTR)
#undef GET_ATTR
        }
    }

    return ExposableObjectGetOwnPropertyCallbackResult(
        ValueRef::create(StringRef::fromASCII("")), false, false, false);
}

void CSSStyleDeclarationDefineOwnPropertyCallback(ExecutionStateRef* state,
                                                  ObjectRef* jsObj,
                                                  ValueRef* key, ValueRef* val)
{
    CSSStyleDeclaration* self = (CSSStyleDeclaration*)jsObj->extraData();
    STARFISH_ASSERT(self->isCSSStyleDeclaration());
    const char* str = toBrowserString(state, key)->utf8Data();
    CSSStyleKind kind = lookupCSSStyleCamelCase(str, strlen(str));

    if (kind == CSSStyleKind::Unknown) {
        kind = lookupCSSStyle(str, strlen(str));
    }
    if (kind == CSSStyleKind::Unknown) {
        return;
    } else {
        if (false) {
        }
#define SET_ATTR(name, nameLower, nameCSSCase)               \
    else if (kind == CSSStyleKind::name)                     \
    {                                                        \
        self->set##name(toBrowserString(state, val), false); \
        return;                                              \
    }
        FOR_EACH_STYLE_ATTRIBUTE_TOTAL(SET_ATTR)
#undef SET_ATTR
    }

    return;
}

ExposableObjectEnumerationCallbackResultVector
CSSStyleDeclarationEnumerationCallback(ExecutionStateRef* state,
                                       ObjectRef* jsObj)
{
    CSSStyleDeclaration* self = (CSSStyleDeclaration*)jsObj->extraData();
    size_t len = self->length();
    ExposableObjectEnumerationCallbackResultVector v;
    for (size_t i = 0; i < len; i++) {
        v.push_back(ExposableObjectEnumerationCallbackResult(
            ValueRef::create(i), true, true, false));
    }

#define ENUM_ATTR(name, nameLower, nameCSSCase)                         \
    v.push_back(ExposableObjectEnumerationCallbackResult(               \
        ValueRef::create(StringRef::fromASCII(#nameLower)), true, true, \
        false));

    FOR_EACH_STYLE_ATTRIBUTE_TOTAL(ENUM_ATTR)
#undef ENUM_ATTR
    return v;
}
}
