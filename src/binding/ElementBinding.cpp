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

#include "Binding.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

namespace StarFish {

using namespace escargot;

static ESValue firstElementChildGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    return originalObj->firstElementChild()
               ? originalObj->firstElementChild()->scriptValue()
               : ESValue(ESValue::ESNull);
}

static ESValue lastElementChildGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    return originalObj->lastElementChild()
               ? originalObj->lastElementChild()->scriptValue()
               : ESValue(ESValue::ESNull);
}

ESValue nextElementSiblingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    return originalObj->nextElementSibling()
               ? originalObj->nextElementSibling()->scriptValue()
               : ESValue(ESValue::ESNull);
}

ESValue previousElementSiblingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    return originalObj->previousElementSibling()
               ? originalObj->previousElementSibling()->scriptValue()
               : ESValue(ESValue::ESNull);
}

static ESValue childElementCountGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    return ESValue(originalObj->childElementCount());
}

static ESValue clientLeftGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    if (originalObj->isElement()) {
        return ESValue(originalObj->asElement()->clientLeft());
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue clientTopGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    if (originalObj->isElement()) {
        return ESValue(originalObj->asElement()->clientTop());
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue clientWidthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    if (originalObj->isElement()) {
        return ESValue(originalObj->asElement()->clientWidth());
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue clientHeightGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    if (originalObj->isElement()) {
        return ESValue(originalObj->asElement()->clientHeight());
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue offsetParentGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    if (originalObj->isElement()) {
        return originalObj->asElement()->offsetParent()
                   ? originalObj->asElement()->offsetParent()->scriptValue()
                   : ESValue(ESValue::ESNull);
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

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

static ESValue localNameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement()) {
        return toJSString(nd->asElement()->localName());
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue tagNameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement()) {
        return toJSString(nd->asElement()->tagName());
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue namespaceURIGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement()) {
        if (nd->asElement()->name().namespaceURIAtomic() ==
            AtomicString::emptyAtomicString()) {
            return ESValue(ESValue::ESNull);
        }
        return toJSString(nd->asElement()->name().namespaceURI());
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue idGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement()) {
        return toJSString(nd->asElement()->getAttribute(
            nd->document()->window()->starFish()->staticStrings()->m_id));
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue idSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement()) {
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        nd->asElement()->setAttribute(
            nd->document()->window()->starFish()->staticStrings()->m_id,
            toBrowserString(v));
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
    return ESValue();
}

static ESValue classNameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement()) {
        return toJSString(nd->asElement()->getAttribute(
            nd->document()->window()->starFish()->staticStrings()->m_class));
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue classNameSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement()) {
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        nd->asElement()->setAttribute(
            nd->document()->window()->starFish()->staticStrings()->m_class,
            toBrowserString(v));
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
    return ESValue();
}

static ESValue childrenGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    HTMLCollection* nd = originalObj->children();
    return nd->scriptValue();
}

static ESValue classListGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    DOMTokenList* nd = originalObj->classList();
    if (nd == nullptr) {
        return ESValue(ESValue::ESUndefined);
    }
    return nd->scriptValue();
}

static ESValue innerHTMLGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement()) {
        return toJSString(nd->asElement()->innerHTML());
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue innerHTMLSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement()) {
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        nd->asElement()->setInnerHTML(toBrowserString(v));
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
    return ESValue();
}

static ESValue attributesGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    NamedNodeMap* nd = originalObj->attributes();
    if (nd == nullptr) {
        return ESValue(ESValue::ESUndefined);
    }
    return nd->scriptValue();
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

        auto sf =
            ((Window*)instance->globalObject()->extraPointerData())->starFish();
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
            auto sf = ((Window*)instance->globalObject()->extraPointerData())
                          ->starFish();
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
            auto sf = ((Window*)instance->globalObject()->extraPointerData())
                          ->starFish();
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

static ESValue styleGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    if (!originalObj->isElement()) {
        return ESValue(ESValue::ESNull);
    }
    CSSStyleDeclaration* s = originalObj->asElement()->inlineStyle();
    return s->scriptValue();
}

static ESValue styleSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement()) {
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        nd->asElement()->setAttribute(
            nd->document()->window()->starFish()->staticStrings()->m_style,
            toBrowserString(v));
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
    return ESValue();
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
            auto sf = ((Window*)ESVMInstance::currentInstance()
                           ->globalObject()
                           ->extraPointerData())
                          ->starFish();
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
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        Element, fetchData(scriptBindingInstance)->fnNode());

    /* 4.8 Interface Element */
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("firstElementChild"), firstElementChildGetterFunction,
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("lastElementChild"), lastElementChildGetterFunction,
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("nextElementSibling"),
        nextElementSiblingGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("previousElementSibling"),
        previousElementSiblingGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("childElementCount"), childElementCountGetterFunction,
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("clientLeft"), clientLeftGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("clientTop"), clientTopGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("clientWidth"), clientWidthGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("clientHeight"), clientHeightGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("offsetParent"), offsetParentGetterFunction, nullptr);

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

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("localName"), localNameGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("tagName"), tagNameGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("namespaceURI"), namespaceURIGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("id"), idGetterFunction, idSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("className"), classNameGetterFunction,
        classNameSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("children"), childrenGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("classList"), classListGetterFunction, nullptr);

#ifdef STARFISH_ENABLE_TEST
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("innerHTML"), innerHTMLGetterFunction,
        innerHTMLSetterFunction);
#endif

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("attributes"), attributesGetterFunction, nullptr);

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

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        ElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("style"), styleGetterFunction, styleSetterFunction);

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
