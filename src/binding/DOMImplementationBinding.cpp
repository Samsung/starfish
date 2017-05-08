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

#if defined(STARFISH_EXP)
#include "dom/DOMImplementation.h"

namespace StarFish {

using namespace escargot;

static ESValue createHTMLDocumentFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMImplementation);
    DOMImplementation* impl = originalObj;
    if (impl) {
        Document* doc = impl->createHTMLDocument();
        if (doc) {
            return doc->scriptValue();
        }
    }
    return ESValue(ESValue::ESNull);
}

ESFunctionObject* bindingDOMPImplemntation(
    ScriptBindingInstance* scriptBindingInstance)
{
    ESString* DOMImplementationString = ESString::create("DOMImplementation");
    ESFunctionObject* DOMImplementationFunction =
        ESFunctionObject::create(nullptr, errorOnConstructorFunction,
                                 DOMImplementationString, 0, true, true);
    ESObject* DOMImplementationPrototypeObj =
        DOMImplementationFunction->protoType().asESPointer()->asESObject();
    DOMImplementationFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    DOMImplementationPrototypeObj->forceNonVectorHiddenClass(false);
    DOMImplementationFunction->set__proto__(fetchData(scriptBindingInstance)
                                                ->m_instance->globalObject()
                                                ->objectPrototype());

    ESString* createHTMLDocumentString = ESString::create("createHTMLDocument");
    ESFunctionObject* createHTMLDocumentESFn =
        ESFunctionObject::create(nullptr, createHTMLDocumentFunction,
                                 createHTMLDocumentString, 1, false);
    DOMImplementationPrototypeObj->defineDataProperty(
        createHTMLDocumentString, true, true, true, createHTMLDocumentESFn);
}
}
#endif
