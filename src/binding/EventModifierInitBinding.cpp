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

EventModifierInit toEventModifierInitFromESValue(ESVMInstance* instance,
                                                 ESValue& from)
{
    if (!from.isObject()) {
        auto msg = ESString::create(
            "Failed to generate EventModifierInit from non-object");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    ESValue arg0 =
        from.asESPointer()->asESObject()->get(ESString::create("ctrlKey"));
    ESValue arg1 =
        from.asESPointer()->asESObject()->get(ESString::create("shiftKey"));
    ESValue arg2 =
        from.asESPointer()->asESObject()->get(ESString::create("altKey"));
    ESValue arg3 =
        from.asESPointer()->asESObject()->get(ESString::create("metaKey"));
    ESValue arg4 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierAltGraph"));
    ESValue arg5 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierCapsLock"));
    ESValue arg6 =
        from.asESPointer()->asESObject()->get(ESString::create("modifierFn"));
    ESValue arg7 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierFnLock"));
    ESValue arg8 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierHyper"));
    ESValue arg9 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierNumLock"));
    ESValue arg10 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierScrollLock"));
    ESValue arg11 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierSuper"));
    ESValue arg12 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierSymbol"));
    ESValue arg13 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierSymbolLock"));
    ESValue arg14 =
        from.asESPointer()->asESObject()->get(ESString::create("view"));
    ESValue arg15 =
        from.asESPointer()->asESObject()->get(ESString::create("detail"));
    ESValue arg16 =
        from.asESPointer()->asESObject()->get(ESString::create("bubbles"));
    ESValue arg17 =
        from.asESPointer()->asESObject()->get(ESString::create("cancelable"));
    ESValue arg18 =
        from.asESPointer()->asESObject()->get(ESString::create("composed"));
    EventModifierInit result;
    // Handle argument arg0
    bool value0 = false;
    if (!arg0.isUndefinedOrNull()) {
        value0 = arg0.toBoolean();
    }
    result.setCtrlKey(value0);
    // Handle argument arg1
    bool value1 = false;
    if (!arg1.isUndefinedOrNull()) {
        value1 = arg1.toBoolean();
    }
    result.setShiftKey(value1);
    // Handle argument arg2
    bool value2 = false;
    if (!arg2.isUndefinedOrNull()) {
        value2 = arg2.toBoolean();
    }
    result.setAltKey(value2);
    // Handle argument arg3
    bool value3 = false;
    if (!arg3.isUndefinedOrNull()) {
        value3 = arg3.toBoolean();
    }
    result.setMetaKey(value3);
    // Handle argument arg4
    bool value4 = false;
    if (!arg4.isUndefinedOrNull()) {
        value4 = arg4.toBoolean();
    }
    result.setModifierAltGraph(value4);
    // Handle argument arg5
    bool value5 = false;
    if (!arg5.isUndefinedOrNull()) {
        value5 = arg5.toBoolean();
    }
    result.setModifierCapsLock(value5);
    // Handle argument arg6
    bool value6 = false;
    if (!arg6.isUndefinedOrNull()) {
        value6 = arg6.toBoolean();
    }
    result.setModifierFn(value6);
    // Handle argument arg7
    bool value7 = false;
    if (!arg7.isUndefinedOrNull()) {
        value7 = arg7.toBoolean();
    }
    result.setModifierFnLock(value7);
    // Handle argument arg8
    bool value8 = false;
    if (!arg8.isUndefinedOrNull()) {
        value8 = arg8.toBoolean();
    }
    result.setModifierHyper(value8);
    // Handle argument arg9
    bool value9 = false;
    if (!arg9.isUndefinedOrNull()) {
        value9 = arg9.toBoolean();
    }
    result.setModifierNumLock(value9);
    // Handle argument arg10
    bool value10 = false;
    if (!arg10.isUndefinedOrNull()) {
        value10 = arg10.toBoolean();
    }
    result.setModifierScrollLock(value10);
    // Handle argument arg11
    bool value11 = false;
    if (!arg11.isUndefinedOrNull()) {
        value11 = arg11.toBoolean();
    }
    result.setModifierSuper(value11);
    // Handle argument arg12
    bool value12 = false;
    if (!arg12.isUndefinedOrNull()) {
        value12 = arg12.toBoolean();
    }
    result.setModifierSymbol(value12);
    // Handle argument arg13
    bool value13 = false;
    if (!arg13.isUndefinedOrNull()) {
        value13 = arg13.toBoolean();
    }
    result.setModifierSymbolLock(value13);
    // Handle argument arg14
    Window* value14 = nullptr;
    if (!arg14.isUndefinedOrNull()) {
        CHECK_TYPEOF(arg14, Window);
        value14 =
            (Window*)(arg14.asESPointer()->asESObject()->extraPointerData());
    }
    result.setView(value14);
    // Handle argument arg15
    int32_t value15 = 0;
    if (!arg15.isUndefinedOrNull()) {
        value15 = arg15.toInt32();
    }
    result.setDetail(value15);
    // Handle argument arg16
    bool value16 = false;
    if (!arg16.isUndefinedOrNull()) {
        value16 = arg16.toBoolean();
    }
    result.setBubbles(value16);
    // Handle argument arg17
    bool value17 = false;
    if (!arg17.isUndefinedOrNull()) {
        value17 = arg17.toBoolean();
    }
    result.setCancelable(value17);
    // Handle argument arg18
    bool value18 = false;
    if (!arg18.isUndefinedOrNull()) {
        value18 = arg18.toBoolean();
    }
    result.setComposed(value18);
    return result;
}

ESValue toESValueFromEventModifierInit(ESVMInstance* instance,
                                       EventModifierInit& from)
{
    ESObject* result = ESObject::create();
    // Declare native value (empty when type is void)
    bool value0;
    value0 = from.ctrlKey();
    result->set(ESString::create("ctrlKey"), ESValue(value0));
    // Declare native value (empty when type is void)
    bool value1;
    value1 = from.shiftKey();
    result->set(ESString::create("shiftKey"), ESValue(value1));
    // Declare native value (empty when type is void)
    bool value2;
    value2 = from.altKey();
    result->set(ESString::create("altKey"), ESValue(value2));
    // Declare native value (empty when type is void)
    bool value3;
    value3 = from.metaKey();
    result->set(ESString::create("metaKey"), ESValue(value3));
    // Declare native value (empty when type is void)
    bool value4;
    value4 = from.modifierAltGraph();
    result->set(ESString::create("modifierAltGraph"), ESValue(value4));
    // Declare native value (empty when type is void)
    bool value5;
    value5 = from.modifierCapsLock();
    result->set(ESString::create("modifierCapsLock"), ESValue(value5));
    // Declare native value (empty when type is void)
    bool value6;
    value6 = from.modifierFn();
    result->set(ESString::create("modifierFn"), ESValue(value6));
    // Declare native value (empty when type is void)
    bool value7;
    value7 = from.modifierFnLock();
    result->set(ESString::create("modifierFnLock"), ESValue(value7));
    // Declare native value (empty when type is void)
    bool value8;
    value8 = from.modifierHyper();
    result->set(ESString::create("modifierHyper"), ESValue(value8));
    // Declare native value (empty when type is void)
    bool value9;
    value9 = from.modifierNumLock();
    result->set(ESString::create("modifierNumLock"), ESValue(value9));
    // Declare native value (empty when type is void)
    bool value10;
    value10 = from.modifierScrollLock();
    result->set(ESString::create("modifierScrollLock"), ESValue(value10));
    // Declare native value (empty when type is void)
    bool value11;
    value11 = from.modifierSuper();
    result->set(ESString::create("modifierSuper"), ESValue(value11));
    // Declare native value (empty when type is void)
    bool value12;
    value12 = from.modifierSymbol();
    result->set(ESString::create("modifierSymbol"), ESValue(value12));
    // Declare native value (empty when type is void)
    bool value13;
    value13 = from.modifierSymbolLock();
    result->set(ESString::create("modifierSymbolLock"), ESValue(value13));
    // Declare native value (empty when type is void)
    Window* value14 = nullptr;
    value14 = from.view();
    if (value14 == nullptr) {
        result->set(ESString::create("view"), ESValue(ESValue::ESNull));
    } else {
        result->set(ESString::create("view"), value14->scriptValue());
    }
    // Declare native value (empty when type is void)
    int32_t value15;
    value15 = from.detail();
    result->set(ESString::create("detail"), ESValue(value15));
    // Declare native value (empty when type is void)
    bool value16;
    value16 = from.bubbles();
    result->set(ESString::create("bubbles"), ESValue(value16));
    // Declare native value (empty when type is void)
    bool value17;
    value17 = from.cancelable();
    result->set(ESString::create("cancelable"), ESValue(value17));
    // Declare native value (empty when type is void)
    bool value18;
    value18 = from.composed();
    result->set(ESString::create("composed"), ESValue(value18));
    return ESValue(result);
}
}
