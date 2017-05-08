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

#include "extra/Blob.h"

namespace StarFish {

using namespace escargot;

// Implement for constructor
extern ESValue blobConstructor(ESVMInstance* instance);

// Implement for attributes
static ESValue sizeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Blob);
    // Declare native value (empty when type is void)
    uint64_t result;
    result = originalObj->size();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue typeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Blob);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->type();
    // Return ESValue from native value
    return toJSString(result);
}

// Implement for functions
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
    // Bind for constructor
    ESString* BlobString = ESString::create("Blob");
    ESFunctionObject* BlobFunction = ESFunctionObject::create(
        nullptr, blobConstructor, BlobString, 0, true, true);
    ESObject* BlobPrototypeObj =
        BlobFunction->protoType().asESPointer()->asESObject();
    BlobFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    BlobPrototypeObj->forceNonVectorHiddenClass(false);
    BlobPrototypeObj->set__proto__(fetchData(scriptBindingInstance)
                                       ->m_instance->globalObject()
                                       ->objectPrototype());

    // Bind for attributes
    ESString* sizeString = ESString::create("size");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        BlobPrototypeObj, sizeString, sizeGetterFunction, nullptr);

    ESString* typeString = ESString::create("type");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        BlobPrototypeObj, typeString, typeGetterFunction, nullptr);

    // Bind for functions
    ESString* sliceString = ESString::create("slice");
    ESFunctionObject* sliceESFn =
        ESFunctionObject::create(nullptr, sliceFunction, sliceString, 0, false);
    BlobPrototypeObj->defineDataProperty(sliceString, true, true, true,
                                         sliceESFn);

    return BlobFunction;
}
}
