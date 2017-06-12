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
#include "core/dom/Attr.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/NamedNodeMap.h"

#include <EscargotPublic.h>
using namespace Escargot;

namespace StarFish {

ExposableObjectGetOwnPropertyCallbackResult NamedNodeMapGetOwnPropertyCallback(
    ExecutionStateRef* state, ObjectRef* jsObj, ValueRef* key)
{
    NamedNodeMap* self = (NamedNodeMap*)jsObj->extraData();
    STARFISH_ASSERT(self->isNamedNodeMap());

    uint32_t idx = key->toArrayIndex(state);
    if (idx == ValueRef::InvalidArrayIndexValue) {
        ObjectRef* ref = jsObj->getPrototypeObject();
        while (ref) {
            if (ref->hasOwnProperty(state, key)) {
                return ExposableObjectGetOwnPropertyCallbackResult();
            }
            ref = ref->getPrototypeObject();
        }

        String* str = toBrowserString(state, key);
        auto attrName = self->element()->document()->createAttributeName(str);
        Attr* e = self->getNamedItem(attrName);
        if (e != nullptr) {
            return ExposableObjectGetOwnPropertyCallbackResult(
                e->scriptValue(), false, false, false);
        }
    } else if (idx < self->length()) {
        return ExposableObjectGetOwnPropertyCallbackResult(
            self->item(idx)->scriptValue(), false, true, false);
    }

    return ExposableObjectGetOwnPropertyCallbackResult();
}

void NamedNodeMapDefineOwnPropertyCallback(ExecutionStateRef* state,
                                           ObjectRef* jsObj, ValueRef* key,
                                           ValueRef* val)
{
    return;
}

ExposableObjectEnumerationCallbackResultVector NamedNodeMapEnumerationCallback(
    ExecutionStateRef* state, ObjectRef* jsObj)
{
    NamedNodeMap* self = (NamedNodeMap*)jsObj->extraData();
    size_t len = self->length();
    ExposableObjectEnumerationCallbackResultVector v;
    for (size_t i = 0; i < len; i++) {
        v.push_back(ExposableObjectEnumerationCallbackResult(
            ValueRef::create(i), false, true, false));
    }
    return v;
}
}
