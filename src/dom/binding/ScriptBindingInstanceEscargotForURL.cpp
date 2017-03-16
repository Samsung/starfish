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

namespace StarFish {

using namespace escargot;

ESFunctionObject* bindingURL(ScriptBindingInstance* scriptBindingInstance)
{
#ifdef STARFISH_ENABLE_TEST
    ESFunctionObject* URLFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            int argCount = instance->currentExecutionContext()->argumentCount();
            if (argCount < 1) {
                // throw error
            } else if (argCount == 1) {
                ESValue urlString =
                    instance->currentExecutionContext()->readArgument(0);
                auto url = URL::createURL(
                    String::emptyString,
                    String::fromUTF8(urlString.asESString()->utf8Data()));
                return url->scriptValue();
            } else { // ignore redundant arguments
                ESValue urlString =
                    instance->currentExecutionContext()->readArgument(0);
                ESValue baseURLString =
                    instance->currentExecutionContext()->readArgument(1);
                // FIXME second argument can be not only string but also
                // object
                STARFISH_ASSERT(baseURLString.isESString());

                auto url = URL::createURL(
                    String::fromUTF8(baseURLString.asESString()->utf8Data()),
                    String::fromUTF8(urlString.asESString()->utf8Data()));
                return url->scriptValue();
            }
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        },
        ESString::create("URL"), 2, true, true);

    URLFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    URLFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    URLFunction->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)
            ->m_instance->globalObject()
            ->objectPrototype());
    URLFunction->set__proto__(fetchData(scriptBindingInstance)
                                  ->m_instance->globalObject()
                                  ->objectPrototype());
#else
    DEFINE_FUNCTION_NOT_CONSTRUCTOR(URL, fetchData(scriptBindingInstance)
                                             ->m_instance->globalObject()
                                             ->objectPrototype());
#endif
    ESFunctionObject* URLCreateObjectURLFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
            if (arg0.isObject() &&
                (arg0.asObject()->extraData() == kEscargotObjectCheckMagic) &&
                ((ScriptWrappable*)arg0.asObject()->extraPointerData())
                        ->type() == ScriptWrappable::Type::BlobObject) {
                Blob* b = (Blob*)arg0.toObject()->extraPointerData();
                String* url = URL::createObjectURL(b);
                return toJSString(url);
#ifdef STARFISH_ENABLE_MULTIMEDIA
            } else if (arg0.isObject() && (arg0.asObject()->extraData() ==
                                           kEscargotObjectCheckMagic) &&
                       ((ScriptWrappable*)arg0.asObject()->extraPointerData())
                               ->type() ==
                           ScriptWrappable::Type::MediaSourceObject) {
                MediaSource* m =
                    (MediaSource*)arg0.toObject()->extraPointerData();
                String* url = URL::createObjectURL(m);
                return toJSString(url);
#endif
            } else {
                ESString* msg = ESString::create(
                    "Failed to execute 'createObjectURL' on 'URL': No "
                    "function was found that matched the signature "
                    "provided.");
                instance->throwError(ESValue(TypeError::create(msg)));
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        },
        ESString::create("createObjectURL"), 1, false);
    URLFunction->defineDataProperty(ESString::create("createObjectURL"), false,
                                    false, false, URLCreateObjectURLFunction);

    ESFunctionObject* URLRevokeObjectURLFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            String* arg0 = toBrowserString(instance->currentExecutionContext()
                                               ->readArgument(0)
                                               .toString());
            StarFish* sf =
                ((Window*)instance->globalObject()->extraPointerData())
                    ->starFish();
            URL::revokeObjectURL(sf, arg0);
            return ESValue();
        },
        ESString::create("revokeObjectURL"), 1, false);
    URLFunction->defineDataProperty(ESString::create("revokeObjectURL"), false,
                                    false, false, URLRevokeObjectURLFunction);

#ifdef STARFISH_ENABLE_TEST
    // FIXME setters below should not be null
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(),
        ESString::create("href"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
            return toJSString(originalObj->getHref());
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(),
        ESString::create("origin"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
            return toJSString(originalObj->origin());
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(),
        ESString::create("protocol"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
            return toJSString(originalObj->getProtocol());
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(),
        ESString::create("username"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
            return toJSString(originalObj->getUsername());
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
            int argCount = instance->currentExecutionContext()->argumentCount();
            if (argCount < 1) {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            } else {
                ESValue arg =
                    instance->currentExecutionContext()->readArgument(0);
                ESString* argString = arg.toString();
                originalObj->setUsername(
                    String::fromUTF8(argString->utf8Data()));
            }
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(),
        ESString::create("password"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
            return toJSString(originalObj->getPassword());
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
            int argCount = instance->currentExecutionContext()->argumentCount();
            if (argCount < 1) {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            } else {
                ESValue arg =
                    instance->currentExecutionContext()->readArgument(0);
                ESString* argString = arg.toString();
                originalObj->setPassword(
                    String::fromUTF8(argString->utf8Data()));
            }
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(),
        ESString::create("host"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
            return toJSString(originalObj->getHost());
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(),
        ESString::create("hostname"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
            return toJSString(originalObj->getHostname());
        },
        nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(),
        ESString::create("port"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
            return toJSString(originalObj->getPort());
        },
        nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(),
        ESString::create("pathname"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
            return toJSString(originalObj->getPathname());
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
            int argCount = instance->currentExecutionContext()->argumentCount();
            if (argCount < 1) {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            } else {
                ESValue arg =
                    instance->currentExecutionContext()->readArgument(0);
                ESString* argString = arg.toString();
                originalObj->setPathname(
                    String::fromUTF8(argString->utf8Data()));
            }
            return ESValue();
        });
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(),
        ESString::create("search"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
            return toJSString(originalObj->getSearch());
        },
        nullptr);
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(),
        ESString::create("hash"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
            return toJSString(originalObj->getHash());
        },
        nullptr);
/*
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        URLFunction->protoType().asESPointer()->asESObject(),
        ESString::create("searchParams"),
        [](ESVMInstance* instance) -> ESValue {
        GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::URLObject, URL);
    }, nullptr);
*/
#endif
    return URLFunction;
}
}
