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
#if defined(STARFISH_ENABLE_DOMPARSER)
#include "dom/DOMException.h"
#include "dom/Document.h"
#include "dom/DOMParser.h"

namespace StarFish {

using namespace escargot;

// Implement for constructor
static ESValue domparserConstructor(ESVMInstance* instance)
{
    if (!instance->currentExecutionContext()->isNewExpression()) {
        THROW_EXCEPTION(CALLED_CONSTRUCTOR_WITHOUT_NEW, "DOMParser");
    }
    DOMParser* result = nullptr;
    StarFish* callWith = fetchStarFish(instance);
    // Call native function (nargs: 0)
    result = new DOMParser(callWith);
    return result->scriptValue();
}

// Implement for functions
static ESValue parseFromStringFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMParser);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                        "parseFromString", "DOMParser", "2", buffer);
    }
    // Declare native value (empty when type is void)
    Document* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Handle argument arg1
    String* value1 = String::emptyString;
    value1 = toBrowserString(arg1);

    // Call native function (nargs: 2)
    try {
        result = originalObj->parseFromString(value0, value1);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

ESFunctionObject* bindingDOMParser(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* DOMParserString = ESString::create("DOMParser");
    ESFunctionObject* DOMParserFunction = ESFunctionObject::create(
        nullptr, domparserConstructor, DOMParserString, 0, true, true);
    DOMParserFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    DOMParserFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    DOMParserFunction->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)
            ->m_instance->globalObject()
            ->objectPrototype());
    ESObject* DOMParserPrototypeObj =
        DOMParserFunction->protoType().asESPointer()->asESObject();

    // Bind for functions
    ESString* parseFromStringString = ESString::create("parseFromString");
    ESFunctionObject* parseFromStringESFn = ESFunctionObject::create(
        nullptr, parseFromStringFunction, parseFromStringString, 2, false);
    DOMParserPrototypeObj->defineDataProperty(parseFromStringString, true, true,
                                              true, parseFromStringESFn);

    return DOMParserFunction;
}
}
#endif
