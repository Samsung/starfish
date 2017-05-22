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

#ifdef STARFISH_ENABLE_MULTIMEDIA
#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)
#include "StarFishConfig.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "core/dom/DOMException.h"
#include "core/extra/Avplay.h"
#include "core/extra/WebApis.h"

namespace StarFish {

using namespace escargot;

static ESValue avplayGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(WebApis);
    Avplay* avplay = originalObj->avplay();

    if (avplay == nullptr) {
        return ESValue(ESValue::ESNull);
    }

    return avplay->scriptObject();
}

ESFunctionObject* bindingwebapis(ScriptBindingInstance* scriptBindingInstance)
{
    ESString* webapisString = ESString::create("webapis");
    ESFunctionObject* webapisFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, webapisString, 0, true, true);
    ESObject* webapisPrototypeObj =
        webapisFunction->protoType().asESPointer()->asESObject();
    webapisFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    webapisPrototypeObj->forceNonVectorHiddenClass(false);
    webapisFunction->set__proto__(fetchData(scriptBindingInstance)
                                      ->m_instance->globalObject()
                                      ->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        webapisFunction, ESString::create("avplay"), avplayGetterFunction,
        nullptr);
    return webapisFunction;
}
}
#endif
#endif
