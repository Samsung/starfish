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

#ifdef STARFISH_ENABLE_MULTIMEDIA

#include "StarFishConfig.h"
#include "core/dom/TextTrackList.h"

#include <EscargotPublic.h>
using namespace Escargot;

namespace StarFish {

ExposableObjectGetOwnPropertyCallbackResult TextTrackListGetOwnPropertyCallback(
    ExecutionStateRef* state, ObjectRef* jsObj, ValueRef* key)
{
    TextTrackList* self = (TextTrackList*)jsObj->extraData();
    STARFISH_ASSERT(self->isTextTrackList());
    uint64_t idx = key->toArrayIndex(state);
    if (idx < self->size()) {
        TextTrack* e = (*self)[idx];
        if (e != nullptr) {
            return ExposableObjectGetOwnPropertyCallbackResult(
                e->scriptValue(), false, true, false);
        }
    }
    return ExposableObjectGetOwnPropertyCallbackResult();
}

void TextTrackListDefineOwnPropertyCallback(ExecutionStateRef* state,
                                            ObjectRef* self,
                                            ValueRef* propertyName,
                                            ValueRef* value)
{
    // do nothing
}

ExposableObjectEnumerationCallbackResultVector TextTrackListEnumerationCallback(
    ExecutionStateRef* state, ObjectRef* jsObj)
{
    TextTrackList* self = (TextTrackList*)jsObj->extraData();
    STARFISH_ASSERT(self->isTextTrackList());
    size_t len = self->size();
    ExposableObjectEnumerationCallbackResultVector v;
    for (size_t i = 0; i < len; i++) {
        v.push_back(ExposableObjectEnumerationCallbackResult(
            ValueRef::create(i), false, true, false));
    }
    return v;
}
}

#endif
