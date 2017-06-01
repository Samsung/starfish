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
#include "core/dom/CharacterData.h"

#include <EscargotPublic.h>

namespace StarFish {

using namespace Escargot;

ValueRef* lengthCharacterDataGetterFunction(ExecutionStateRef* state,
                                            ValueRef* thisValue, size_t argc,
                                            ValueRef** argv,
                                            bool isNewExpression)
{
    GENERATE_THIS_AND_CHECK_TYPE(CharacterData);
    String* data = originalObj->data();
    if (data->isASCIIString()) {
        return ValueRef::create(originalObj->length());
    } else {
        // TODO: measure length without converting
        StringRef* data2 = toJSString(data);
        return ValueRef::create(data2->length());
    }
}
}
