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

#include "dom/DOMException.h"
#include "dom/HTMLCollection.h"
#include "dom/DOMTokenList.h"
#include "dom/NamedNodeMap.h"
#include "dom/DOMRect.h"
#include "dom/NodeList.h"
#include "dom/DOMRectList.h"
#include "dom/Element.h"
#include "dom/Element.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
extern ESValue namespaceURIElementGetterFunction(ESVMInstance* instance);

static ESValue localNameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->localName();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue tagNameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->tagName();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue idGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->idAttr();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue idSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setIdAttr(value0);
    return ESValue();
}

static ESValue classNameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->className();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue classNameSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);
    originalObj->setClassName(value0);
    return ESValue();
}

static ESValue classListGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    DOMTokenList* result = nullptr;
    result = originalObj->classList();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

extern ESValue classListElementSetterFunction(ESVMInstance* instance);

static ESValue attributesGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    NamedNodeMap* result = nullptr;
    result = originalObj->attributes();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

#ifdef STARFISH_ENABLE_TEST
static ESValue innerHTMLGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    String* result = String::emptyString;
    result = originalObj->innerHTML();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue innerHTMLSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    if (!arg0.isNull()) {
        value0 = toBrowserString(arg0);
    }
    originalObj->setInnerHTML(value0);
    return ESValue();
}
#endif

static ESValue clientTopGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    int32_t result;
    result = originalObj->clientTop();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue clientLeftGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    int32_t result;
    result = originalObj->clientLeft();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue clientWidthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    int32_t result;
    result = originalObj->clientWidth();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue clientHeightGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    int32_t result;
    result = originalObj->clientHeight();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue childrenGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    HTMLCollection* result = nullptr;
    result = originalObj->children();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue firstElementChildGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    Element* result = nullptr;
    result = originalObj->firstElementChild();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue lastElementChildGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    Element* result = nullptr;
    result = originalObj->lastElementChild();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue childElementCountGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    uint32_t result;
    result = originalObj->childElementCount();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue previousElementSiblingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    Element* result = nullptr;
    result = originalObj->previousElementSibling();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue nextElementSiblingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    Element* result = nullptr;
    result = originalObj->nextElementSibling();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

// Implement for functions
static ESValue getAttributeFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "getAttribute", "Element",
                        reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    Nullable<String*> result = String::emptyString;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    result = originalObj->getAttribute(value0);

    // Return ESValue from native value
    if (!result.hasValue()) {
        return ESValue(ESValue::ESNull);
    }
    String* result_value = result.getValue();
    return toJSString(result_value);
}

static ESValue setAttributeFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "2", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "setAttribute", "Element",
                        reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
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
        originalObj->setAttribute(value0, value1);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue removeAttributeFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "removeAttribute", "Element",
                        reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    originalObj->removeAttribute(value0);

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

static ESValue hasAttributeFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "hasAttribute", "Element",
                        reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    bool result;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    result = originalObj->hasAttribute(value0);

    // Return ESValue from native value
    return ESValue(result);
}

static ESValue getElementsByTagNameFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "getElementsByTagName",
                        "Element", reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    HTMLCollection* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    result = originalObj->getElementsByTagName(value0);

    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue getElementsByClassNameFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "getElementsByClassName",
                        "Element", reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    HTMLCollection* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    result = originalObj->getElementsByClassName(value0);

    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue getClientRectsFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    DOMRectList* result = nullptr;
    // Call native function (nargs: 0)
    result = originalObj->getClientRects();

    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue getBoundingClientRectFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    DOMRect* result = nullptr;
    // Call native function (nargs: 0)
    result = originalObj->getBoundingClientRect();

    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue querySelectorFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "querySelector", "Element",
                        reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    Element* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    try {
        result = originalObj->querySelector(value0);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

// TODO Move throw DOM exception code into querySelectorAll()
static ESValue querySelectorAllFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "querySelectorAll", "Element",
                        reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    NodeList* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    try {
        result = originalObj->querySelectorAll(value0);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue removeFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare native value (empty when type is void)
    // Call native function (nargs: 0)
    originalObj->remove();

    // Return ESValue from native value
    return ESValue(ESValue::ESUndefined);
}

ESFunctionObject* bindingElement(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* ElementString = ESString::create("Element");
    ESFunctionObject* ElementFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, ElementString, 0, true, true);
    ESObject* ElementPrototypeObj =
        ElementFunction->protoType().asESPointer()->asESObject();
    ElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    ElementPrototypeObj->forceNonVectorHiddenClass(false);
    ElementPrototypeObj->set__proto__(
        fetchData(scriptBindingInstance)->fnNode()->protoType());
    ElementFunction->set__proto__(fetchData(scriptBindingInstance)->fnNode());

    // Bind for attributes
    ESString* namespaceURIString = ESString::create("namespaceURI");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementPrototypeObj, namespaceURIString,
        namespaceURIElementGetterFunction, nullptr);

    ESString* localNameString = ESString::create("localName");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementPrototypeObj, localNameString, localNameGetterFunction, nullptr);

    ESString* tagNameString = ESString::create("tagName");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementPrototypeObj, tagNameString, tagNameGetterFunction, nullptr);

    ESString* idString = ESString::create("id");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementPrototypeObj, idString, idGetterFunction, idSetterFunction);

    ESString* classNameString = ESString::create("className");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementPrototypeObj, classNameString, classNameGetterFunction,
        classNameSetterFunction);

    ESString* classListString = ESString::create("classList");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementPrototypeObj, classListString, classListGetterFunction,
        classListElementSetterFunction);

    ESString* attributesString = ESString::create("attributes");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementPrototypeObj, attributesString, attributesGetterFunction,
        nullptr);

#ifdef STARFISH_ENABLE_TEST
    ESString* innerHTMLString = ESString::create("innerHTML");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementPrototypeObj, innerHTMLString, innerHTMLGetterFunction,
        innerHTMLSetterFunction);
#endif

    ESString* clientTopString = ESString::create("clientTop");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementPrototypeObj, clientTopString, clientTopGetterFunction, nullptr);

    ESString* clientLeftString = ESString::create("clientLeft");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementPrototypeObj, clientLeftString, clientLeftGetterFunction,
        nullptr);

    ESString* clientWidthString = ESString::create("clientWidth");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementPrototypeObj, clientWidthString, clientWidthGetterFunction,
        nullptr);

    ESString* clientHeightString = ESString::create("clientHeight");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementPrototypeObj, clientHeightString, clientHeightGetterFunction,
        nullptr);

    ESString* childrenString = ESString::create("children");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementPrototypeObj, childrenString, childrenGetterFunction, nullptr);

    ESString* firstElementChildString = ESString::create("firstElementChild");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementPrototypeObj, firstElementChildString,
        firstElementChildGetterFunction, nullptr);

    ESString* lastElementChildString = ESString::create("lastElementChild");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementPrototypeObj, lastElementChildString,
        lastElementChildGetterFunction, nullptr);

    ESString* childElementCountString = ESString::create("childElementCount");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementPrototypeObj, childElementCountString,
        childElementCountGetterFunction, nullptr);

    ESString* previousElementSiblingString =
        ESString::create("previousElementSibling");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementPrototypeObj, previousElementSiblingString,
        previousElementSiblingGetterFunction, nullptr);

    ESString* nextElementSiblingString = ESString::create("nextElementSibling");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementPrototypeObj, nextElementSiblingString,
        nextElementSiblingGetterFunction, nullptr);

    // Bind for functions
    ESString* getAttributeString = ESString::create("getAttribute");
    ESFunctionObject* getAttributeESFn = ESFunctionObject::create(
        nullptr, getAttributeFunction, getAttributeString, 1, false);
    ElementPrototypeObj->defineDataProperty(getAttributeString, true, true,
                                            true, getAttributeESFn);

    ESString* setAttributeString = ESString::create("setAttribute");
    ESFunctionObject* setAttributeESFn = ESFunctionObject::create(
        nullptr, setAttributeFunction, setAttributeString, 2, false);
    ElementPrototypeObj->defineDataProperty(setAttributeString, true, true,
                                            true, setAttributeESFn);

    ESString* removeAttributeString = ESString::create("removeAttribute");
    ESFunctionObject* removeAttributeESFn = ESFunctionObject::create(
        nullptr, removeAttributeFunction, removeAttributeString, 1, false);
    ElementPrototypeObj->defineDataProperty(removeAttributeString, true, true,
                                            true, removeAttributeESFn);

    ESString* hasAttributeString = ESString::create("hasAttribute");
    ESFunctionObject* hasAttributeESFn = ESFunctionObject::create(
        nullptr, hasAttributeFunction, hasAttributeString, 1, false);
    ElementPrototypeObj->defineDataProperty(hasAttributeString, true, true,
                                            true, hasAttributeESFn);

    ESString* getElementsByTagNameString =
        ESString::create("getElementsByTagName");
    ESFunctionObject* getElementsByTagNameESFn =
        ESFunctionObject::create(nullptr, getElementsByTagNameFunction,
                                 getElementsByTagNameString, 1, false);
    ElementPrototypeObj->defineDataProperty(
        getElementsByTagNameString, true, true, true, getElementsByTagNameESFn);

    ESString* getElementsByClassNameString =
        ESString::create("getElementsByClassName");
    ESFunctionObject* getElementsByClassNameESFn =
        ESFunctionObject::create(nullptr, getElementsByClassNameFunction,
                                 getElementsByClassNameString, 1, false);
    ElementPrototypeObj->defineDataProperty(getElementsByClassNameString, true,
                                            true, true,
                                            getElementsByClassNameESFn);

    ESString* getClientRectsString = ESString::create("getClientRects");
    ESFunctionObject* getClientRectsESFn = ESFunctionObject::create(
        nullptr, getClientRectsFunction, getClientRectsString, 0, false);
    ElementPrototypeObj->defineDataProperty(getClientRectsString, true, true,
                                            true, getClientRectsESFn);

    ESString* getBoundingClientRectString =
        ESString::create("getBoundingClientRect");
    ESFunctionObject* getBoundingClientRectESFn =
        ESFunctionObject::create(nullptr, getBoundingClientRectFunction,
                                 getBoundingClientRectString, 0, false);
    ElementPrototypeObj->defineDataProperty(getBoundingClientRectString, true,
                                            true, true,
                                            getBoundingClientRectESFn);

    ESString* querySelectorString = ESString::create("querySelector");
    ESFunctionObject* querySelectorESFn = ESFunctionObject::create(
        nullptr, querySelectorFunction, querySelectorString, 1, false);
    ElementPrototypeObj->defineDataProperty(querySelectorString, true, true,
                                            true, querySelectorESFn);

    ESString* querySelectorAllString = ESString::create("querySelectorAll");
    ESFunctionObject* querySelectorAllESFn = ESFunctionObject::create(
        nullptr, querySelectorAllFunction, querySelectorAllString, 1, false);
    ElementPrototypeObj->defineDataProperty(querySelectorAllString, true, true,
                                            true, querySelectorAllESFn);

    ESString* removeString = ESString::create("remove");
    ESFunctionObject* removeESFn = ESFunctionObject::create(
        nullptr, removeFunction, removeString, 0, false);
    ElementPrototypeObj->defineDataProperty(removeString, true, true, true,
                                            removeESFn);

    return ElementFunction;
}
}
