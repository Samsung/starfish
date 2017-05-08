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

#include "platform/window/Window.h"
#include "dom/UIEvent.h"

namespace StarFish {

using namespace escargot;

UIEventInit toUIEventInitFromESValue(ESVMInstance* instance, ESValue& from)
{
    if (!from.isObject()) {
        auto msg =
            ESString::create("Failed to generate UIEventInit from non-object");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    ESValue arg0 =
        from.asESPointer()->asESObject()->get(ESString::create("view"));
    ESValue arg1 =
        from.asESPointer()->asESObject()->get(ESString::create("detail"));
    ESValue arg2 =
        from.asESPointer()->asESObject()->get(ESString::create("bubbles"));
    ESValue arg3 =
        from.asESPointer()->asESObject()->get(ESString::create("cancelable"));
    ESValue arg4 =
        from.asESPointer()->asESObject()->get(ESString::create("composed"));
    ESValue arg5 =
        from.asESPointer()->asESObject()->get(ESString::create("bubbles"));
    ESValue arg6 =
        from.asESPointer()->asESObject()->get(ESString::create("cancelable"));
    ESValue arg7 =
        from.asESPointer()->asESObject()->get(ESString::create("composed"));
    UIEventInit result;
    // Handle argument arg0
    Window* value0 = nullptr;
    if (!arg0.isUndefinedOrNull()) {
        CHECK_TYPEOF(arg0, Window);
        value0 =
            (Window*)(arg0.asESPointer()->asESObject()->extraPointerData());
    }
    result.setView(value0);
    // Handle argument arg1
    int32_t value1 = 0;
    if (!arg1.isUndefinedOrNull()) {
        value1 = arg1.toInt32();
    }
    result.setDetail(value1);
    // Handle argument arg2
    bool value2 = false;
    if (!arg2.isUndefinedOrNull()) {
        value2 = arg2.toBoolean();
    }
    result.setBubbles(value2);
    // Handle argument arg3
    bool value3 = false;
    if (!arg3.isUndefinedOrNull()) {
        value3 = arg3.toBoolean();
    }
    result.setCancelable(value3);
    // Handle argument arg4
    bool value4 = false;
    if (!arg4.isUndefinedOrNull()) {
        value4 = arg4.toBoolean();
    }
    result.setComposed(value4);
    // Handle argument arg5
    bool value5 = false;
    if (!arg5.isUndefinedOrNull()) {
        value5 = arg5.toBoolean();
    }
    result.setBubbles(value5);
    // Handle argument arg6
    bool value6 = false;
    if (!arg6.isUndefinedOrNull()) {
        value6 = arg6.toBoolean();
    }
    result.setCancelable(value6);
    // Handle argument arg7
    bool value7 = false;
    if (!arg7.isUndefinedOrNull()) {
        value7 = arg7.toBoolean();
    }
    result.setComposed(value7);
    return result;
}

ESValue toESValueFromUIEventInit(ESVMInstance* instance, UIEventInit& from)
{
    ESObject* result = ESObject::create();
    // Declare native value (empty when type is void)
    Window* value0 = nullptr;
    value0 = from.view();
    if (value0 == nullptr) {
        result->set(ESString::create("view"), ESValue(ESValue::ESNull));
    } else {
        result->set(ESString::create("view"), value0->scriptValue());
    }
    // Declare native value (empty when type is void)
    int32_t value1;
    value1 = from.detail();
    result->set(ESString::create("detail"), ESValue(value1));
    // Declare native value (empty when type is void)
    bool value2;
    value2 = from.bubbles();
    result->set(ESString::create("bubbles"), ESValue(value2));
    // Declare native value (empty when type is void)
    bool value3;
    value3 = from.cancelable();
    result->set(ESString::create("cancelable"), ESValue(value3));
    // Declare native value (empty when type is void)
    bool value4;
    value4 = from.composed();
    result->set(ESString::create("composed"), ESValue(value4));
    // Declare native value (empty when type is void)
    bool value5;
    value5 = from.bubbles();
    result->set(ESString::create("bubbles"), ESValue(value5));
    // Declare native value (empty when type is void)
    bool value6;
    value6 = from.cancelable();
    result->set(ESString::create("cancelable"), ESValue(value6));
    // Declare native value (empty when type is void)
    bool value7;
    value7 = from.composed();
    result->set(ESString::create("composed"), ESValue(value7));
    return ESValue(result);
}
}
