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
    /*
        Todo : bind to Constructor of DOMQuad
        [Constructor(optional DOMPointInit p1, optional DOMPointInit p2,
       optional DOMPointInit p3, optional DOMPointInit p4),
         Constructor(optional DOMRectInit rect),Exposed=(Window,Worker)]
    */
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(DOMQuad, fetchData(scriptBindingInstance)
                                                 ->m_instance->globalObject()
                                                 ->objectPrototype());
    ESObject* DOMQuadPrototypeObj =
        DOMQuadFunction->protoType().asESPointer()->asESObject();

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
