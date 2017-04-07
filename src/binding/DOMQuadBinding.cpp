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

#include "StarFishConfig.h"
#include "ScriptBindingInstance.h"

#include "Binding.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

static ESValue p1GetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMQuad);
    DOMQuad* quad = originalObj;
    return quad->p1()->scriptValue();
}

static ESValue p2GetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMQuad);
    DOMQuad* quad = originalObj;
    return quad->p2()->scriptValue();
}

static ESValue p3GetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMQuad);
    DOMQuad* quad = originalObj;
    return quad->p3()->scriptValue();
}

static ESValue p4GetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMQuad);
    DOMQuad* quad = originalObj;
    return quad->p4()->scriptValue();
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

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMQuadFunction->protoType().asESPointer()->asESObject(),
        ESString::create("p1"), p1GetterFunction, nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMQuadFunction->protoType().asESPointer()->asESObject(),
        ESString::create("p2"), p2GetterFunction, nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMQuadFunction->protoType().asESPointer()->asESObject(),
        ESString::create("p3"), p3GetterFunction, nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMQuadFunction->protoType().asESPointer()->asESObject(),
        ESString::create("p4"), p4GetterFunction, nullptr, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DOMQuadFunction->protoType().asESPointer()->asESObject(),
        ESString::create("bounds"), boundsGetterFunction, nullptr, true, true);

    return DOMQuadFunction;
}
}
