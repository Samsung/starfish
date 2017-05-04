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
#include "dom/KeyboardEvent.h"

namespace StarFish {

using namespace escargot;

KeyboardEventInit toKeyboardEventInitFromESValue(ESVMInstance* instance,
                                                 ESValue& from)
{
    if (!from.isObject()) {
        auto msg = ESString::create(
            "Failed to generate KeyboardEventInit from non-object");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    ESValue arg0 =
        from.asESPointer()->asESObject()->get(ESString::create("key"));
    ESValue arg1 =
        from.asESPointer()->asESObject()->get(ESString::create("code"));
    ESValue arg2 =
        from.asESPointer()->asESObject()->get(ESString::create("location"));
    ESValue arg3 =
        from.asESPointer()->asESObject()->get(ESString::create("repeat"));
    ESValue arg4 =
        from.asESPointer()->asESObject()->get(ESString::create("isComposing"));
    ESValue arg5 =
        from.asESPointer()->asESObject()->get(ESString::create("ctrlKey"));
    ESValue arg6 =
        from.asESPointer()->asESObject()->get(ESString::create("shiftKey"));
    ESValue arg7 =
        from.asESPointer()->asESObject()->get(ESString::create("altKey"));
    ESValue arg8 =
        from.asESPointer()->asESObject()->get(ESString::create("metaKey"));
    ESValue arg9 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierAltGraph"));
    ESValue arg10 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierCapsLock"));
    ESValue arg11 =
        from.asESPointer()->asESObject()->get(ESString::create("modifierFn"));
    ESValue arg12 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierFnLock"));
    ESValue arg13 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierHyper"));
    ESValue arg14 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierNumLock"));
    ESValue arg15 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierScrollLock"));
    ESValue arg16 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierSuper"));
    ESValue arg17 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierSymbol"));
    ESValue arg18 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierSymbolLock"));
    ESValue arg19 =
        from.asESPointer()->asESObject()->get(ESString::create("view"));
    ESValue arg20 =
        from.asESPointer()->asESObject()->get(ESString::create("detail"));
    ESValue arg21 =
        from.asESPointer()->asESObject()->get(ESString::create("bubbles"));
    ESValue arg22 =
        from.asESPointer()->asESObject()->get(ESString::create("cancelable"));
    ESValue arg23 =
        from.asESPointer()->asESObject()->get(ESString::create("composed"));
    KeyboardEventInit result;
    // Handle argument arg0
    String* value0 = String::emptyString;
    if (!arg0.isUndefinedOrNull()) {
        value0 = toBrowserString(arg0);
    }
    result.setKey(value0);
    // Handle argument arg1
    String* value1 = String::emptyString;
    if (!arg1.isUndefinedOrNull()) {
        value1 = toBrowserString(arg1);
    }
    result.setCode(value1);
    // Handle argument arg2
    uint32_t value2 = 0;
    if (!arg2.isUndefinedOrNull()) {
        value2 = arg2.toUint32();
    }
    result.setLocation(value2);
    // Handle argument arg3
    bool value3 = false;
    if (!arg3.isUndefinedOrNull()) {
        value3 = arg3.toBoolean();
    }
    result.setRepeat(value3);
    // Handle argument arg4
    bool value4 = false;
    if (!arg4.isUndefinedOrNull()) {
        value4 = arg4.toBoolean();
    }
    result.setIsComposing(value4);
    // Handle argument arg5
    bool value5 = false;
    if (!arg5.isUndefinedOrNull()) {
        value5 = arg5.toBoolean();
    }
    result.setCtrlKey(value5);
    // Handle argument arg6
    bool value6 = false;
    if (!arg6.isUndefinedOrNull()) {
        value6 = arg6.toBoolean();
    }
    result.setShiftKey(value6);
    // Handle argument arg7
    bool value7 = false;
    if (!arg7.isUndefinedOrNull()) {
        value7 = arg7.toBoolean();
    }
    result.setAltKey(value7);
    // Handle argument arg8
    bool value8 = false;
    if (!arg8.isUndefinedOrNull()) {
        value8 = arg8.toBoolean();
    }
    result.setMetaKey(value8);
    // Handle argument arg9
    bool value9 = false;
    if (!arg9.isUndefinedOrNull()) {
        value9 = arg9.toBoolean();
    }
    result.setModifierAltGraph(value9);
    // Handle argument arg10
    bool value10 = false;
    if (!arg10.isUndefinedOrNull()) {
        value10 = arg10.toBoolean();
    }
    result.setModifierCapsLock(value10);
    // Handle argument arg11
    bool value11 = false;
    if (!arg11.isUndefinedOrNull()) {
        value11 = arg11.toBoolean();
    }
    result.setModifierFn(value11);
    // Handle argument arg12
    bool value12 = false;
    if (!arg12.isUndefinedOrNull()) {
        value12 = arg12.toBoolean();
    }
    result.setModifierFnLock(value12);
    // Handle argument arg13
    bool value13 = false;
    if (!arg13.isUndefinedOrNull()) {
        value13 = arg13.toBoolean();
    }
    result.setModifierHyper(value13);
    // Handle argument arg14
    bool value14 = false;
    if (!arg14.isUndefinedOrNull()) {
        value14 = arg14.toBoolean();
    }
    result.setModifierNumLock(value14);
    // Handle argument arg15
    bool value15 = false;
    if (!arg15.isUndefinedOrNull()) {
        value15 = arg15.toBoolean();
    }
    result.setModifierScrollLock(value15);
    // Handle argument arg16
    bool value16 = false;
    if (!arg16.isUndefinedOrNull()) {
        value16 = arg16.toBoolean();
    }
    result.setModifierSuper(value16);
    // Handle argument arg17
    bool value17 = false;
    if (!arg17.isUndefinedOrNull()) {
        value17 = arg17.toBoolean();
    }
    result.setModifierSymbol(value17);
    // Handle argument arg18
    bool value18 = false;
    if (!arg18.isUndefinedOrNull()) {
        value18 = arg18.toBoolean();
    }
    result.setModifierSymbolLock(value18);
    // Handle argument arg19
    Window* value19 = nullptr;
    if (!arg19.isUndefinedOrNull()) {
        CHECK_TYPEOF(arg19, Window);
        value19 =
            (Window*)(arg19.asESPointer()->asESObject()->extraPointerData());
    }
    result.setView(value19);
    // Handle argument arg20
    int32_t value20 = 0;
    if (!arg20.isUndefinedOrNull()) {
        value20 = arg20.toInt32();
    }
    result.setDetail(value20);
    // Handle argument arg21
    bool value21 = false;
    if (!arg21.isUndefinedOrNull()) {
        value21 = arg21.toBoolean();
    }
    result.setBubbles(value21);
    // Handle argument arg22
    bool value22 = false;
    if (!arg22.isUndefinedOrNull()) {
        value22 = arg22.toBoolean();
    }
    result.setCancelable(value22);
    // Handle argument arg23
    bool value23 = false;
    if (!arg23.isUndefinedOrNull()) {
        value23 = arg23.toBoolean();
    }
    result.setComposed(value23);
    return result;
}

ESValue toESValueFromKeyboardEventInit(ESVMInstance* instance,
                                       KeyboardEventInit& from)
{
    ESObject* result = ESObject::create();
    // Declare native value (empty when type is void)
    String* value0 = String::emptyString;
    value0 = from.key();
    result->set(ESString::create("key"), toJSString(value0));
    // Declare native value (empty when type is void)
    String* value1 = String::emptyString;
    value1 = from.code();
    result->set(ESString::create("code"), toJSString(value1));
    // Declare native value (empty when type is void)
    uint32_t value2;
    value2 = from.location();
    result->set(ESString::create("location"), ESValue(value2));
    // Declare native value (empty when type is void)
    bool value3;
    value3 = from.repeat();
    result->set(ESString::create("repeat"), ESValue(value3));
    // Declare native value (empty when type is void)
    bool value4;
    value4 = from.isComposing();
    result->set(ESString::create("isComposing"), ESValue(value4));
    // Declare native value (empty when type is void)
    bool value5;
    value5 = from.ctrlKey();
    result->set(ESString::create("ctrlKey"), ESValue(value5));
    // Declare native value (empty when type is void)
    bool value6;
    value6 = from.shiftKey();
    result->set(ESString::create("shiftKey"), ESValue(value6));
    // Declare native value (empty when type is void)
    bool value7;
    value7 = from.altKey();
    result->set(ESString::create("altKey"), ESValue(value7));
    // Declare native value (empty when type is void)
    bool value8;
    value8 = from.metaKey();
    result->set(ESString::create("metaKey"), ESValue(value8));
    // Declare native value (empty when type is void)
    bool value9;
    value9 = from.modifierAltGraph();
    result->set(ESString::create("modifierAltGraph"), ESValue(value9));
    // Declare native value (empty when type is void)
    bool value10;
    value10 = from.modifierCapsLock();
    result->set(ESString::create("modifierCapsLock"), ESValue(value10));
    // Declare native value (empty when type is void)
    bool value11;
    value11 = from.modifierFn();
    result->set(ESString::create("modifierFn"), ESValue(value11));
    // Declare native value (empty when type is void)
    bool value12;
    value12 = from.modifierFnLock();
    result->set(ESString::create("modifierFnLock"), ESValue(value12));
    // Declare native value (empty when type is void)
    bool value13;
    value13 = from.modifierHyper();
    result->set(ESString::create("modifierHyper"), ESValue(value13));
    // Declare native value (empty when type is void)
    bool value14;
    value14 = from.modifierNumLock();
    result->set(ESString::create("modifierNumLock"), ESValue(value14));
    // Declare native value (empty when type is void)
    bool value15;
    value15 = from.modifierScrollLock();
    result->set(ESString::create("modifierScrollLock"), ESValue(value15));
    // Declare native value (empty when type is void)
    bool value16;
    value16 = from.modifierSuper();
    result->set(ESString::create("modifierSuper"), ESValue(value16));
    // Declare native value (empty when type is void)
    bool value17;
    value17 = from.modifierSymbol();
    result->set(ESString::create("modifierSymbol"), ESValue(value17));
    // Declare native value (empty when type is void)
    bool value18;
    value18 = from.modifierSymbolLock();
    result->set(ESString::create("modifierSymbolLock"), ESValue(value18));
    // Declare native value (empty when type is void)
    Window* value19 = nullptr;
    value19 = from.view();
    if (value19 == nullptr) {
        result->set(ESString::create("view"), ESValue(ESValue::ESNull));
    } else {
        result->set(ESString::create("view"), value19->scriptValue());
    }
    // Declare native value (empty when type is void)
    int32_t value20;
    value20 = from.detail();
    result->set(ESString::create("detail"), ESValue(value20));
    // Declare native value (empty when type is void)
    bool value21;
    value21 = from.bubbles();
    result->set(ESString::create("bubbles"), ESValue(value21));
    // Declare native value (empty when type is void)
    bool value22;
    value22 = from.cancelable();
    result->set(ESString::create("cancelable"), ESValue(value22));
    // Declare native value (empty when type is void)
    bool value23;
    value23 = from.composed();
    result->set(ESString::create("composed"), ESValue(value23));
    return ESValue(result);
}
}
