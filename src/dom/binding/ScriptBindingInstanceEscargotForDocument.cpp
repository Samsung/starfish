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

ESFunctionObject* bindingDocument(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        Document, fetchData(scriptBindingInstance)->node());

    /* 4.5 Interface Document */

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("head"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                Document* document = nd->asDocument();
                HTMLHeadElement* head = document->headElement();
                if (head) {
                    return head->scriptValue();
                }
            }
            return ESValue(ESValue::ESNull);
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("body"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                Document* document = nd->asDocument();
                HTMLBodyElement* body = document->bodyElement();
                if (body) {
                    return body->scriptValue();
                }
            }
            return ESValue(ESValue::ESNull);
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0),
                         ScriptWrappable::Type::NodeObject);
            if (!v.isUndefinedOrNull()) {
                Node* nd = originalObj;
                Node* node_v = (Node*)instance->currentExecutionContext()
                                   ->readArgument(0)
                                   .asESPointer()
                                   ->asESObject()
                                   ->extraPointerData();
                if (nd->isDocument()) {
                    if (node_v->isElement() &&
                        node_v->asElement()->isHTMLElement() &&
                        node_v->asElement()
                            ->asHTMLElement()
                            ->isHTMLBodyElement()) {
                        HTMLBodyElement* body = nd->asDocument()->bodyElement();
                        HTMLHtmlElement* html_root =
                            nd->asDocument()->rootElement();
                        if (body) {
                            html_root->removeChild(body);
                        }
                        html_root->appendChild(node_v);
                    } else {
                        THROW_DOM_EXCEPTION(
                            instance, DOMException::HIERARCHY_REQUEST_ERR);
                    }
                    return ESValue();
                }
            }
            THROW_ILLEGAL_INVOCATION();
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("documentElement"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                Document* document = nd->asDocument();
                Element* docElem = document->documentElement();
                if (docElem) {
                    return docElem->scriptValue();
                }
            }
            return ESValue(ESValue::ESNull);
        },
        nullptr);

#ifdef STARFISH_EXP
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("implementation"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                Document* document = nd->asDocument();
                DOMImplementation* impl = document->domImplementation();
                if (impl) {
                    return impl->scriptValue();
                }
            }
            return ESValue(ESValue::ESNull);
        },
        nullptr);
#endif

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("characterSet"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                return toJSString(nd->asDocument()->charset());
            }
            THROW_ILLEGAL_INVOCATION();
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("charset"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                return toJSString(nd->asDocument()->charset());
            }
            THROW_ILLEGAL_INVOCATION();
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("contentType"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                return toJSString(nd->asDocument()->contentType());
            }
            THROW_ILLEGAL_INVOCATION();
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("compatMode"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                return toJSString(nd->asDocument()->compatMode());
            }
            THROW_ILLEGAL_INVOCATION();
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("documentURI"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                return nd->asDocument()->documentURI()
                           ? toJSString(
                                 nd->asDocument()->documentURI()->urlString())
                           : ScriptValueNull;
            }
            THROW_ILLEGAL_INVOCATION();
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("URL"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                return nd->asDocument()->documentURI()
                           ? toJSString(
                                 nd->asDocument()->documentURI()->urlString())
                           : ScriptValueNull;
            }
            THROW_ILLEGAL_INVOCATION();
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("firstElementChild"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            return originalObj->firstElementChild()
                       ? originalObj->firstElementChild()->scriptValue()
                       : ESValue(ESValue::ESNull);
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("lastElementChild"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            return originalObj->lastElementChild()
                       ? originalObj->lastElementChild()->scriptValue()
                       : ESValue(ESValue::ESNull);
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("childElementCount"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            return ESValue(originalObj->childElementCount());
        },
        nullptr);

    ESFunctionObject* getElementByIdFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            ESValue thisValue =
                instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()
                            ->asESObject()
                            ->extraPointerData();

            if (obj->isDocument()) {
                Document* doc = obj->asDocument();
                ESValue argValue =
                    instance->currentExecutionContext()->readArgument(0);

                if (argValue.isESString()) {
                    ESString* argStr = argValue.asESString();

                    if (*argStr == *(strings->emptyString.string())) {
                        return ESValue(ESValue::ESNull);
                    }

                    Element* elem =
                        doc->getElementById(toBrowserString(argStr));
                    if (elem != nullptr) {
                        return elem->scriptValue();
                    }
                } else if (argValue.isNull()) {
                    Element* elem = doc->getElementById(
                        toBrowserString(ESString::create("null")));
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
        },
        ESString::create("getElementById"), 1, false);
    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("getElementById"), false, false,
                             false, getElementByIdFunction);

    ESFunctionObject* querySelectorFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            ESValue thisValue =
                instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()
                            ->asESObject()
                            ->extraPointerData();

            if (obj->isDocument()) {
                if (instance->currentExecutionContext()->argumentCount() > 0) {
                    Document* doc = obj->asDocument();
                    ESValue argValue =
                        instance->currentExecutionContext()->readArgument(0);
                    if (argValue.isESString()) {
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

                            Element* elem =
                                doc->querySelector(toBrowserString(argStr));
                            if (elem != nullptr) {
                                return elem->scriptValue();
                            }
                        } catch (DOMException* e) {
                            ESVMInstance::currentInstance()->throwError(
                                e->scriptValue());
                        }
                    } else if (argValue.isNull() || argValue.isUndefined()) {
                        return ESValue(ESValue::ESNull);
                    }
                } else {
                    auto msg = ESString::create(
                        "Failed to execute 'querySelector' on 'Document': "
                        "1 argument required, but only 0 present.");
                    instance->throwError(ESValue(TypeError::create(msg)));
                }
            } else {
                THROW_ILLEGAL_INVOCATION()
            }
            return ESValue(ESValue::ESNull);
        },
        ESString::create("querySelector"), 1, false);
    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("querySelector"), false, false,
                             false, querySelectorFunction);

#ifdef STARFISH_ENABLE_WASU
    ESFunctionObject* querySelectorAllFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            ESValue thisValue =
                instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()
                            ->asESObject()
                            ->extraPointerData();

            if (obj->isDocument()) {
                if (instance->currentExecutionContext()->argumentCount() > 0) {
                    Document* doc = obj->asDocument();
                    ESValue argValue =
                        instance->currentExecutionContext()->readArgument(0);
                    if (argValue.isESString()) {
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

                            NodeList* list =
                                doc->querySelectorAll(toBrowserString(argStr));
                            if (list != nullptr) {
                                return list->scriptValue();
                            }
                        } catch (DOMException* e) {
                            ESVMInstance::currentInstance()->throwError(
                                e->scriptValue());
                        }
                    } else if (argValue.isNull() || argValue.isUndefined()) {
                        return ESValue(ESValue::ESNull);
                    }
                } else {
                    auto msg = ESString::create(
                        "Failed to execute 'querySelectorAll' on "
                        "'Document': 1 argument required, but only 0 "
                        "present.");
                    instance->throwError(ESValue(TypeError::create(msg)));
                }
            } else {
                THROW_ILLEGAL_INVOCATION()
            }
            return ESValue(ESValue::ESNull);
        },
        ESString::create("querySelectorAll"), 1, false);
    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("querySelectorAll"), false, false,
                             false, querySelectorAllFunction);
#endif
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("doctype"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                DocumentType* docType = nd->asDocument()->docType();
                if (docType != nullptr) {
                    return docType->scriptValue();
                }
            }
            return ESValue(ESValue::ESNull);
        },
        nullptr);

    ESFunctionObject* createDocumentFragmentFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            try {
                ESValue thisValue =
                    instance->currentExecutionContext()->resolveThisBinding();
                CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
                Node* obj = (Node*)thisValue.asESPointer()
                                ->asESObject()
                                ->extraPointerData();

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
        },
        ESString::create("DocumentFragment"), 1, false);
    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("createDocumentFragment"), false,
                             false, false, createDocumentFragmentFunction);

    ESFunctionObject* createElementFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            try {
                ESValue thisValue =
                    instance->currentExecutionContext()->resolveThisBinding();
                CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
                Node* obj = (Node*)thisValue.asESPointer()
                                ->asESObject()
                                ->extraPointerData();

                if (obj->isDocument()) {
                    Document* doc = obj->asDocument();
                    ESValue argValue =
                        instance->currentExecutionContext()->readArgument(0);
                    if (argValue.isUndefined()) {
                        AtomicString name =
                            AtomicString::createAttrAtomicString(
                                doc->window()->starFish(), "undefined");
                        Element* elem = doc->createElement(name, true);
                        if (elem != nullptr) {
                            return elem->scriptValue();
                        }
                    } else if (argValue.isNull()) {
                        AtomicString name =
                            AtomicString::createAttrAtomicString(
                                doc->window()->starFish(), "null");
                        Element* elem = doc->createElement(name, true);
                        if (elem != nullptr) {
                            return elem->scriptValue();
                        }
                    } else if (argValue.isESString()) {
                        ESString* argStr = argValue.asESString();
                        auto bStr = toBrowserString(argStr);
                        if (!QualifiedName::checkNameProductionRule(
                                bStr, bStr->length())) {
                            throw new DOMException(
                                doc->window()->scriptBindingInstance(),
                                DOMException::Code::INVALID_CHARACTER_ERR,
                                nullptr);
                        }
                        AtomicString name =
                            AtomicString::createAttrAtomicString(
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
        },
        ESString::create("createElement"), 1, false);
    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("createElement"), false, false,
                             false, createElementFunction);

    ESFunctionObject* createTextNodeFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            ESValue thisValue =
                instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()
                            ->asESObject()
                            ->extraPointerData();

            if (obj->isDocument()) {
                Document* doc = obj->asDocument();
                ESValue argValue =
                    instance->currentExecutionContext()->readArgument(0);
                if (argValue.isESString()) {
                    ESString* argStr = argValue.asESString();
                    Text* elem = doc->createTextNode(toBrowserString(argStr));
                    if (elem != nullptr) {
                        return elem->scriptValue();
                    }
                } else if (argValue.isNull()) {
                    Text* elem = doc->createTextNode(
                        toBrowserString(ESString::create("null")));
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
        },
        ESString::create("createTextNode"), 1, false);
    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("createTextNode"), false, false,
                             false, createTextNodeFunction);

    ESFunctionObject* createCommentNodeFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            ESValue thisValue =
                instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()
                            ->asESObject()
                            ->extraPointerData();

            if (obj->isDocument()) {
                Document* doc = obj->asDocument();
                ESValue argValue =
                    instance->currentExecutionContext()->readArgument(0);
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
        },
        ESString::create("createComment"), 1, false);
    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("createComment"), false, false,
                             false, createCommentNodeFunction);

    ESFunctionObject* getElementsByTagNameFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            ESValue thisValue =
                instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()
                            ->asESObject()
                            ->extraPointerData();

            if (obj->isDocument()) {
                Document* doc = obj->asDocument();
                ESValue argValue =
                    instance->currentExecutionContext()->readArgument(0);
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
        },
        ESString::create("getElementsByTagName"), 1, false);
    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("getElementsByTagName"), false,
                             false, false, getElementsByTagNameFunction);

    ESFunctionObject* getElementsByClassNameFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            ESValue thisValue =
                instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()
                            ->asESObject()
                            ->extraPointerData();

            if (obj->isDocument()) {
                Document* doc = obj->asDocument();
                ESValue argValue =
                    instance->currentExecutionContext()->readArgument(0);
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
                    ESArrayObject* array =
                        argValue.asESPointer()->asESArrayObject();
                    String* listSoFar = String::createASCIIString("");
                    for (unsigned i = 0; i < array->length(); i++) {
                        ESValue val = array->get(i);
                        if (val.isESString()) {
                            listSoFar = listSoFar->concat(
                                toBrowserString(val.asESString()));
                            if (i < array->length() - 1) {
                                listSoFar = listSoFar->concat(
                                    String::createASCIIString(","));
                            }
                        } else {
                            return ESValue(ESValue::ESNull);
                        }
                    }
                    HTMLCollection* result =
                        doc->getElementsByClassName(listSoFar);
                    if (result) {
                        return result->scriptValue();
                    }
                }
            } else {
                THROW_ILLEGAL_INVOCATION()
            }
            return ESValue(ESValue::ESNull);
        },
        ESString::create("getElementsByClassName"), 1, false);
    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("getElementsByClassName"), false,
                             false, false, getElementsByClassNameFunction);

    ESFunctionObject* createAttributeFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            try {
                ESValue thisValue =
                    instance->currentExecutionContext()->resolveThisBinding();
                CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
                Node* obj = (Node*)thisValue.asESPointer()
                                ->asESObject()
                                ->extraPointerData();

                if (obj->isDocument()) {
                    Document* doc = obj->asDocument();
                    ESValue argValue =
                        instance->currentExecutionContext()->readArgument(0);
                    if (argValue.isUndefined()) {
                        Attr* result = doc->createAttribute(QualifiedName(
                            AtomicString::emptyAtomicString(),
                            AtomicString::createAttrAtomicString(
                                doc->window()->starFish(), "undefined")));
                        if (result != nullptr) {
                            return result->scriptValue();
                        }
                    } else if (argValue.isNull()) {
                        Attr* result = doc->createAttribute(QualifiedName(
                            AtomicString::emptyAtomicString(),
                            AtomicString::createAttrAtomicString(
                                doc->window()->starFish(), "null")));
                        if (result != nullptr) {
                            return result->scriptValue();
                        }
                    } else if (argValue.isESString()) {
                        ESString* argStr = argValue.asESString();
                        Attr* result = doc->createAttribute(
                            QualifiedName(AtomicString::emptyAtomicString(),
                                          AtomicString::createAttrAtomicString(
                                              doc->window()->starFish(),
                                              argStr->utf8Data())));
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
        },
        ESString::create("createAttribute"), 1, false);
    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("createAttribute"), false, false,
                             false, createAttributeFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("children"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            if (originalObj->isDocument()) {
                return originalObj->children()->scriptValue();
            }
            THROW_ILLEGAL_INVOCATION()
        },
        nullptr);

    /* Page Visibility */
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("hidden"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            if (originalObj->isDocument()) {
                bool hidden = originalObj->asDocument()->hidden();
                return ESValue(hidden);
            }
            THROW_ILLEGAL_INVOCATION()
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("visibilityState"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            if (originalObj->isDocument()) {
                String* visibilityState =
                    originalObj->asDocument()->visibilityState();
                return toJSString(visibilityState);
            }
            THROW_ILLEGAL_INVOCATION()
        },
        nullptr);

    ESFunctionObject* elementFromPointFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            ESValue thisValue =
                instance->currentExecutionContext()->resolveThisBinding();
            CHECK_TYPEOF(thisValue, ScriptWrappable::Type::NodeObject);
            Node* obj = (Node*)thisValue.asESPointer()
                            ->asESObject()
                            ->extraPointerData();
            if (obj->isDocument()) {
                Document* doc = obj->asDocument();
                ESValue argValue0 =
                    instance->currentExecutionContext()->readArgument(0);
                ESValue argValue1 =
                    instance->currentExecutionContext()->readArgument(0);
                Element* element = doc->elementFromPoint(argValue0.toNumber(),
                                                         argValue1.toNumber());
                if (element) {
                    return element->scriptValue();
                }
            } else {
                THROW_ILLEGAL_INVOCATION()
            }
            return ESValue(ESValue::ESNull);
        },
        ESString::create("elementFromPoint"), 2, false);
    DocumentFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("elementFromPoint"), false, false,
                             false, elementFromPointFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onclick"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                return nd->attributeEventListener(nd->document()
                                                      ->window()
                                                      ->starFish()
                                                      ->staticStrings()
                                                      ->m_click);
            }
            THROW_ILLEGAL_INVOCATION();
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                auto eventType = nd->document()
                                     ->window()
                                     ->starFish()
                                     ->staticStrings()
                                     ->m_click;
                if (v.isObject() || (v.isESPointer() &&
                                     v.asESPointer()->isESFunctionObject())) {
                    nd->setAttributeEventListener(eventType, v);
                } else {
                    nd->clearAttributeEventListener(eventType);
                }
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onmouseover"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                return nd->attributeEventListener(nd->document()
                                                      ->window()
                                                      ->starFish()
                                                      ->staticStrings()
                                                      ->m_mouseover);
            }
            THROW_ILLEGAL_INVOCATION();
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                auto eventType = nd->document()
                                     ->window()
                                     ->starFish()
                                     ->staticStrings()
                                     ->m_mouseover;
                if (v.isObject() || (v.isESPointer() &&
                                     v.asESPointer()->isESFunctionObject())) {
                    nd->setAttributeEventListener(eventType, v);
                } else {
                    nd->clearAttributeEventListener(eventType);
                }
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onfocus"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                return nd->attributeEventListener(nd->document()
                                                      ->window()
                                                      ->starFish()
                                                      ->staticStrings()
                                                      ->m_focus);
            }
            THROW_ILLEGAL_INVOCATION();
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                auto eventType = nd->document()
                                     ->window()
                                     ->starFish()
                                     ->staticStrings()
                                     ->m_focus;
                if (v.isObject() || (v.isESPointer() &&
                                     v.asESPointer()->isESFunctionObject())) {
                    nd->setAttributeEventListener(eventType, v);
                } else {
                    nd->clearAttributeEventListener(eventType);
                }
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onkeydown"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                return nd->attributeEventListener(nd->document()
                                                      ->window()
                                                      ->starFish()
                                                      ->staticStrings()
                                                      ->m_keydown);
            }
            THROW_ILLEGAL_INVOCATION();
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                auto eventType = nd->document()
                                     ->window()
                                     ->starFish()
                                     ->staticStrings()
                                     ->m_keydown;
                if (v.isObject() || (v.isESPointer() &&
                                     v.asESPointer()->isESFunctionObject())) {
                    nd->setAttributeEventListener(eventType, v);
                } else {
                    nd->clearAttributeEventListener(eventType);
                }
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("defaultView"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                Document* document = nd->asDocument();
                Window* window = document->window();
                return window->scriptValue();
            }
            return ESValue(ESValue::ESNull);
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("location"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                LocationObj* location = nd->asDocument()->location();
                return location->scriptValue();
            }
            return ESValue(ESValue::ESNull);
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isDocument()) {
                nd->asDocument()->location()->setHref(
                    String::fromUTF8(v.toString()->utf8Data()));
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
            return ESValue();
        });

    return DocumentFunction;
}
}
