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

#include "dom/DOMPointReadOnly.h"

namespace StarFish {

using namespace escargot;

// Implement for constructor
static ESValue dompointreadonlyConstructor(ESVMInstance* instance)
{
    if (!instance->currentExecutionContext()->isNewExpression()) {
        THROW_EXCEPTION(CALLED_CONSTRUCTOR_WITHOUT_NEW, "DOMPointReadOnly");
    }
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    ESValue arg2 = instance->currentExecutionContext()->readArgument(2);
    ESValue arg3 = instance->currentExecutionContext()->readArgument(3);
    // Handle argument arg0
    double value0 = 0;
    if (!arg0.isUndefinedOrNull()) {
        value0 = arg0.toNumber();
    }
    // Handle argument arg1
    double value1 = 0;
    if (!arg1.isUndefinedOrNull()) {
        value1 = arg1.toNumber();
    }
    // Handle argument arg2
    double value2 = 0;
    if (!arg2.isUndefinedOrNull()) {
        value2 = arg2.toNumber();
    }
    // Handle argument arg3
    double value3 = 1;
    if (!arg3.isUndefinedOrNull()) {
        value3 = arg3.toNumber();
    }
    DOMPointReadOnly* result = nullptr;
    // Call native function (nargs: 4)
    result = new DOMPointReadOnly(value0, value1, value2, value3);
    return result->scriptValue();
}

// Implement for attributes
static ESValue xGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPointReadOnly);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->x();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue yGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPointReadOnly);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->y();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue zGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPointReadOnly);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->z();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue wGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPointReadOnly);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->w();
    // Return ESValue from native value
    return ESValue(result);
}

// Implement for functions
ESFunctionObject* bindingDOMPointReadOnly(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* DOMPointReadOnlyString = ESString::create("DOMPointReadOnly");
    ESFunctionObject* DOMPointReadOnlyFunction =
        ESFunctionObject::create(nullptr, dompointreadonlyConstructor,
                                 DOMPointReadOnlyString, 0, true, true);
    ESObject* DOMPointReadOnlyPrototypeObj =
        DOMPointReadOnlyFunction->protoType().asESPointer()->asESObject();
    DOMPointReadOnlyFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    DOMPointReadOnlyPrototypeObj->forceNonVectorHiddenClass(false);
    DOMPointReadOnlyPrototypeObj->set__proto__(fetchData(scriptBindingInstance)
                                                   ->m_instance->globalObject()
                                                   ->objectPrototype());

    // Bind for attributes
    ESString* xString = ESString::create("x");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMPointReadOnlyPrototypeObj, xString, xGetterFunction, nullptr);

    ESString* yString = ESString::create("y");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMPointReadOnlyPrototypeObj, yString, yGetterFunction, nullptr);

    ESString* zString = ESString::create("z");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMPointReadOnlyPrototypeObj, zString, zGetterFunction, nullptr);

    ESString* wString = ESString::create("w");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMPointReadOnlyPrototypeObj, wString, wGetterFunction, nullptr);

    // Bind for functions
    return DOMPointReadOnlyFunction;
}
}
