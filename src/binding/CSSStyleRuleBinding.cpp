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

#include "dom/CSSStyleRule.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
ESFunctionObject* bindingCSSStyleRule(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* CSSStyleRuleString = ESString::create("CSSStyleRule");
    ESFunctionObject* CSSStyleRuleFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, CSSStyleRuleString, 0, true, true);
    ESObject* CSSStyleRulePrototypeObj =
        CSSStyleRuleFunction->protoType().asESPointer()->asESObject();
    CSSStyleRuleFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    CSSStyleRulePrototypeObj->forceNonVectorHiddenClass(false);
    CSSStyleRulePrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnCSSRule()->protoType());
    CSSStyleRuleFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnCSSRule());

    // Bind for attributes

    return CSSStyleRuleFunction;
}

void CSSStyleRule::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnCSSStyleRule()->protoType());

    postInit(instance);
}

bool CSSStyleRule::isCSSStyleRule() const
{
    return true;
}
}
