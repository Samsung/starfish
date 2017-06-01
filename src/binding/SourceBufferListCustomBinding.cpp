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
#if defined(STARFISH_ENABLE_MULTIMEDIA)

#include "StarFishConfig.h"
#include "core/modules/mediasource/SourceBuffer.h"
#include "core/modules/mediasource/SourceBufferList.h"

#include <EscargotPublic.h>
using namespace Escargot;

namespace StarFish {

ExposableObjectGetOwnPropertyCallbackResult
SourceBufferListGetOwnPropertyCallback(ExecutionStateRef* state,
                                       ObjectRef* jsObj, ValueRef* key)
{
    SourceBufferList* self = (SourceBufferList*)jsObj->extraData();
    STARFISH_ASSERT(self->isSourceBufferList());

    uint32_t idx = key->toArrayIndex(state);
    if (idx < self->length()) {
        SourceBuffer* e = (*self)[idx];
        STARFISH_ASSERT(e);
        return ExposableObjectGetOwnPropertyCallbackResult(e->scriptValue(),
                                                           false, true, false);
    }

    return ExposableObjectGetOwnPropertyCallbackResult();
}

void SourceBufferListDefineOwnPropertyCallback(ExecutionStateRef* state,
                                               ObjectRef* jsObj, ValueRef* key,
                                               ValueRef* val)
{
    return;
}

ExposableObjectEnumerationCallbackResultVector
SourceBufferListEnumerationCallback(ExecutionStateRef* state, ObjectRef* jsObj)
{
    SourceBufferList* self = (SourceBufferList*)jsObj->extraData();
    size_t len = self->length();
    ExposableObjectEnumerationCallbackResultVector v;
    for (size_t i = 0; i < len; i++) {
        v.push_back(ExposableObjectEnumerationCallbackResult(
            ValueRef::create(i), false, true, false));
    }
    return v;
}
}
#endif
