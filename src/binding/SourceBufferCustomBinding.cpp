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
#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "dom/DOMException.h"
#include "extra/SourceBuffer.h"

namespace StarFish {

using namespace escargot;

ESValue appendBufferSourceBufferFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, SourceBuffer);
    SourceBuffer* sourceBuffer = (SourceBuffer*)thisValue.asESPointer()
                                     ->asESObject()
                                     ->extraPointerData();
    ESValue firstArg = instance->currentExecutionContext()->readArgument(0);

    try {
        if (false) {
        }
#ifdef USE_ES6_FEATURE
        else if (firstArg.isESPointer() &&
                 firstArg.asESPointer()->isESArrayBufferObject()) {
            ESArrayBufferObject* v =
                firstArg.asESPointer()->asESArrayBufferObject();
            sourceBuffer->appendBuffer((uint8_t*)v->data(), v->bytelength());
        } else if (firstArg.isESPointer() &&
                   firstArg.asESPointer()->isESArrayBufferView()) {
            ESArrayBufferView* v =
                firstArg.asESPointer()->asESArrayBufferView();
            uint8_t* p = (uint8_t*)v->buffer()->data();
            sourceBuffer->appendBuffer(p, v->bytelength());
        }
#endif
        else {
            COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "appendBuffer",
                            "SourceBuffer", SIGNATURE_NOT_FOUND);
            THROW_EXCEPTION(msg);
        }
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    return ESValue(ESValue::ESUndefined);
}
}
#endif
