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

static ESValue typeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement() &&
        nd->asElement()->asHTMLElement()->isHTMLStyleElement()) {
        return toJSString(nd->asElement()->getAttribute(
            nd->document()->window()->starFish()->staticStrings()->m_type));
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue typeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement() &&
        nd->asElement()->asHTMLElement()->isHTMLStyleElement()) {
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        nd->asElement()->setAttribute(
            nd->document()->window()->starFish()->staticStrings()->m_type,
            toBrowserString(v.toString()));
        return ESValue();
    }
    THROW_ILLEGAL_INVOCATION();
    return ESValue();
}

ESFunctionObject* bindingHTMLStyleElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        HTMLStyleElement, fetchData(scriptBindingInstance)->fnHTMLElement());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLStyleElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("type"), typeGetterFunction, typeSetterFunction);
    return HTMLStyleElementFunction;
}
}
