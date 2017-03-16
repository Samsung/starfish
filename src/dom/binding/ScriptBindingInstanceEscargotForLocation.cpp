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

#include "dom/DOM.h"
#include "dom/binding/escargot/ScriptBindingInstanceDataEscargot.h"
#include "extra/Location.h"

namespace StarFish {

using namespace escargot;

ESFunctionObject* bindingLocation(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(Location, fetchData(scriptBindingInstance)
                                                  ->m_instance->globalObject()
                                                  ->objectPrototype());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationFunction->protoType().asESPointer()->asESObject(),
        ESString::create("href"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject,
                                         LocationObj);
            return toJSString(originalObj->getHref());
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject,
                                         LocationObj);
            int argCount = instance->currentExecutionContext()->argumentCount();
            if (argCount < 1) {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            } else {
                originalObj->setHref(
                    String::fromUTF8(instance->currentExecutionContext()
                                         ->readArgument(0)
                                         .toString()
                                         ->utf8Data()));
            }
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationFunction->protoType().asESPointer()->asESObject(),
        ESString::create("pathname"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject,
                                         LocationObj);
            return toJSString(originalObj->getPathname());
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject,
                                         LocationObj);
            int argCount = instance->currentExecutionContext()->argumentCount();
            if (argCount < 1) {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            } else {
                originalObj->setPathname(
                    String::fromUTF8(instance->currentExecutionContext()
                                         ->readArgument(0)
                                         .toString()
                                         ->utf8Data()));
            }
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationFunction->protoType().asESPointer()->asESObject(),
        ESString::create("search"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject,
                                         LocationObj);
            return toJSString(originalObj->getSearch());
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject,
                                         LocationObj);
            int argCount = instance->currentExecutionContext()->argumentCount();
            if (argCount < 1) {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            } else {
                originalObj->setSearch(
                    String::fromUTF8(instance->currentExecutionContext()
                                         ->readArgument(0)
                                         .toString()
                                         ->utf8Data()));
            }
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationFunction->protoType().asESPointer()->asESObject(),
        ESString::create("hash"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject,
                                         LocationObj);
            return toJSString(originalObj->getHash());
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject,
                                         LocationObj);
            int argCount = instance->currentExecutionContext()->argumentCount();
            if (argCount < 1) {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            } else {
                originalObj->setHash(
                    String::fromUTF8(instance->currentExecutionContext()
                                         ->readArgument(0)
                                         .toString()
                                         ->utf8Data()));
            }
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationFunction->protoType().asESPointer()->asESObject(),
        ESString::create("host"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject,
                                         LocationObj);
            return toJSString(originalObj->getHost());
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationFunction->protoType().asESPointer()->asESObject(),
        ESString::create("hostname"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject,
                                         LocationObj);
            return toJSString(originalObj->getHostname());
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        LocationFunction->protoType().asESPointer()->asESObject(),
        ESString::create("protocol"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject,
                                         LocationObj);
            return toJSString(originalObj->getProtocol());
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::LocationObject,
                                         LocationObj);
            int argCount = instance->currentExecutionContext()->argumentCount();
            if (argCount < 1) {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            } else {
                originalObj->setProtocol(
                    String::fromUTF8(instance->currentExecutionContext()
                                         ->readArgument(0)
                                         .toString()
                                         ->utf8Data()));
            }
            return ESValue();
        });

    return LocationFunction;
}
}
