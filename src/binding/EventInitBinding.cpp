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

#include "dom/Event.h"

namespace StarFish {

using namespace escargot;

EventInit toEventInitFromESValue(ESVMInstance* instance, ESValue& from)
{
    if (from.isUndefinedOrNull()) {
        // Return empty dictionary
        return EventInit();
    }
    if (!from.isObject()) {
        auto msg =
            ESString::create("Failed to generate EventInit from non-object");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    ESValue arg0 =
        from.asESPointer()->asESObject()->get(ESString::create("bubbles"));
    ESValue arg1 =
        from.asESPointer()->asESObject()->get(ESString::create("cancelable"));
    ESValue arg2 =
        from.asESPointer()->asESObject()->get(ESString::create("composed"));
    EventInit result;
    // Handle argument arg0
    bool value0 = false;
    if (!arg0.isUndefinedOrNull()) {
        value0 = arg0.toBoolean();
    }
    result.setBubbles(value0);
    // Handle argument arg1
    bool value1 = false;
    if (!arg1.isUndefinedOrNull()) {
        value1 = arg1.toBoolean();
    }
    result.setCancelable(value1);
    // Handle argument arg2
    bool value2 = false;
    if (!arg2.isUndefinedOrNull()) {
        value2 = arg2.toBoolean();
    }
    result.setComposed(value2);
    return result;
}

ESValue toESValueFromEventInit(ESVMInstance* instance, EventInit& from)
{
    ESObject* result = ESObject::create();
    // Declare native value (empty when type is void)
    bool value0;
    value0 = from.bubbles();
    result->set(ESString::create("bubbles"), ESValue(value0));
    // Declare native value (empty when type is void)
    bool value1;
    value1 = from.cancelable();
    result->set(ESString::create("cancelable"), ESValue(value1));
    // Declare native value (empty when type is void)
    bool value2;
    value2 = from.composed();
    result->set(ESString::create("composed"), ESValue(value2));
    return ESValue(result);
}
}
