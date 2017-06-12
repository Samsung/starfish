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
#include "core/dom/DOMException.h"
#include "core/dom/DOMTokenList.h"

#include <EscargotPublic.h>

namespace StarFish {

using namespace Escargot;

ExposableObjectGetOwnPropertyCallbackResult DOMTokenListGetOwnPropertyCallback(
    ExecutionStateRef* state, ObjectRef* jsObj, ValueRef* key)
{
    DOMTokenList* self = (DOMTokenList*)jsObj->extraData();
    STARFISH_ASSERT(self->isDOMTokenList());

    uint32_t idx = key->toArrayIndex(state);
    if (idx < self->length()) {
        Nullable<String*> result = self->item(idx);
        if (result.hasValue()) {
            return ExposableObjectGetOwnPropertyCallbackResult(
                ValueRef::create(toJSString(result.getValue())), false, true,
                false);
        }
    }
    return ExposableObjectGetOwnPropertyCallbackResult();
}

void DOMTokenListDefineOwnPropertyCallback(ExecutionStateRef* state,
                                           ObjectRef* self,
                                           ValueRef* propertyName,
                                           ValueRef* value)
{
    // do nothing
}

ExposableObjectEnumerationCallbackResultVector DOMTokenListEnumerationCallback(
    ExecutionStateRef* state, ObjectRef* jsObj)
{
    DOMTokenList* self = (DOMTokenList*)jsObj->extraData();
    size_t len = self->length();
    ExposableObjectEnumerationCallbackResultVector v;
    for (size_t i = 0; i < len; i++) {
        v.push_back(ExposableObjectEnumerationCallbackResult(
            ValueRef::create(i), false, true, false));
    }
    return v;
}
}
