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

#include "dom/Document.h"
#include "dom/DOMImplementation.h"

namespace StarFish {

using namespace escargot;

// Implement for functions
static ESValue createHTMLDocumentFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMImplementation);
    size_t validArgCount = 1;
    // Declare native value (empty when type is void)
    Document* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    if (arg0.isUndefined()) {
        validArgCount--;
    } else {
        value0 = toBrowserString(arg0);
    }
    // Call native function (nargs: 0-1)
    if (validArgCount == 0) {
        result = originalObj->createHTMLDocument();
    } else if (validArgCount == 1) {
        result = originalObj->createHTMLDocument(value0);
    }

    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

ESFunctionObject* bindingDOMImplementation(
    ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
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
    DOMImplementationPrototypeObj->set__proto__(fetchData(scriptBindingInstance)
                                                    ->m_instance->globalObject()
                                                    ->objectPrototype());
    // Bind for functions
    ESString* createHTMLDocumentString = ESString::create("createHTMLDocument");
    ESFunctionObject* createHTMLDocumentESFn =
        ESFunctionObject::create(nullptr, createHTMLDocumentFunction,
                                 createHTMLDocumentString, 0, false);
    DOMImplementationPrototypeObj->defineDataProperty(
        createHTMLDocumentString, true /* writable */, true /* enumerable */,
        true /* configurable */, createHTMLDocumentESFn);

    return DOMImplementationFunction;
}

void DOMImplementation::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnDOMImplementation()->protoType());
    // Bind for functions

    postInit(instance);
}

bool DOMImplementation::isDOMImplementation() const
{
    return true;
}
}
#endif
