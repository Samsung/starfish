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

#ifdef STARFISH_ENABLE_DOMPARSER
#include "StarFishConfig.h"
#include "ScriptBindingInstance.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/DOMParser.h"

namespace StarFish {

using namespace escargot;

static ESValue domParserFunction(ESVMInstance* instance)
{
    StarFish* starFish = fetchStarFish(instance);
    auto v = new DOMParser(starFish);
    return v->scriptValue();
}

static ESValue parseFromStringFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMParser);

    if (instance->currentInstance()
            ->currentExecutionContext()
            ->argumentCount() < 2) {
        auto msg = ESString::create(
            "Failed to execute 'parseFromString' on "
            "'DOMParser': "
            "needs 2 parameter.");
        instance->throwError(ESValue(TypeError::create(msg)));
    }

    try {
        Document* doc = originalObj->parseFromString(
            toBrowserString(instance->currentInstance()
                                ->currentExecutionContext()
                                ->readArgument(0)),
            toBrowserString(instance->currentInstance()
                                ->currentExecutionContext()
                                ->readArgument(1)));
        return doc->scriptValue();
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

ESFunctionObject* bindingDOMParser(ScriptBindingInstance* scriptBindingInstance)
{
    /* XMLHttpRequest */
    ESFunctionObject* fnDomParser = ESFunctionObject::create(
        NULL, domParserFunction, ESString::create("DOMParser"), 0, true, false);

    fnDomParser->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    fnDomParser->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)
            ->m_instance->globalObject()
            ->objectPrototype());
    fetchData(scriptBindingInstance)
        ->m_instance->globalObject()
        ->defineDataProperty(ESString::create("DOMParser"), false, false, false,
                             fnDomParser);

    fnDomParser->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("parseFromString"), true, true, true,
        ESFunctionObject::create(nullptr, parseFromStringFunction,
                                 ESString::create("parseFromString"), 2));

    return fnDomParser;
}
}
#endif
