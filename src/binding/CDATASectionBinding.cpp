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

#include "dom/CDATASection.h"

namespace StarFish {

using namespace escargot;

ESFunctionObject* bindingCDATASection(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* CDATASectionString = ESString::create("CDATASection");
    ESFunctionObject* CDATASectionFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, CDATASectionString, 0, true, true);
    ESObject* CDATASectionPrototypeObj =
        CDATASectionFunction->protoType().asESPointer()->asESObject();
    CDATASectionFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    CDATASectionPrototypeObj->forceNonVectorHiddenClass(false);
    CDATASectionPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnText()->protoType());
    CDATASectionFunction->set__proto__(
        fetchData(scriptBindingInstance)->fnText());

    return CDATASectionFunction;
}

void CDATASection::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnCDATASection()->protoType());

    postInit(instance);
}

bool CDATASection::isCDATASection() const
{
    return true;
}
}
