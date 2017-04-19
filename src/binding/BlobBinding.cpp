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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "extra/Blob.h"

namespace StarFish {

using namespace escargot;

extern ESValue blobConstructor(ESVMInstance* instance);

static ESValue sizeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Blob);
    double v = originalObj->size();
    return ESValue(v);
}

static ESValue typeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Blob);
    String* v = originalObj->mimeType();
    return toJSString(v);
}

static ESValue sliceFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Blob);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    ESValue arg2 = instance->currentExecutionContext()->readArgument(2);

    // FIXME range of size_t and int64_t is not match!
    int64_t start = 0;
    int64_t end = (int64_t)originalObj->size();
    if (!arg0.isUndefinedOrNull()) {
        start = arg0.toNumber();
    }
    if (!arg1.isUndefinedOrNull()) {
        end = arg1.toNumber();
    }
    String* type = String::emptyString;
    if (!arg2.isUndefinedOrNull()) {
        type = toBrowserString(arg2.toString());
    }
    Blob* b = originalObj->slice(start, end, type);
    return b->scriptValue();
}

ESFunctionObject* bindingBlob(ScriptBindingInstance* scriptBindingInstance)
{
    /* Blob */
    auto fnBlob = ESFunctionObject::create(
        NULL, blobConstructor, ESString::create("Blob"), 0, true, true);
    fnBlob->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    fnBlob->protoType().asESPointer()->asESObject()->forceNonVectorHiddenClass(
        false);
    fnBlob->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)
            ->m_instance->globalObject()
            ->objectPrototype());
    fnBlob->set__proto__(fetchData(scriptBindingInstance)
                             ->m_instance->globalObject()
                             ->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnBlob->protoType().asESPointer()->asESObject(),
        ESString::create("size"), sizeGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnBlob->protoType().asESPointer()->asESObject(),
        ESString::create("type"), typeGetterFunction, nullptr);

    fnBlob->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("slice"), false, false, false,
        ESFunctionObject::create(NULL, sliceFunction, ESString::create("slice"),
                                 0, false));

    return fnBlob;
}
}
