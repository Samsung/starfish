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
        COMPOSE_MESSAGE(msg, CALLED_CONSTRUCTOR_WITHOUT_NEW, "DOMParser");
        THROW_EXCEPTION(msg);
    }
    DOMParser* result = nullptr;
    Document* callWith = fetchDocument(instance);
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
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "2", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "parseFromString", "DOMParser",
                        reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    Document* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    // Handle argument arg1
    String* value1 = String::emptyString;
    value1 = toBrowserString(arg1);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
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
    ESObject* DOMParserPrototypeObj =
        DOMParserFunction->protoType().asESPointer()->asESObject();
    DOMParserFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    DOMParserPrototypeObj->forceNonVectorHiddenClass(false);
    DOMParserPrototypeObj->set__proto__(fetchData(scriptBindingInstance)
                                            ->m_instance->globalObject()
                                            ->objectPrototype());
    // Bind for functions
    ESString* parseFromStringString = ESString::create("parseFromString");
    ESFunctionObject* parseFromStringESFn = ESFunctionObject::create(
        nullptr, parseFromStringFunction, parseFromStringString, 2, false);
    DOMParserPrototypeObj->defineDataProperty(
        parseFromStringString, true /* writable */, true /* enumerable */,
        true /* configurable */, parseFromStringESFn);

    return DOMParserFunction;
}

void DOMParser::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnDOMParser()->protoType());
    // Bind for functions

    postInit(instance);
}

bool DOMParser::isDOMParser() const
{
    return true;
}
}
#endif
