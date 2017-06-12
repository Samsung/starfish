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
#include "core/dom/DOMException.h"
#include "core/modules/mediasource/SourceBuffer.h"

#include <EscargotPublic.h>
using namespace Escargot;

namespace StarFish {

ValueRef* appendBufferSourceBufferFunction(ExecutionStateRef* state,
                                           ValueRef* thisValue, size_t argc,
                                           ValueRef** argv,
                                           bool isNewExpression)
{
    CHECK_TYPEOF(thisValue, SourceBuffer);
    SourceBuffer* sourceBuffer =
        (SourceBuffer*)thisValue->asObject()->extraData();
    ValueRef* firstArg = argv[0];

    try {
        if (firstArg->isObject() &&
            firstArg->asObject()->isArrayBufferObject()) {
            ArrayBufferObjectRef* v =
                firstArg->asObject()->asArrayBufferObject();
            sourceBuffer->appendBuffer((uint8_t*)v->rawBuffer(),
                                       v->bytelength());
        } else if (firstArg->isObject() &&
                   firstArg->asObject()->isArrayBufferView()) {
            ArrayBufferViewRef* v = firstArg->asObject()->asArrayBufferView();
            uint8_t* p = (uint8_t*)v->buffer()->rawBuffer();
            sourceBuffer->appendBuffer(p, v->bytelength());
        } else {
            COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "appendBuffer",
                            "SourceBuffer", SIGNATURE_NOT_FOUND);
            THROW_EXCEPTION(msg);
        }
    } catch (DOMException* e) {
        state->throwException(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    return scriptUndefined();
}
}
#endif
