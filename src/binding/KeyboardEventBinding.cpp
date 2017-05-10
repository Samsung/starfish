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

#include "dom/KeyboardEvent.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue ctrlKeyGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(KeyboardEvent);
    // Declare native value (empty when type is void)
    bool result;
    result = originalObj->ctrlKey();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue shiftKeyGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(KeyboardEvent);
    // Declare native value (empty when type is void)
    bool result;
    result = originalObj->shiftKey();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue altKeyGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(KeyboardEvent);
    // Declare native value (empty when type is void)
    bool result;
    result = originalObj->altKey();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue metaKeyGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(KeyboardEvent);
    // Declare native value (empty when type is void)
    bool result;
    result = originalObj->metaKey();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue keyCodeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(KeyboardEvent);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->keyCode();
    // Return ESValue from native value
    return ESValue(result);
}

// Implement for functions
ESFunctionObject* bindingKeyboardEvent(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* KeyboardEventString = ESString::create("KeyboardEvent");
    ESFunctionObject* KeyboardEventFunction = ESFunctionObject::create(
        nullptr, defaultFunction, KeyboardEventString, 1, true, true);
    ESObject* KeyboardEventPrototypeObj =
        KeyboardEventFunction->protoType().asESPointer()->asESObject();
    KeyboardEventFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    KeyboardEventPrototypeObj->forceNonVectorHiddenClass(false);
    KeyboardEventPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnUIEvent()->protoType());
    KeyboardEventFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnUIEvent());

    // Bind for constants
    ESString* DOM_KEY_LOCATION_STANDARDString =
        ESString::create("DOM_KEY_LOCATION_STANDARD");
    ESValue DOM_KEY_LOCATION_STANDARDValue = ESValue(0x00);
    KeyboardEventPrototypeObj->defineDataProperty(
        DOM_KEY_LOCATION_STANDARDString, false, true, false,
        DOM_KEY_LOCATION_STANDARDValue);

    KeyboardEventFunction->defineDataProperty(DOM_KEY_LOCATION_STANDARDString,
                                              false, true, false,
                                              DOM_KEY_LOCATION_STANDARDValue);

    ESString* DOM_KEY_LOCATION_LEFTString =
        ESString::create("DOM_KEY_LOCATION_LEFT");
    ESValue DOM_KEY_LOCATION_LEFTValue = ESValue(0x01);
    KeyboardEventPrototypeObj->defineDataProperty(DOM_KEY_LOCATION_LEFTString,
                                                  false, true, false,
                                                  DOM_KEY_LOCATION_LEFTValue);

    KeyboardEventFunction->defineDataProperty(DOM_KEY_LOCATION_LEFTString,
                                              false, true, false,
                                              DOM_KEY_LOCATION_LEFTValue);

    ESString* DOM_KEY_LOCATION_RIGHTString =
        ESString::create("DOM_KEY_LOCATION_RIGHT");
    ESValue DOM_KEY_LOCATION_RIGHTValue = ESValue(0x02);
    KeyboardEventPrototypeObj->defineDataProperty(DOM_KEY_LOCATION_RIGHTString,
                                                  false, true, false,
                                                  DOM_KEY_LOCATION_RIGHTValue);

    KeyboardEventFunction->defineDataProperty(DOM_KEY_LOCATION_RIGHTString,
                                              false, true, false,
                                              DOM_KEY_LOCATION_RIGHTValue);

    ESString* DOM_KEY_LOCATION_NUMPADString =
        ESString::create("DOM_KEY_LOCATION_NUMPAD");
    ESValue DOM_KEY_LOCATION_NUMPADValue = ESValue(0x03);
    KeyboardEventPrototypeObj->defineDataProperty(DOM_KEY_LOCATION_NUMPADString,
                                                  false, true, false,
                                                  DOM_KEY_LOCATION_NUMPADValue);

    KeyboardEventFunction->defineDataProperty(DOM_KEY_LOCATION_NUMPADString,
                                              false, true, false,
                                              DOM_KEY_LOCATION_NUMPADValue);

    // Bind for attributes
    ESString* ctrlKeyString = ESString::create("ctrlKey");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        KeyboardEventPrototypeObj, ctrlKeyString, ctrlKeyGetterFunction,
        nullptr);

    ESString* shiftKeyString = ESString::create("shiftKey");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        KeyboardEventPrototypeObj, shiftKeyString, shiftKeyGetterFunction,
        nullptr);

    ESString* altKeyString = ESString::create("altKey");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        KeyboardEventPrototypeObj, altKeyString, altKeyGetterFunction, nullptr);

    ESString* metaKeyString = ESString::create("metaKey");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        KeyboardEventPrototypeObj, metaKeyString, metaKeyGetterFunction,
        nullptr);

    ESString* keyCodeString = ESString::create("keyCode");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        KeyboardEventPrototypeObj, keyCodeString, keyCodeGetterFunction,
        nullptr);

    // Bind for functions
    return KeyboardEventFunction;
}

void KeyboardEvent::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnKeyboardEvent()->protoType());

    postInit(instance);
}

bool KeyboardEvent::isKeyboardEvent() const
{
    return true;
}
}
