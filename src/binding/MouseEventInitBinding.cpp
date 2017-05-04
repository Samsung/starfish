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
#include "dom/MouseEvent.h"

namespace StarFish {

using namespace escargot;

MouseEventInit toMouseEventInitFromESValue(ESVMInstance* instance,
                                           ESValue& from)
{
    if (!from.isObject()) {
        auto msg = ESString::create(
            "Failed to generate MouseEventInit from non-object");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    ESValue arg0 =
        from.asESPointer()->asESObject()->get(ESString::create("screenX"));
    ESValue arg1 =
        from.asESPointer()->asESObject()->get(ESString::create("screenY"));
    ESValue arg2 =
        from.asESPointer()->asESObject()->get(ESString::create("clientX"));
    ESValue arg3 =
        from.asESPointer()->asESObject()->get(ESString::create("clientY"));
    ESValue arg4 =
        from.asESPointer()->asESObject()->get(ESString::create("button"));
    ESValue arg5 =
        from.asESPointer()->asESObject()->get(ESString::create("buttons"));
    ESValue arg6 = from.asESPointer()->asESObject()->get(
        ESString::create("relatedTarget"));
    ESValue arg7 =
        from.asESPointer()->asESObject()->get(ESString::create("ctrlKey"));
    ESValue arg8 =
        from.asESPointer()->asESObject()->get(ESString::create("shiftKey"));
    ESValue arg9 =
        from.asESPointer()->asESObject()->get(ESString::create("altKey"));
    ESValue arg10 =
        from.asESPointer()->asESObject()->get(ESString::create("metaKey"));
    ESValue arg11 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierAltGraph"));
    ESValue arg12 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierCapsLock"));
    ESValue arg13 =
        from.asESPointer()->asESObject()->get(ESString::create("modifierFn"));
    ESValue arg14 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierFnLock"));
    ESValue arg15 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierHyper"));
    ESValue arg16 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierNumLock"));
    ESValue arg17 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierScrollLock"));
    ESValue arg18 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierSuper"));
    ESValue arg19 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierSymbol"));
    ESValue arg20 = from.asESPointer()->asESObject()->get(
        ESString::create("modifierSymbolLock"));
    ESValue arg21 =
        from.asESPointer()->asESObject()->get(ESString::create("view"));
    ESValue arg22 =
        from.asESPointer()->asESObject()->get(ESString::create("detail"));
    ESValue arg23 =
        from.asESPointer()->asESObject()->get(ESString::create("bubbles"));
    ESValue arg24 =
        from.asESPointer()->asESObject()->get(ESString::create("cancelable"));
    ESValue arg25 =
        from.asESPointer()->asESObject()->get(ESString::create("composed"));
    MouseEventInit result;
    // Handle argument arg0
    int32_t value0 = 0;
    if (!arg0.isUndefinedOrNull()) {
        value0 = arg0.toInt32();
    }
    result.setScreenX(value0);
    // Handle argument arg1
    int32_t value1 = 0;
    if (!arg1.isUndefinedOrNull()) {
        value1 = arg1.toInt32();
    }
    result.setScreenY(value1);
    // Handle argument arg2
    int32_t value2 = 0;
    if (!arg2.isUndefinedOrNull()) {
        value2 = arg2.toInt32();
    }
    result.setClientX(value2);
    // Handle argument arg3
    int32_t value3 = 0;
    if (!arg3.isUndefinedOrNull()) {
        value3 = arg3.toInt32();
    }
    result.setClientY(value3);
    // Handle argument arg4
    int32_t value4 = 0;
    if (!arg4.isUndefinedOrNull()) {
        value4 = arg4.toInt32();
    }
    result.setButton(value4);
    // Handle argument arg5
    uint32_t value5 = 0;
    if (!arg5.isUndefinedOrNull()) {
        value5 = arg5.toUint32();
    }
    result.setButtons(value5);
    // Handle argument arg6
    EventTarget* value6 = nullptr;
    if (!arg6.isUndefinedOrNull()) {
        CHECK_TYPEOF(arg6, EventTarget);
        value6 = (EventTarget*)(arg6.asESPointer()
                                    ->asESObject()
                                    ->extraPointerData());
    }
    result.setRelatedTarget(value6);
    // Handle argument arg7
    bool value7 = false;
    if (!arg7.isUndefinedOrNull()) {
        value7 = arg7.toBoolean();
    }
    result.setCtrlKey(value7);
    // Handle argument arg8
    bool value8 = false;
    if (!arg8.isUndefinedOrNull()) {
        value8 = arg8.toBoolean();
    }
    result.setShiftKey(value8);
    // Handle argument arg9
    bool value9 = false;
    if (!arg9.isUndefinedOrNull()) {
        value9 = arg9.toBoolean();
    }
    result.setAltKey(value9);
    // Handle argument arg10
    bool value10 = false;
    if (!arg10.isUndefinedOrNull()) {
        value10 = arg10.toBoolean();
    }
    result.setMetaKey(value10);
    // Handle argument arg11
    bool value11 = false;
    if (!arg11.isUndefinedOrNull()) {
        value11 = arg11.toBoolean();
    }
    result.setModifierAltGraph(value11);
    // Handle argument arg12
    bool value12 = false;
    if (!arg12.isUndefinedOrNull()) {
        value12 = arg12.toBoolean();
    }
    result.setModifierCapsLock(value12);
    // Handle argument arg13
    bool value13 = false;
    if (!arg13.isUndefinedOrNull()) {
        value13 = arg13.toBoolean();
    }
    result.setModifierFn(value13);
    // Handle argument arg14
    bool value14 = false;
    if (!arg14.isUndefinedOrNull()) {
        value14 = arg14.toBoolean();
    }
    result.setModifierFnLock(value14);
    // Handle argument arg15
    bool value15 = false;
    if (!arg15.isUndefinedOrNull()) {
        value15 = arg15.toBoolean();
    }
    result.setModifierHyper(value15);
    // Handle argument arg16
    bool value16 = false;
    if (!arg16.isUndefinedOrNull()) {
        value16 = arg16.toBoolean();
    }
    result.setModifierNumLock(value16);
    // Handle argument arg17
    bool value17 = false;
    if (!arg17.isUndefinedOrNull()) {
        value17 = arg17.toBoolean();
    }
    result.setModifierScrollLock(value17);
    // Handle argument arg18
    bool value18 = false;
    if (!arg18.isUndefinedOrNull()) {
        value18 = arg18.toBoolean();
    }
    result.setModifierSuper(value18);
    // Handle argument arg19
    bool value19 = false;
    if (!arg19.isUndefinedOrNull()) {
        value19 = arg19.toBoolean();
    }
    result.setModifierSymbol(value19);
    // Handle argument arg20
    bool value20 = false;
    if (!arg20.isUndefinedOrNull()) {
        value20 = arg20.toBoolean();
    }
    result.setModifierSymbolLock(value20);
    // Handle argument arg21
    Window* value21 = nullptr;
    if (!arg21.isUndefinedOrNull()) {
        CHECK_TYPEOF(arg21, Window);
        value21 =
            (Window*)(arg21.asESPointer()->asESObject()->extraPointerData());
    }
    result.setView(value21);
    // Handle argument arg22
    int32_t value22 = 0;
    if (!arg22.isUndefinedOrNull()) {
        value22 = arg22.toInt32();
    }
    result.setDetail(value22);
    // Handle argument arg23
    bool value23 = false;
    if (!arg23.isUndefinedOrNull()) {
        value23 = arg23.toBoolean();
    }
    result.setBubbles(value23);
    // Handle argument arg24
    bool value24 = false;
    if (!arg24.isUndefinedOrNull()) {
        value24 = arg24.toBoolean();
    }
    result.setCancelable(value24);
    // Handle argument arg25
    bool value25 = false;
    if (!arg25.isUndefinedOrNull()) {
        value25 = arg25.toBoolean();
    }
    result.setComposed(value25);
    return result;
}

ESValue toESValueFromMouseEventInit(ESVMInstance* instance,
                                    MouseEventInit& from)
{
    ESObject* result = ESObject::create();
    // Declare native value (empty when type is void)
    int32_t value0;
    value0 = from.screenX();
    result->set(ESString::create("screenX"), ESValue(value0));
    // Declare native value (empty when type is void)
    int32_t value1;
    value1 = from.screenY();
    result->set(ESString::create("screenY"), ESValue(value1));
    // Declare native value (empty when type is void)
    int32_t value2;
    value2 = from.clientX();
    result->set(ESString::create("clientX"), ESValue(value2));
    // Declare native value (empty when type is void)
    int32_t value3;
    value3 = from.clientY();
    result->set(ESString::create("clientY"), ESValue(value3));
    // Declare native value (empty when type is void)
    int32_t value4;
    value4 = from.button();
    result->set(ESString::create("button"), ESValue(value4));
    // Declare native value (empty when type is void)
    uint32_t value5;
    value5 = from.buttons();
    result->set(ESString::create("buttons"), ESValue(value5));
    // Declare native value (empty when type is void)
    EventTarget* value6 = nullptr;
    value6 = from.relatedTarget();
    if (value6 == nullptr) {
        result->set(ESString::create("relatedTarget"),
                    ESValue(ESValue::ESNull));
    } else {
        result->set(ESString::create("relatedTarget"), value6->scriptValue());
    }
    // Declare native value (empty when type is void)
    bool value7;
    value7 = from.ctrlKey();
    result->set(ESString::create("ctrlKey"), ESValue(value7));
    // Declare native value (empty when type is void)
    bool value8;
    value8 = from.shiftKey();
    result->set(ESString::create("shiftKey"), ESValue(value8));
    // Declare native value (empty when type is void)
    bool value9;
    value9 = from.altKey();
    result->set(ESString::create("altKey"), ESValue(value9));
    // Declare native value (empty when type is void)
    bool value10;
    value10 = from.metaKey();
    result->set(ESString::create("metaKey"), ESValue(value10));
    // Declare native value (empty when type is void)
    bool value11;
    value11 = from.modifierAltGraph();
    result->set(ESString::create("modifierAltGraph"), ESValue(value11));
    // Declare native value (empty when type is void)
    bool value12;
    value12 = from.modifierCapsLock();
    result->set(ESString::create("modifierCapsLock"), ESValue(value12));
    // Declare native value (empty when type is void)
    bool value13;
    value13 = from.modifierFn();
    result->set(ESString::create("modifierFn"), ESValue(value13));
    // Declare native value (empty when type is void)
    bool value14;
    value14 = from.modifierFnLock();
    result->set(ESString::create("modifierFnLock"), ESValue(value14));
    // Declare native value (empty when type is void)
    bool value15;
    value15 = from.modifierHyper();
    result->set(ESString::create("modifierHyper"), ESValue(value15));
    // Declare native value (empty when type is void)
    bool value16;
    value16 = from.modifierNumLock();
    result->set(ESString::create("modifierNumLock"), ESValue(value16));
    // Declare native value (empty when type is void)
    bool value17;
    value17 = from.modifierScrollLock();
    result->set(ESString::create("modifierScrollLock"), ESValue(value17));
    // Declare native value (empty when type is void)
    bool value18;
    value18 = from.modifierSuper();
    result->set(ESString::create("modifierSuper"), ESValue(value18));
    // Declare native value (empty when type is void)
    bool value19;
    value19 = from.modifierSymbol();
    result->set(ESString::create("modifierSymbol"), ESValue(value19));
    // Declare native value (empty when type is void)
    bool value20;
    value20 = from.modifierSymbolLock();
    result->set(ESString::create("modifierSymbolLock"), ESValue(value20));
    // Declare native value (empty when type is void)
    Window* value21 = nullptr;
    value21 = from.view();
    if (value21 == nullptr) {
        result->set(ESString::create("view"), ESValue(ESValue::ESNull));
    } else {
        result->set(ESString::create("view"), value21->scriptValue());
    }
    // Declare native value (empty when type is void)
    int32_t value22;
    value22 = from.detail();
    result->set(ESString::create("detail"), ESValue(value22));
    // Declare native value (empty when type is void)
    bool value23;
    value23 = from.bubbles();
    result->set(ESString::create("bubbles"), ESValue(value23));
    // Declare native value (empty when type is void)
    bool value24;
    value24 = from.cancelable();
    result->set(ESString::create("cancelable"), ESValue(value24));
    // Declare native value (empty when type is void)
    bool value25;
    value25 = from.composed();
    result->set(ESString::create("composed"), ESValue(value25));
    return ESValue(result);
}
}
