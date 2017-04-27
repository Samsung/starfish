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

#include "dom/DOMRect.h"

namespace StarFish {

using namespace escargot;

// Implement for constructor
static ESValue domrectConstructor(ESVMInstance* instance)
{
    if (!instance->currentExecutionContext()->isNewExpression()) {
        THROW_EXCEPTION(CALLED_CONSTRUCTOR_WITHOUT_NEW, "DOMRect");
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
    double value3 = 0;
    if (!arg3.isUndefinedOrNull()) {
        value3 = arg3.toNumber();
    }
    DOMRect* result = nullptr;
    // Call native function (nargs: 4)
    result = new DOMRect(value0, value1, value2, value3);
    return result->scriptValue();
}

// Implement for attributes
static ESValue xGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRect);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->x();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue xSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRect);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();
    originalObj->setX(value0);
    return ESValue();
}

static ESValue yGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRect);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->y();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue ySetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRect);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();
    originalObj->setY(value0);
    return ESValue();
}

static ESValue widthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRect);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->width();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue widthSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRect);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();
    originalObj->setWidth(value0);
    return ESValue();
}

static ESValue heightGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRect);
    // Declare native value (empty when type is void)
    double result;
    result = originalObj->height();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue heightSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRect);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();
    originalObj->setHeight(value0);
    return ESValue();
}

// Implement for functions
ESFunctionObject* bindingDOMRect(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* DOMRectString = ESString::create("DOMRect");
    ESFunctionObject* DOMRectFunction = ESFunctionObject::create(
        nullptr, domrectConstructor, DOMRectString, 0, true, true);
    ESObject* DOMRectPrototypeObj =
        DOMRectFunction->protoType().asESPointer()->asESObject();
    DOMRectFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    DOMRectPrototypeObj->forceNonVectorHiddenClass(false);
    DOMRectPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnDOMRectReadOnly()->protoType());
    DOMRectFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnDOMRectReadOnly());

    // Bind for attributes
    ESString* xString = ESString::create("x");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectPrototypeObj, xString, xGetterFunction, xSetterFunction);

    ESString* yString = ESString::create("y");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectPrototypeObj, yString, yGetterFunction, ySetterFunction);

    ESString* widthString = ESString::create("width");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectPrototypeObj, widthString, widthGetterFunction,
        widthSetterFunction);

    ESString* heightString = ESString::create("height");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMRectPrototypeObj, heightString, heightGetterFunction,
        heightSetterFunction);

    // Bind for functions
    return DOMRectFunction;
}
}
