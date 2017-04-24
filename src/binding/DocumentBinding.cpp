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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/Attr.h"
#include "dom/Comment.h"
#include "dom/Document.h"
#include "dom/DocumentFragment.h"
#include "dom/DocumentType.h"
#include "dom/DOMException.h"
#ifdef STARFISH_EXP
#include "dom/DOMImplementation.h"
#endif
#include "dom/Element.h"
#include "dom/HTMLBodyElement.h"
#include "dom/HTMLCollection.h"
#include "dom/HTMLElement.h"
#include "dom/HTMLHeadElement.h"
#include "dom/Node.h"
#include "dom/Text.h"
#include "extra/Location.h"
#include "platform/window/Window.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
#ifdef STARFISH_EXP
static ESValue implementationGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    DOMImplementation* v = originalObj->implementation();
    return v->scriptValue();
}
#endif

static ESValue URLGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    String* v = originalObj->urlString();
    return toJSString(v);
}

static ESValue documentURIGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    String* v = originalObj->urlString();
    return toJSString(v);
}

static ESValue compatModeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    String* v = originalObj->compatMode();
    return toJSString(v);
}

static ESValue characterSetGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    String* v = originalObj->characterSet();
    return toJSString(v);
}

static ESValue charsetGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    String* v = originalObj->characterSet();
    return toJSString(v);
}

static ESValue contentTypeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    String* v = originalObj->contentType();
    return toJSString(v);
}

static ESValue doctypeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    DocumentType* v = originalObj->doctype();
    if (v != nullptr) {
        return v->scriptValue();
    }
    return ESValue(ESValue::ESNull);
}

static ESValue documentElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    Element* v = originalObj->documentElement();
    if (v != nullptr) {
        return v->scriptValue();
    }
    return ESValue(ESValue::ESNull);
}

static ESValue locationGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    Location* v = originalObj->location();
    if (v != nullptr) {
        return v->scriptValue();
    }
    return ESValue(ESValue::ESNull);
}

extern ESValue locationDocumentSetterFunction(ESVMInstance* instance);

static ESValue bodyGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    HTMLElement* v = originalObj->body();
    if (v != nullptr) {
        return v->scriptValue();
    }
    return ESValue(ESValue::ESNull);
}

extern ESValue bodyDocumentSetterFunction(ESVMInstance* instance);

static ESValue headGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    HTMLHeadElement* v = originalObj->head();
    if (v != nullptr) {
        return v->scriptValue();
    }
    return ESValue(ESValue::ESNull);
}

extern ESValue defaultViewDocumentGetterFunction(ESVMInstance* instance);

static ESValue hiddenGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    bool v = originalObj->hidden();
    return ESValue(v);
}

static ESValue visibilityStateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    String* v = originalObj->visibilityState();
    return toJSString(v);
}

static ESValue getElementByIdFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, Node);
    Node* obj =
        (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

    if (obj->isDocument()) {
        Document* doc = obj->asDocument();
        ESValue argValue = instance->currentExecutionContext()->readArgument(0);

        if (argValue.isESString()) {
            ESString* argStr = argValue.asESString();

            if (*argStr == *(strings->emptyString.string())) {
                return ESValue(ESValue::ESNull);
            }

            Element* elem = doc->getElementById(toBrowserString(argStr));
            if (elem != nullptr) {
                return elem->scriptValue();
            }
        } else if (argValue.isNull()) {
            Element* elem =
                doc->getElementById(toBrowserString(ESString::create("null")));
            if (elem != nullptr) {
                return elem->scriptValue();
            }
        } else if (argValue.isUndefined()) {
            Element* elem = doc->getElementById(
                toBrowserString(ESString::create("undefined")));
            if (elem != nullptr) {
                return elem->scriptValue();
            }
        }
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

    if (obj->isDocument()) {
        if (instance->currentExecutionContext()->argumentCount() > 0) {
            Document* doc = obj->asDocument();
            ESValue argValue =
                instance->currentExecutionContext()->readArgument(0);
            if (!argValue.isESString()) {
                argValue = argValue.toString();
            }
            try {
                ESString* argStr = argValue.asESString();
                if (*argStr == *(strings->emptyString.string())) {
                    throw new DOMException(
                        doc->window()->scriptBindingInstance(),
                        DOMException::Code::DOM_EXCEPTION,
                        "Failed to execute 'querySelector' on "
                        "'Document': The provided selector is "
                        "empty.");
                }

                Element* elem = doc->querySelector(toBrowserString(argStr));
                if (elem != nullptr) {
                    return elem->scriptValue();
                }
            } catch (DOMException* e) {
                ESVMInstance::currentInstance()->throwError(e->scriptValue());
            }
        } else {
            THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                            "querySelector", "Document", "1", "0");
        }
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

    if (obj->isDocument()) {
        if (instance->currentExecutionContext()->argumentCount() > 0) {
            Document* doc = obj->asDocument();
            ESValue argValue =
                instance->currentExecutionContext()->readArgument(0);
            if (!argValue.isESString()) {
                argValue = argValue.toString();
            }
            try {
                ESString* argStr = argValue.asESString();
                if (*argStr == *(strings->emptyString.string())) {
                    throw new DOMException(
                        doc->window()->scriptBindingInstance(),
                        DOMException::Code::DOM_EXCEPTION,
                        "Failed to execute 'querySelectorAll' "
                        "on 'Document': The provided selector "
                        "is empty.");
                }

                NodeList* list = doc->querySelectorAll(toBrowserString(argStr));
                if (list != nullptr) {
                    return list->scriptValue();
                }
            } catch (DOMException* e) {
                ESVMInstance::currentInstance()->throwError(e->scriptValue());
            }
        } else {
            THROW_EXCEPTION(FAILED_TO_EXECUTE_BECAUSE_ARGS_NOT_ENOUGH,
                            "querySelectorAll", "Document", "1", "0")
        }
    } else {
        THROW_ILLEGAL_INVOCATION()
    }
    return ESValue(ESValue::ESNull);
}

static ESValue createDocumentFragmentFunction(ESVMInstance* instance)
{
    try {
        ESValue thisValue =
            instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, Node);
        Node* obj =
            (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

        if (obj->isDocument()) {
            Document* doc = obj->asDocument();
            return doc->createDocumentFragment()->scriptValue();
        } else {
            THROW_ILLEGAL_INVOCATION()
        }
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue createElementFunction(ESVMInstance* instance)
{
    try {
        ESValue thisValue =
            instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, Node);
        Node* obj =
            (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

        if (obj->isDocument()) {
            Document* doc = obj->asDocument();
            ESValue argValue =
                instance->currentExecutionContext()->readArgument(0);
            if (argValue.isUndefined()) {
                AtomicString name = AtomicString::createAttrAtomicString(
                    doc->window()->starFish(), "undefined");
                Element* elem = doc->createElement(name, true);
                if (elem != nullptr) {
                    return elem->scriptValue();
                }
            } else if (argValue.isNull()) {
                AtomicString name = AtomicString::createAttrAtomicString(
                    doc->window()->starFish(), "null");
                Element* elem = doc->createElement(name, true);
                if (elem != nullptr) {
                    return elem->scriptValue();
                }
            } else if (argValue.isESString()) {
                ESString* argStr = argValue.asESString();
                auto bStr = toBrowserString(argStr);
                if (!QualifiedName::checkNameProductionRule(bStr,
                                                            bStr->length())) {
                    throw new DOMException(
                        doc->window()->scriptBindingInstance(),
                        DOMException::Code::INVALID_CHARACTER_ERR, nullptr);
                }
                AtomicString name = AtomicString::createAttrAtomicString(
                    doc->window()->starFish(), argStr->utf8Data());
                Element* elem = doc->createElement(name, true);
                if (elem != nullptr) {
                    return elem->scriptValue();
                }
            }
        } else {
            THROW_ILLEGAL_INVOCATION()
        }
        return ESValue(ESValue::ESNull);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue createTextNodeFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, Node);
    Node* obj =
        (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

    if (obj->isDocument()) {
        Document* doc = obj->asDocument();
        ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        if (argValue.isESString()) {
            ESString* argStr = argValue.asESString();
            Text* elem = doc->createTextNode(toBrowserString(argStr));
            if (elem != nullptr) {
                return elem->scriptValue();
            }
        } else if (argValue.isNull()) {
            Text* elem =
                doc->createTextNode(toBrowserString(ESString::create("null")));
            if (elem != nullptr) {
                return elem->scriptValue();
            }
        } else if (argValue.isUndefined()) {
            Text* elem = doc->createTextNode(
                toBrowserString(ESString::create("undefined")));
            if (elem != nullptr) {
                return elem->scriptValue();
            }
        } else {
            THROW_ILLEGAL_INVOCATION();
        }

    } else {
        THROW_ILLEGAL_INVOCATION()
    }
    return ESValue(ESValue::ESNull);
}

static ESValue createCommentFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, Node);
    Node* obj =
        (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

    if (obj->isDocument()) {
        Document* doc = obj->asDocument();
        ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        Comment* elem = nullptr;
        if (argValue.isUndefined()) {
            elem = doc->createComment(String::fromUTF8("undefined"));
        } else if (argValue.isNull()) {
            elem = doc->createComment(String::fromUTF8("null"));
        } else if (argValue.isESString()) {
            ESString* argStr = argValue.asESString();
            elem = doc->createComment(toBrowserString(argStr));
        }
        if (elem != nullptr) {
            return elem->scriptValue();
        }
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

    if (obj->isDocument()) {
        Document* doc = obj->asDocument();
        ESValue argValue = instance->currentExecutionContext()->readArgument(0);
        if (argValue.isESString()) {
            ESString* argStr = argValue.asESString();
            HTMLCollection* result = doc->getElementsByTagName(
                doc->createAttributeName(toBrowserString(argStr)));
            if (result != nullptr) {
                return result->scriptValue();
            }
        }
    } else {
        THROW_ILLEGAL_INVOCATION()
    }
    return ESValue(ESValue::ESNull);
}

static ESValue getElementsByClassNameFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, Node);
    Node* obj =
        (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

    if (obj->isDocument()) {
        Document* doc = obj->asDocument();
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
                doc->getElementsByClassName(toBrowserString(argStr));
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
            HTMLCollection* result = doc->getElementsByClassName(listSoFar);
            if (result) {
                return result->scriptValue();
            }
        }
    } else {
        THROW_ILLEGAL_INVOCATION()
    }
    return ESValue(ESValue::ESNull);
}

static ESValue createAttributeFunction(ESVMInstance* instance)
{
    try {
        ESValue thisValue =
            instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, Node);
        Node* obj =
            (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

        if (obj->isDocument()) {
            Document* doc = obj->asDocument();
            ESValue argValue =
                instance->currentExecutionContext()->readArgument(0);
            if (argValue.isUndefined()) {
                Attr* result = doc->createAttribute(
                    QualifiedName(AtomicString::emptyAtomicString(),
                                  AtomicString::createAttrAtomicString(
                                      doc->window()->starFish(), "undefined")));
                if (result != nullptr) {
                    return result->scriptValue();
                }
            } else if (argValue.isNull()) {
                Attr* result = doc->createAttribute(
                    QualifiedName(AtomicString::emptyAtomicString(),
                                  AtomicString::createAttrAtomicString(
                                      doc->window()->starFish(), "null")));
                if (result != nullptr) {
                    return result->scriptValue();
                }
            } else if (argValue.isESString()) {
                ESString* argStr = argValue.asESString();
                Attr* result = doc->createAttribute(QualifiedName(
                    AtomicString::emptyAtomicString(),
                    AtomicString::createAttrAtomicString(
                        doc->window()->starFish(), argStr->utf8Data())));
                if (result != nullptr) {
                    return result->scriptValue();
                }
            }
        } else {
            THROW_ILLEGAL_INVOCATION()
        }
        return ESValue(ESValue::ESNull);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue childrenGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    return originalObj->children()->scriptValue();
}

static ESValue firstElementChildGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    return originalObj->firstElementChild()->scriptValue();
}

static ESValue lastElementChildGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    return originalObj->lastElementChild()->scriptValue();
}

static ESValue childElementCountGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    uint32_t v = originalObj->childElementCount();
    return ESValue(v);
}

static ESValue elementFromPointFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, Node);
    Node* obj =
        (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
    if (obj->isDocument()) {
        Document* doc = obj->asDocument();
        ESValue argValue0 =
            instance->currentExecutionContext()->readArgument(0);
        ESValue argValue1 =
            instance->currentExecutionContext()->readArgument(0);
        Element* element =
            doc->elementFromPoint(argValue0.toNumber(), argValue1.toNumber());
        if (element) {
            return element->scriptValue();
        }
    } else {
        THROW_ILLEGAL_INVOCATION()
    }
    return ESValue(ESValue::ESNull);
}

static ESValue onClickGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);

    return originalObj->onclickEventListener();
}

static ESValue onClickSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnclickEventListener(arg0);

    return ESValue();
}

static ESValue onMouseOverGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);

    return originalObj->onmouseoverEventListener();
}

static ESValue onMouseOverSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnmouseoverEventListener(arg0);

    return ESValue();
}

static ESValue onFocusGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);

    return originalObj->onfocusEventListener();
}

static ESValue onFocusSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnfocusEventListener(arg0);

    return ESValue();
}

static ESValue onKeyDownGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);

    return originalObj->onkeydownEventListener();
}

static ESValue onKeyDownSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnkeydownEventListener(arg0);

    return ESValue();
}

ESFunctionObject* bindingDocument(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        Document, fetchData(scriptBindingInstance)->fnNode());

    /* 4.5 Interface Document */

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("head"), headGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("body"), bodyGetterFunction,
        bodyDocumentSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("documentElement"), documentElementGetterFunction,
        nullptr);

#ifdef STARFISH_EXP
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("implementation"), implementationGetterFunction,
        nullptr);
#endif

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("characterSet"), characterSetGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("charset"), charsetGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("contentType"), contentTypeGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("compatMode"), compatModeGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("documentURI"), documentURIGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("URL"), URLGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("firstElementChild"), firstElementChildGetterFunction,
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("lastElementChild"), lastElementChildGetterFunction,
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("childElementCount"), childElementCountGetterFunction,
        nullptr);

    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("getElementById"), false, false, false,
            ESFunctionObject::create(NULL, getElementByIdFunction,
                                     ESString::create("getElementById"), 1,
                                     false));

    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("querySelector"), true, true, true,
            ESFunctionObject::create(NULL, querySelectorFunction,
                                     ESString::create("querySelector"), 1,
                                     false));

    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("querySelectorAll"), true, true, true,
            ESFunctionObject::create(NULL, querySelectorAllFunction,
                                     ESString::create("querySelectorAll"), 1,
                                     false));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("doctype"), doctypeGetterFunction, nullptr);

    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("createDocumentFragment"), false, false, false,
            ESFunctionObject::create(NULL, createDocumentFragmentFunction,
                                     ESString::create("DocumentFragment"), 1,
                                     false));

    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("createElement"), false, false, false,
            ESFunctionObject::create(NULL, createElementFunction,
                                     ESString::create("createElement"), 1,
                                     false));

    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("createTextNode"), false, false, false,
            ESFunctionObject::create(NULL, createTextNodeFunction,
                                     ESString::create("createTextNode"), 1,
                                     false));

    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("createComment"), false, false, false,
            ESFunctionObject::create(NULL, createCommentFunction,
                                     ESString::create("createComment"), 1,
                                     false));

    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("getElementsByTagName"), false, false, false,
            ESFunctionObject::create(NULL, getElementsByTagNameFunction,
                                     ESString::create("getElementsByTagName"),
                                     1, false));

    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("getElementsByClassName"), false, false, false,
            ESFunctionObject::create(NULL, getElementsByClassNameFunction,
                                     ESString::create("getElementsByClassName"),
                                     1, false));

    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("createAttribute"), false, false, false,
            ESFunctionObject::create(NULL, createAttributeFunction,
                                     ESString::create("createAttribute"), 1,
                                     false));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("children"), childrenGetterFunction, nullptr);

    /* Page Visibility */
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("hidden"), hiddenGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("visibilityState"), visibilityStateGetterFunction,
        nullptr);

    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("elementFromPoint"), false, false, false,
            ESFunctionObject::create(NULL, elementFromPointFunction,
                                     ESString::create("elementFromPoint"), 2,
                                     false));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onclick"), onClickGetterFunction,
        onClickSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onmouseover"), onMouseOverGetterFunction,
        onMouseOverSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onfocus"), onFocusGetterFunction,
        onFocusSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onkeydown"), onKeyDownGetterFunction,
        onKeyDownSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("defaultView"), defaultViewDocumentGetterFunction,
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("location"), locationGetterFunction,
        locationDocumentSetterFunction);

    return DocumentFunction;
}
}
