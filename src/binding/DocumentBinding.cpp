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

static ESValue onclickGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);

    return originalObj->onclick();
}

static ESValue onclickSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnclick(arg0);

    return ESValue();
}

static ESValue onmouseoverGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);

    return originalObj->onmouseover();
}

static ESValue onmouseoverSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnmouseover(arg0);

    return ESValue();
}

static ESValue onfocusGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);

    return originalObj->onfocus();
}

static ESValue onfocusSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnfocus(arg0);

    return ESValue();
}

static ESValue onkeydownGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);

    return originalObj->onkeydown();
}

static ESValue onkeydownSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);

    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);

    originalObj->setOnkeydown(arg0);

    return ESValue();
}

// Implement for functions
static ESValue getElementsByTagNameFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        auto msg = ESString::create("Not enough arguments");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Declare return value (empty when void)
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
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        auto msg = ESString::create("Not enough arguments");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Declare return value (empty when void)
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

static ESValue createElementFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        auto msg = ESString::create("Not enough arguments");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Declare return value (empty when void)
    Element* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    try {
        result = originalObj->createElement(value0);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue createDocumentFragmentFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    // Declare return value (empty when void)
    DocumentFragment* result = nullptr;
    // Call native function (nargs: 0)
    try {
        result = originalObj->createDocumentFragment();
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue createTextNodeFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        auto msg = ESString::create("Not enough arguments");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Declare return value (empty when void)
    Text* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    result = originalObj->createTextNode(value0);

    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue createCommentFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        auto msg = ESString::create("Not enough arguments");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Declare return value (empty when void)
    Comment* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    result = originalObj->createComment(value0);

    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue createAttributeFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        auto msg = ESString::create("Not enough arguments");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Declare return value (empty when void)
    Attr* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    try {
        result = originalObj->createAttribute(value0);
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue elementFromPointFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 2) {
        auto msg = ESString::create("Not enough arguments");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Declare return value (empty when void)
    Element* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
    // Handle argument arg0
    double value0;
    value0 = arg0.toNumber();

    // Handle argument arg1
    double value1;
    value1 = arg1.toNumber();

    // Call native function (nargs: 2)
    result = originalObj->elementFromPoint(value0, value1);

    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue getElementByIdFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Document);
    size_t argCount = instance->currentExecutionContext()->argumentCount();
    if (argCount < 1) {
        auto msg = ESString::create("Not enough arguments");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    // Declare return value (empty when void)
    Element* result = nullptr;
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    String* value0 = String::emptyString;
    value0 = toBrowserString(arg0);

    // Call native function (nargs: 1)
    result = originalObj->getElementById(value0);

    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

// TODO Move throw DOM exception code into querySelector()
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

// TODO Move throw DOM exception code into querySelectorAll()
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
        ESString::create("onclick"), onclickGetterFunction,
        onclickSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onmouseover"), onmouseoverGetterFunction,
        onmouseoverSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onfocus"), onfocusGetterFunction,
        onfocusSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onkeydown"), onkeydownGetterFunction,
        onkeydownSetterFunction);

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
