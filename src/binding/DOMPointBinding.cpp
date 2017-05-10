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

#include "dom/DOMPoint.h"

namespace StarFish {

using namespace escargot;

// Implement for constructor
static ESValue dompointConstructor(ESVMInstance* instance)
{
    if (!instance->currentExecutionContext()->isNewExpression()) {
        COMPOSE_MESSAGE(msg, CALLED_CONSTRUCTOR_WITHOUT_NEW, "DOMPoint");
        THROW_EXCEPTION(msg);
    }
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    ESValue arg2 = instance->currentExecutionContext()->readArgument(2);
    ESValue arg3 = instance->currentExecutionContext()->readArgument(3);
    // Handle argument arg3
    double value3 = 1;
    if (!arg3.isUndefinedOrNull()) {
        value3 = arg3.toNumber();
    }
    // Handle argument arg2
    double value2 = 0;
    if (!arg2.isUndefinedOrNull()) {
        value2 = arg2.toNumber();
    }
    // Handle argument arg1
    double value1 = 0;
    if (!arg1.isUndefinedOrNull()) {
        value1 = arg1.toNumber();
    }
    // Handle argument arg0
    double value0 = 0;
    if (!arg0.isUndefinedOrNull()) {
        value0 = arg0.toNumber();
    }
    DOMPoint* result = nullptr;
    // Call native function (nargs: 4)
    result = new DOMPoint(value0, value1, value2, value3);
    return result->scriptValue();
}

// Implement for attributes
static ESValue xGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPoint);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->x();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue xSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPoint);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();
    originalObj->setX(value0);
    return ESValue();
}

static ESValue yGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPoint);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->y();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue ySetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPoint);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();
    originalObj->setY(value0);
    return ESValue();
}

static ESValue zGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPoint);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->z();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue zSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPoint);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();
    originalObj->setZ(value0);
    return ESValue();
}

static ESValue wGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPoint);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->w();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue wSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPoint);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();
    originalObj->setW(value0);
    return ESValue();
}

// Implement for functions
ESFunctionObject* bindingDOMPoint(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* DOMPointString = ESString::create("DOMPoint");
    ESFunctionObject* DOMPointFunction = ESFunctionObject::create(
        nullptr, dompointConstructor, DOMPointString, 0, true, true);
    ESObject* DOMPointPrototypeObj =
        DOMPointFunction->protoType().asESPointer()->asESObject();
    DOMPointFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    DOMPointPrototypeObj->forceNonVectorHiddenClass(false);
    DOMPointPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnDOMPointReadOnly()->protoType());
    DOMPointFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnDOMPointReadOnly());

    // Bind for attributes
    ESString* xString = ESString::create("x");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMPointPrototypeObj, xString, xGetterFunction, xSetterFunction);

    ESString* yString = ESString::create("y");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMPointPrototypeObj, yString, yGetterFunction, ySetterFunction);

    ESString* zString = ESString::create("z");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMPointPrototypeObj, zString, zGetterFunction, zSetterFunction);

    ESString* wString = ESString::create("w");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMPointPrototypeObj, wString, wGetterFunction, wSetterFunction);

    // Bind for functions

    return DOMPointFunction;
}

void DOMPoint::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnDOMPoint()->protoType());

    postInit(instance);
}

bool DOMPoint::isDOMPoint() const
{
    return true;
}
}
