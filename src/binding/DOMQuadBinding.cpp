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
#include "dom/DOMRect.h"
#include "dom/DOMQuad.h"

namespace StarFish {

using namespace escargot;

extern DOMPointInit toDOMPointInitFromESValue(ESVMInstance* instance,
                                              ESValue& from);
extern ESValue toESValueFromDOMPointInit(ESVMInstance* instance,
                                         DOMPointInit& from);

// Implement for constructor
static ESValue domquadConstructor(ESVMInstance* instance)
{
    if (!instance->currentExecutionContext()->isNewExpression()) {
        THROW_EXCEPTION(CALLED_CONSTRUCTOR_WITHOUT_NEW, "DOMQuad");
    }
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 4) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_CONSTRUCT_BECAUSE_ARGS_NOT_ENOUGH, "DOMQuad",
                        "4", buffer);
    }
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    ESValue arg2 = instance->currentExecutionContext()->readArgument(2);
    ESValue arg3 = instance->currentExecutionContext()->readArgument(3);
    // Handle argument arg3
    DOMPointInit value3;
    value3 = toDOMPointInitFromESValue(instance, arg3);

    // Handle argument arg2
    DOMPointInit value2;
    value2 = toDOMPointInitFromESValue(instance, arg2);

    // Handle argument arg1
    DOMPointInit value1;
    value1 = toDOMPointInitFromESValue(instance, arg1);

    // Handle argument arg0
    DOMPointInit value0;
    value0 = toDOMPointInitFromESValue(instance, arg0);

    DOMQuad* result = nullptr;
    // Call native function (nargs: 4)
    result = new DOMQuad(value0, value1, value2, value3);
    return result->scriptValue();
}

// Implement for attributes
static ESValue p1GetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMQuad);
    // Declare native value (empty when type is void)
    DOMPoint* result = nullptr;
    result = originalObj->p1();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue p2GetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMQuad);
    // Declare native value (empty when type is void)
    DOMPoint* result = nullptr;
    result = originalObj->p2();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue p3GetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMQuad);
    // Declare native value (empty when type is void)
    DOMPoint* result = nullptr;
    result = originalObj->p3();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue p4GetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMQuad);
    // Declare native value (empty when type is void)
    DOMPoint* result = nullptr;
    result = originalObj->p4();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue boundsGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMQuad);
    DOMQuad* quad = originalObj;
    return quad->bounds()->scriptValue();
}

ESFunctionObject* bindingDOMQuad(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* DOMQuadString = ESString::create("DOMQuad");
    ESFunctionObject* DOMQuadFunction = ESFunctionObject::create(
        nullptr, domquadConstructor, DOMQuadString, 4, true, true);
    ESObject* DOMQuadPrototypeObj =
        DOMQuadFunction->protoType().asESPointer()->asESObject();
    DOMQuadFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    DOMQuadPrototypeObj->forceNonVectorHiddenClass(false);
    DOMQuadPrototypeObj->set__proto__(fetchData(scriptBindingInstance)
                                          ->m_instance->globalObject()
                                          ->objectPrototype());

    // Bind for attributes
    ESString* p1String = ESString::create("p1");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMQuadPrototypeObj, p1String, p1GetterFunction, nullptr);

    ESString* p2String = ESString::create("p2");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMQuadPrototypeObj, p2String, p2GetterFunction, nullptr);

    ESString* p3String = ESString::create("p3");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMQuadPrototypeObj, p3String, p3GetterFunction, nullptr);

    ESString* p4String = ESString::create("p4");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMQuadPrototypeObj, p4String, p4GetterFunction, nullptr);

    ESString* boundsString = ESString::create("bounds");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMQuadPrototypeObj, boundsString, boundsGetterFunction, nullptr);

    // Bind for functions
    return DOMQuadFunction;
}
}
