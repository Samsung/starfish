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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"
#include "extra/Location.h"

namespace StarFish {

using namespace escargot;

static ESValue headGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        Document* document = nd->asDocument();
        HTMLHeadElement* head = document->headElement();
        if (head) {
            return head->scriptValue();
        }
    }
    return ESValue(ESValue::ESNull);
}

static ESValue bodyGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        Document* document = nd->asDocument();
        HTMLBodyElement* body = document->bodyElement();
        if (body) {
            return body->scriptValue();
        }
    }
    return ESValue(ESValue::ESNull);
}

static ESValue bodySetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0), Node);
    ESValue v = instance->currentExecutionContext()->readArgument(0);
    if (!v.isUndefinedOrNull()) {
        Node* nd = originalObj;
        Node* node_v = (Node*)instance->currentExecutionContext()
                           ->readArgument(0)
                           .asESPointer()
                           ->asESObject()
                           ->extraPointerData();
        if (nd->isDocument()) {
            if (node_v->isElement() && node_v->asElement()->isHTMLElement() &&
                node_v->asElement()->asHTMLElement()->isHTMLBodyElement()) {
                HTMLBodyElement* body = nd->asDocument()->bodyElement();
                HTMLHtmlElement* html_root = nd->asDocument()->rootElement();
                if (body) {
                    html_root->removeChild(body);
                }
                html_root->appendChild(node_v);
            } else {
                THROW_DOM_EXCEPTION(instance,
                                    DOMException::HIERARCHY_REQUEST_ERR);
            }
            return ESValue();
        }
    }
    THROW_ILLEGAL_INVOCATION();
    return ESValue();
}

static ESValue documentElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        Document* document = nd->asDocument();
        Element* docElem = document->documentElement();
        if (docElem) {
            return docElem->scriptValue();
        }
    }
    return ESValue(ESValue::ESNull);
}

#ifdef STARFISH_EXP
static ESValue implementationGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        Document* document = nd->asDocument();
        DOMImplementation* impl = document->domImplementation();
        if (impl) {
            return impl->scriptValue();
        }
    }
    return ESValue(ESValue::ESNull);
}
#endif

static ESValue characterSetGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        return toJSString(nd->asDocument()->charset());
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue charsetGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        return toJSString(nd->asDocument()->charset());
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue contentTypeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        return toJSString(nd->asDocument()->contentType());
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue compatModeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        return toJSString(nd->asDocument()->compatMode());
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue documentURIGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        return nd->asDocument()->documentURI()
                   ? toJSString(nd->asDocument()->documentURI()->urlString())
                   : ESValue(ESValue::ESNull);
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue urlGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        return nd->asDocument()->documentURI()
                   ? toJSString(nd->asDocument()->documentURI()->urlString())
                   : ESValue(ESValue::ESNull);
    }
    THROW_ILLEGAL_INVOCATION();
}

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

static ESValue childElementCountGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    return ESValue(originalObj->childElementCount());
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
            auto msg = ESString::create(
                "Failed to execute 'querySelector' on 'Document': "
                "1 argument required, but only 0 present.");
            instance->throwError(ESValue(TypeError::create(msg)));
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
}

static ESValue doctypeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        DocumentType* docType = nd->asDocument()->docType();
        if (docType != nullptr) {
            return docType->scriptValue();
        }
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
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    if (originalObj->isDocument()) {
        return originalObj->children()->scriptValue();
    }
    THROW_ILLEGAL_INVOCATION()
}

static ESValue hiddenGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    if (originalObj->isDocument()) {
        bool hidden = originalObj->asDocument()->hidden();
        return ESValue(hidden);
    }
    THROW_ILLEGAL_INVOCATION()
}

static ESValue visibilityStateGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    if (originalObj->isDocument()) {
        String* visibilityState = originalObj->asDocument()->visibilityState();
        return toJSString(visibilityState);
    }
    THROW_ILLEGAL_INVOCATION()
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
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        return nd->attributeEventListener(
            nd->document()->window()->starFish()->staticStrings()->m_click);
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue onClickSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        auto eventType =
            nd->document()->window()->starFish()->staticStrings()->m_click;
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        if (v.isObject() ||
            (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
            nd->setAttributeEventListener(eventType, v);
        } else {
            nd->clearAttributeEventListener(eventType);
        }
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
    return ESValue();
}

static ESValue onMouseOverGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        return nd->attributeEventListener(
            nd->document()->window()->starFish()->staticStrings()->m_mouseover);
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue onMouseOverSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        auto eventType =
            nd->document()->window()->starFish()->staticStrings()->m_mouseover;
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        if (v.isObject() ||
            (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
            nd->setAttributeEventListener(eventType, v);
        } else {
            nd->clearAttributeEventListener(eventType);
        }
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
    return ESValue();
}

static ESValue onFocusGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        return nd->attributeEventListener(
            nd->document()->window()->starFish()->staticStrings()->m_focus);
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue onFocusSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        auto eventType =
            nd->document()->window()->starFish()->staticStrings()->m_focus;
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        if (v.isObject() ||
            (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
            nd->setAttributeEventListener(eventType, v);
        } else {
            nd->clearAttributeEventListener(eventType);
        }
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
    return ESValue();
}

static ESValue onKeyDownGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        return nd->attributeEventListener(
            nd->document()->window()->starFish()->staticStrings()->m_keydown);
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue onKeyDownSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        auto eventType =
            nd->document()->window()->starFish()->staticStrings()->m_keydown;
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        if (v.isObject() ||
            (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
            nd->setAttributeEventListener(eventType, v);
        } else {
            nd->clearAttributeEventListener(eventType);
        }
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
    return ESValue();
}

static ESValue defaultViewGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        Document* document = nd->asDocument();
        Window* window = document->window();
        return window->scriptValue();
    }
    return ESValue(ESValue::ESNull);
}

static ESValue locationGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        Location* location = nd->asDocument()->location();
        return location->scriptValue();
    }
    return ESValue(ESValue::ESNull);
}

static ESValue locationSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocument()) {
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        nd->asDocument()->location()->setHref(
            String::fromUTF8(v.toString()->utf8Data()));
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
    return ESValue();
}

ESFunctionObject* bindingDocument(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        Document, fetchData(scriptBindingInstance)->node());

    /* 4.5 Interface Document */

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("head"), headGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("body"), bodyGetterFunction, bodySetterFunction);

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
        ESString::create("URL"), urlGetterFunction, nullptr);

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
        ESString::create("defaultView"), defaultViewGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        DocumentFunction->protoType().asESPointer()->asESObject(),
        ESString::create("location"), locationGetterFunction,
        locationSetterFunction);

    return DocumentFunction;
}
}
