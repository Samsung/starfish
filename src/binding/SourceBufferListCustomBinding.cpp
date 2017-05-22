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
#include "core/extra/SourceBuffer.h"
#include "core/extra/SourceBufferList.h"

namespace StarFish {

using namespace escargot;

static ESValue readCallbackFunction(const ESValue& key, ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    SourceBufferList* self = (SourceBufferList*)obj->extraPointerData();
    STARFISH_ASSERT(self->isSourceBufferList());
    uint32_t idx = key.toIndex();
    if (idx != ESValue::ESInvalidIndexValue && idx < self->length()) {
        SourceBuffer* e = (*self)[idx];
        STARFISH_ASSERT(e);
        return e->scriptValue();
    }
    return ESValue(ESValue::ESDeletedValue);
}

static bool writeCallbackFunction(const ESValue& key, const ESValue& val,
                                  ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    return false;
}

static ESValueVector enumerateCallbackFunction(ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    SourceBufferList* self = (SourceBufferList*)obj->extraPointerData();
    STARFISH_ASSERT(self->isSourceBufferList());
    size_t len = self->length();
    ESValueVector v(len);
    for (size_t i = 0; i < len; i++) {
        v[i] = ESValue(i);
    }
    return v;
}

void SourceBufferList::postInit(ScriptBindingInstance* instance)
{
    scriptObject()->setPropertyInterceptor(readCallbackFunction,
                                           writeCallbackFunction,
                                           enumerateCallbackFunction, true);
}
}
#endif
