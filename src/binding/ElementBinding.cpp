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

#include "StarFish.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/Document.h"
#include "dom/DOMException.h"
#include "dom/DOMRect.h"
#include "dom/DOMRectList.h"
#include "dom/Element.h"
#include "dom/HTMLCollection.h"
#include "dom/NamedNodeMap.h"
#include "platform/window/Window.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
extern ESValue namespaceURIElementGetterFunction(ESVMInstance* instance);

static ESValue localNameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->localName();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue tagNameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->tagName();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue idGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare return value (empty when void)
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
    // Declare return value (empty when void)
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

extern ESValue classListElementGetterFunction(ESVMInstance* instance);

static ESValue attributesGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare return value (empty when void)
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
    // Declare return value (empty when void)
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
    if (!arg0.isUndefinedOrNull()) {
        value0 = toBrowserString(arg0);
    }
    originalObj->setInnerHTML(value0);
    return ESValue();
}
#endif

static ESValue clientTopGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare return value (empty when void)
    int32_t result;
    result = originalObj->clientTop();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue clientLeftGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare return value (empty when void)
    int32_t result;
    result = originalObj->clientLeft();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue clientWidthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare return value (empty when void)
    int32_t result;
    result = originalObj->clientWidth();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue clientHeightGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare return value (empty when void)
    int32_t result;
    result = originalObj->clientHeight();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue childrenGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare return value (empty when void)
    HTMLCollection* result = nullptr;
    result = originalObj->children();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue firstElementChildGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare return value (empty when void)
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
    // Declare return value (empty when void)
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
    // Declare return value (empty when void)
    uint32_t result;
    result = originalObj->childElementCount();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue previousElementSiblingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Element);
    // Declare return value (empty when void)
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
    // Declare return value (empty when void)
    Element* result = nullptr;
    result = originalObj->nextElementSibling();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

extern ESValue styleElementGetterFunction(ESVMInstance* instance);

extern ESValue styleElementSetterFunction(ESVMInstance* instance);

static ESValue getClientRectsGetterFunction(ESVMInstance* instance)
{
    ESValue nd = instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(nd, Node);

    Element* elem = ((Node*)nd.asESPointer()->asESObject()->extraPointerData())
                        ->asElement();
    DOMRectList* rectList = elem->getClientRects();

    return rectList->scriptValue();
}

static ESValue getBoundingClientRectGetterFunction(ESVMInstance* instance)
{
    ESValue nd = instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(nd, Node);

    Element* elem = ((Node*)nd.asESPointer()->asESObject()->extraPointerData())
                        ->asElement();
    DOMRect* rect = elem->getBoundingClientRect();

    return rect->scriptValue();
}

ESValue removeFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, Node);
    Node* obj =
        (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
    Node* p = obj->parentNode();
    if (p == nullptr) {
        return ESValue(ESValue::ESUndefined);
    }
    obj->remove();
    return ESValue(ESValue::ESUndefined);
}

static ESValue getAttributeFunction(ESVMInstance* instance)
{
    try {
        ESValue thisValue =
            instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, Node);

        StarFish* sf = fetchStarFish(instance);
        ESValue argValue = instance->currentExecutionContext()->readArgument(0);

        if (argValue.isESString()) {
            String* keyStr = toBrowserString(argValue);
            Element* elem = ((Node*)thisValue.asESPointer()
                                 ->asESObject()
                                 ->extraPointerData())
                                ->asElement();
            size_t idx = elem->hasAttribute(
                elem->document()->createAttributeName(keyStr));
            if (idx == SIZE_MAX) {
                return ESValue(ESValue::ESNull);
            } else {
                return toJSString(elem->getAttribute(idx));
            }
        }
        return ESValue(ESValue::ESNull);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue setAttributeFunction(ESVMInstance* instance)
{
    try {
        ESValue nd = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(nd, Node);

        ESValue key = instance->currentExecutionContext()->readArgument(0);
        ESValue val = instance->currentExecutionContext()->readArgument(1);

        if (key.isESString()) {
            StarFish* sf = fetchStarFish(instance);
            // Validate key string
            String* keyStr = toBrowserString(key);
            if (!QualifiedName::checkNameProductionRule(keyStr,
                                                        keyStr->length())) {
                throw new DOMException(
                    sf->window()->scriptBindingInstance(),
                    DOMException::Code::INVALID_CHARACTER_ERR, nullptr);
            }

            String* attrVal = toBrowserString(val);
            Element* elem =
                ((Node*)nd.asESPointer()->asESObject()->extraPointerData())
                    ->asElement();
            elem->setAttribute(elem->document()->createAttributeName(keyStr),
                               attrVal);
        }
        return ESValue(ESValue::ESNull);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue removeAttributeFunction(ESVMInstance* instance)
{
    try {
        ESValue nd = instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(nd, Node);

        ESValue key = instance->currentExecutionContext()->readArgument(0);

        if (key.isESString()) {
            StarFish* sf = fetchStarFish(instance);
            Element* elem =
                ((Node*)nd.asESPointer()->asESObject()->extraPointerData())
                    ->asElement();
            String* keyStr = toBrowserString(key);
            elem->removeAttribute(
                elem->document()->createAttributeName(keyStr));
        }
        return ESValue();
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue getElementsByClassNameFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, Node);
    Node* obj =
        (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

    if (obj->isElement()) {
        Element* elem = obj->asElement();
        ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        if (argValue.isESString() || argValue.isUndefinedOrNull()) {
            ESString* argStr;
            if (argValue.isNull()) {
                argStr = ESString::create("null");
            } else if (argValue.isUndefined()) {
                argStr = ESString::create("undefined");
            } else {
                argStr = argValue.asESString();
            }
            HTMLCollection* result =
                elem->getElementsByClassName(toBrowserString(argStr));
            if (result != nullptr) {
                return result->scriptValue();
            }
        } else if (argValue.isObject() &&
                   argValue.asESPointer()->isESArrayObject()) {
            ESArrayObject* array = argValue.asESPointer()->asESArrayObject();
            String* listSoFar = String::createASCIIString("");
            for (unsigned i = 0; i < array->length(); i++) {
                ESValue val = array->get(i);
                if (val.isESString()) {
                    listSoFar =
                        listSoFar->concat(toBrowserString(val.asESString()));
                    if (i < array->length() - 1) {
                        listSoFar =
                            listSoFar->concat(String::createASCIIString(","));
                    }
                } else {
                    return ESValue(ESValue::ESNull);
                }
            }
            HTMLCollection* result = elem->getElementsByClassName(listSoFar);
            if (result) {
                return result->scriptValue();
            }
        }
#ifdef STARFISH_TC_COVERAGE
        STARFISH_LOG_INFO("Element&&&getElementsByClassName\n");
#endif
    } else {
        THROW_ILLEGAL_INVOCATION()
    }
    return ESValue(ESValue::ESNull);
}

static ESValue getElementsByTagNameFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, Node);
    Node* obj =
        (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

    if (obj->isElement()) {
        Element* elem = obj->asElement();
        ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        if (argValue.isESString()) {
            ESString* argStr = argValue.asESString();
            HTMLCollection* result = elem->getElementsByTagName(
                elem->document()->createAttributeName(toBrowserString(argStr)));
            if (result) {
                return result->scriptValue();
            }
        }
#ifdef STARFISH_TC_COVERAGE
        STARFISH_LOG_INFO("Element&&&getElementsByTagName\n");
#endif
    } else {
        THROW_ILLEGAL_INVOCATION()
    }
    return ESValue(ESValue::ESNull);
}

static ESValue querySelectorFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, Node);
    Node* obj =
        (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

    if (obj->isElement()) {
        if (instance->currentExecutionContext()->argumentCount() > 0) {
            Element* elem = obj->asElement();
            ESValue argValue =
                instance->currentExecutionContext()->readArgument(0);
            if (!argValue.isESString()) {
                argValue = argValue.toString();
            }
            try {
                ESString* argStr = argValue.asESString();
                if (*argStr == *(strings->emptyString.string())) {
                    throw new DOMException(
                        elem->document()->window()->scriptBindingInstance(),
                        DOMException::Code::DOM_EXCEPTION,
                        "Failed to execute 'querySelector' "
                        "on 'Element': The provided "
                        "selector is empty.");
                }

                Element* result = elem->querySelector(toBrowserString(argStr));
                if (result != nullptr) {
                    return result->scriptValue();
                }
            } catch (DOMException* e) {
                ESVMInstance::currentInstance()->throwError(e->scriptValue());
            }
        } else {
            auto msg = ESString::create(
                "Failed to execute 'querySelector' on "
                "'Element': 1 argument required, but only 0 "
                "present.");
            instance->throwError(ESValue(TypeError::create(msg)));
        }
#ifdef STARFISH_TC_COVERAGE
        STARFISH_LOG_INFO("Element&&&querySelector\n");
#endif
    } else {
        THROW_ILLEGAL_INVOCATION()
    }
    return ESValue(ESValue::ESNull);
}

static ESValue querySelectorAllFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, Node);
    Node* obj =
        (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

    if (obj->isElement()) {
        if (instance->currentExecutionContext()->argumentCount() > 0) {
            Element* elem = obj->asElement();
            ESValue argValue =
                instance->currentExecutionContext()->readArgument(0);
            if (!argValue.isESString()) {
                argValue = argValue.toString();
            }
            try {
                ESString* argStr = argValue.asESString();
                if (*argStr == *(strings->emptyString.string())) {
                    throw new DOMException(
                        elem->document()->window()->scriptBindingInstance(),
                        DOMException::Code::DOM_EXCEPTION,
                        "Failed to execute "
                        "'querySelectorAll' on 'Element': "
                        "The provided selector is empty.");
                }

                NodeList* list =
                    elem->querySelectorAll(toBrowserString(argStr));
                if (list != nullptr) {
                    return list->scriptValue();
                }
            } catch (DOMException* e) {
                ESVMInstance::currentInstance()->throwError(e->scriptValue());
            }
        } else {
            auto msg = ESString::create(
                "Failed to execute 'querySelectorAll' on "
                "'Element': 1 argument required, but only 0 "
                "present.");
            instance->throwError(ESValue(TypeError::create(msg)));
        }
#ifdef STARFISH_TC_COVERAGE
        STARFISH_LOG_INFO("Element&&&querySelectorAll\n");
#endif
    } else {
        THROW_ILLEGAL_INVOCATION()
    }
    return ESValue(ESValue::ESNull);
}

static ESValue hasAttributeFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, Node);

    int count = instance->currentExecutionContext()->argumentCount();
    if (count == 1) {
        ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        if (argValue.isESString()) {
            StarFish* sf = fetchStarFish(instance);
            QualifiedName name =
                QualifiedName(AtomicString::emptyAtomicString(),
                              AtomicString::createAttrAtomicString(
                                  sf, argValue.asESString()->utf8Data()));
            size_t res = ((Element*)thisValue.asESPointer()
                              ->asESObject()
                              ->extraPointerData())
                             ->hasAttribute(name);
            return res != SIZE_MAX ? ESValue(true) : ESValue(false);
        } else {
            return ESValue(false);
        }
    } else {
        auto msg = ESString::create(
            "Failed to execute 'hasAttribute' on Element: 1 "
            "argument required, but only 0 present.");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

ESFunctionObject* bindingElement(ScriptBindingInstance* scriptBindingInstance)
{
    // Bind for constructor
    ESString* ElementString = ESString::create("Element");
    ESFunctionObject* ElementFunction = ESFunctionObject::create(
        nullptr, errorOnConstructorFunction, ElementString, 1, true, true);
    ElementFunction->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    ElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    ElementFunction->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)->fnNode()->protoType());
    ElementFunction->set__proto__(fetchData(scriptBindingInstance)->fnNode());
    ESObject* ElementObj =
        ElementFunction->protoType().asESPointer()->asESObject();

    // Bind for attributes
    ESString* namespaceURIString = ESString::create("namespaceURI");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        namespaceURIString, namespaceURIElementGetterFunction, nullptr);

    ESString* localNameString = ESString::create("localName");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        localNameString, localNameGetterFunction, nullptr);

    ESString* tagNameString = ESString::create("tagName");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), tagNameString,
        tagNameGetterFunction, nullptr);

    ESString* idString = ESString::create("id");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), idString,
        idGetterFunction, idSetterFunction);

    ESString* classNameString = ESString::create("className");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        classNameString, classNameGetterFunction, classNameSetterFunction);

    ESString* classListString = ESString::create("classList");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        classListString, classListElementGetterFunction, nullptr);

    ESString* attributesString = ESString::create("attributes");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        attributesString, attributesGetterFunction, nullptr);

    ESString* innerHTMLString = ESString::create("innerHTML");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        innerHTMLString, innerHTMLGetterFunction, innerHTMLSetterFunction);

    ESString* clientTopString = ESString::create("clientTop");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        clientTopString, clientTopGetterFunction, nullptr);

    ESString* clientLeftString = ESString::create("clientLeft");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        clientLeftString, clientLeftGetterFunction, nullptr);

    ESString* clientWidthString = ESString::create("clientWidth");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        clientWidthString, clientWidthGetterFunction, nullptr);

    ESString* clientHeightString = ESString::create("clientHeight");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        clientHeightString, clientHeightGetterFunction, nullptr);

    ESString* childrenString = ESString::create("children");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        childrenString, childrenGetterFunction, nullptr);

    ESString* firstElementChildString = ESString::create("firstElementChild");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        firstElementChildString, firstElementChildGetterFunction, nullptr);

    ESString* lastElementChildString = ESString::create("lastElementChild");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        lastElementChildString, lastElementChildGetterFunction, nullptr);

    ESString* childElementCountString = ESString::create("childElementCount");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        childElementCountString, childElementCountGetterFunction, nullptr);

    ESString* previousElementSiblingString =
        ESString::create("previousElementSibling");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        previousElementSiblingString, previousElementSiblingGetterFunction,
        nullptr);

    ESString* nextElementSiblingString = ESString::create("nextElementSibling");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        nextElementSiblingString, nextElementSiblingGetterFunction, nullptr);

    ESString* styleString = ESString::create("style");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(), styleString,
        styleElementGetterFunction, styleElementSetterFunction);

    ElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("getClientRects"), true, true, true,
            ESFunctionObject::create(NULL, getClientRectsGetterFunction,
                                     ESString::create("getClientRects"), 0,
                                     false));

    ElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("getBoundingClientRect"), true, true, true,
            ESFunctionObject::create(NULL, getBoundingClientRectGetterFunction,
                                     ESString::create("getBoundingClientRect"),
                                     0, false));

    ElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("remove"), false, false, false,
            ESFunctionObject::create(NULL, removeFunction,
                                     ESString::create("remove"), 0, false));

    ElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("getAttribute"), false, false, false,
            ESFunctionObject::create(NULL, getAttributeFunction,
                                     ESString::create("getAttribute"), 0,
                                     false));

    ElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("setAttribute"), false, false, false,
            ESFunctionObject::create(NULL, setAttributeFunction,
                                     ESString::create("setAttribute"), 0,
                                     false));

    // spec of removeAttribute
    // https://www.w3.org/TR/DOM-Level-2-Core/core.html#ID-6D6AC0F9
    ElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("removeAttribute"), false, false, false,
            ESFunctionObject::create(NULL, removeAttributeFunction,
                                     ESString::create("removeAttribute"), 0,
                                     false));

    ElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("getElementsByClassName"), false, false, false,
            ESFunctionObject::create(NULL, getElementsByClassNameFunction,
                                     ESString::create("getElementsByClassName"),
                                     1, false));

    ElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("getElementsByTagName"), false, false, false,
            ESFunctionObject::create(NULL, getElementsByTagNameFunction,
                                     ESString::create("getElementsByTagName"),
                                     1, false));

    ElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("querySelector"), true, true, true,
            ESFunctionObject::create(NULL, querySelectorFunction,
                                     ESString::create("querySelector"), 1,
                                     false));

    ElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("querySelectorAll"), true, true, true,
            ESFunctionObject::create(NULL, querySelectorAllFunction,
                                     ESString::create("querySelectorAll"), 1,
                                     false));

    ElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("hasAttribute"), false, false, false,
            ESFunctionObject::create(NULL, hasAttributeFunction,
                                     ESString::create("hasAttribute"), 1,
                                     false));

    return ElementFunction;
}
}
