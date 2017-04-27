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
#include "dom/EventTarget.h"
#include "dom/FocusEvent.h"

namespace StarFish {

using namespace escargot;

FocusEventInit toFocusEventInitFromESValue(ESVMInstance* instance,
                                           ESValue& from)
{
    if (!from.isObject()) {
        auto msg = ESString::create(
            "Failed to generate FocusEventInit from non-object");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    ESValue arg0 = from.asESPointer()->asESObject()->get(
        ESString::create("relatedTarget"));
    ESValue arg1 =
        from.asESPointer()->asESObject()->get(ESString::create("view"));
    ESValue arg2 =
        from.asESPointer()->asESObject()->get(ESString::create("detail"));
    ESValue arg3 =
        from.asESPointer()->asESObject()->get(ESString::create("bubbles"));
    ESValue arg4 =
        from.asESPointer()->asESObject()->get(ESString::create("cancelable"));
    ESValue arg5 =
        from.asESPointer()->asESObject()->get(ESString::create("composed"));
    ESValue arg6 =
        from.asESPointer()->asESObject()->get(ESString::create("bubbles"));
    ESValue arg7 =
        from.asESPointer()->asESObject()->get(ESString::create("cancelable"));
    ESValue arg8 =
        from.asESPointer()->asESObject()->get(ESString::create("composed"));
    FocusEventInit result;
    // Handle argument arg0
    EventTarget* value0 = nullptr;
    if (!arg0.isUndefinedOrNull()) {
        CHECK_TYPEOF(arg0, EventTarget);
        value0 = (EventTarget*)(arg0.asESPointer()
                                    ->asESObject()
                                    ->extraPointerData());
    }
    result.setRelatedTarget(value0);
    // Handle argument arg1
    Window* value1 = nullptr;
    if (!arg1.isUndefinedOrNull()) {
        CHECK_TYPEOF(arg1, Window);
        value1 =
            (Window*)(arg1.asESPointer()->asESObject()->extraPointerData());
    }
    result.setView(value1);
    // Handle argument arg2
    int32_t value2 = 0;
    if (!arg2.isUndefinedOrNull()) {
        value2 = arg2.toInt32();
    }
    result.setDetail(value2);
    // Handle argument arg3
    bool value3 = false;
    if (!arg3.isUndefinedOrNull()) {
        value3 = arg3.toBoolean();
    }
    result.setBubbles(value3);
    // Handle argument arg4
    bool value4 = false;
    if (!arg4.isUndefinedOrNull()) {
        value4 = arg4.toBoolean();
    }
    result.setCancelable(value4);
    // Handle argument arg5
    bool value5 = false;
    if (!arg5.isUndefinedOrNull()) {
        value5 = arg5.toBoolean();
    }
    result.setComposed(value5);
    // Handle argument arg6
    bool value6 = false;
    if (!arg6.isUndefinedOrNull()) {
        value6 = arg6.toBoolean();
    }
    result.setBubbles(value6);
    // Handle argument arg7
    bool value7 = false;
    if (!arg7.isUndefinedOrNull()) {
        value7 = arg7.toBoolean();
    }
    result.setCancelable(value7);
    // Handle argument arg8
    bool value8 = false;
    if (!arg8.isUndefinedOrNull()) {
        value8 = arg8.toBoolean();
    }
    result.setComposed(value8);
    return result;
}

ESValue toESValueFromFocusEventInit(ESVMInstance* instance,
                                    FocusEventInit& from)
{
    ESObject* result = ESObject::create();
    // Declare native value (empty when type is void)
    EventTarget* value0 = nullptr;
    value0 = from.relatedTarget();
    if (value0 == nullptr) {
        result->set(ESString::create("relatedTarget"),
                    ESValue(ESValue::ESNull));
    } else {
        result->set(ESString::create("relatedTarget"), value0->scriptValue());
    }
    // Declare native value (empty when type is void)
    Window* value1 = nullptr;
    value1 = from.view();
    if (value1 == nullptr) {
        result->set(ESString::create("view"), ESValue(ESValue::ESNull));
    } else {
        result->set(ESString::create("view"), value1->scriptValue());
    }
    // Declare native value (empty when type is void)
    int32_t value2;
    value2 = from.detail();
    result->set(ESString::create("detail"), ESValue(value2));
    // Declare native value (empty when type is void)
    bool value3;
    value3 = from.bubbles();
    result->set(ESString::create("bubbles"), ESValue(value3));
    // Declare native value (empty when type is void)
    bool value4;
    value4 = from.cancelable();
    result->set(ESString::create("cancelable"), ESValue(value4));
    // Declare native value (empty when type is void)
    bool value5;
    value5 = from.composed();
    result->set(ESString::create("composed"), ESValue(value5));
    // Declare native value (empty when type is void)
    bool value6;
    value6 = from.bubbles();
    result->set(ESString::create("bubbles"), ESValue(value6));
    // Declare native value (empty when type is void)
    bool value7;
    value7 = from.cancelable();
    result->set(ESString::create("cancelable"), ESValue(value7));
    // Declare native value (empty when type is void)
    bool value8;
    value8 = from.composed();
    result->set(ESString::create("composed"), ESValue(value8));
    return ESValue(result);
}
}
