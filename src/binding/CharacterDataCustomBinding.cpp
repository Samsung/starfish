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

#include "dom/CharacterData.h"

namespace StarFish {

using namespace escargot;

ESValue lengthCharacterDataGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(CharacterData);
    String* data = originalObj->data();
    if (data->isASCIIString()) {
        return ESValue(originalObj->length());
    } else {
        // TODO: measure length without converting
        ESString* data2 = toJSString(data).toString();
        return ESValue(data2->length());
    }
}
}
