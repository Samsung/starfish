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
#include "ScriptWrappable.h"

#include "dom/DOM.h"
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include <Escargot.h>

namespace StarFish {

using namespace escargot;

static ESValue nodeTypeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    unsigned short nodeType = originalObj->nodeType();
    return ESValue(nodeType);
}

static ESValue nodeNameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    String* nodeName = originalObj->nodeName();
    return toJSString(nodeName);
}

static ESValue ownerDocumentGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Document* doc = originalObj->ownerDocument();
    if (doc == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return doc->scriptValue();
}

static ESValue parentNodeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* p = originalObj->parentNode();
    if (p == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return p->scriptValue();
}

static ESValue parentElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Element* p = originalObj->parentElement();
    if (p == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return p->scriptValue();
}

static ESValue childNodesGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    NodeList* list = originalObj->childNodes();
    STARFISH_ASSERT(list);
    return list->scriptValue();
}

static ESValue nextSiblingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj->nextSibling();
    if (nd == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return nd->scriptValue();
}

static ESValue previousSiblingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj->previousSibling();
    if (nd == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return nd->scriptValue();
}

static ESValue firstChildGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj->firstChild();
    if (nd == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return nd->scriptValue();
}

static ESValue lastChildGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj->lastChild();
    if (nd == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return nd->scriptValue();
}

static ESValue nodeValueGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (!nd->isCharacterData()) {
        return ESValue(ESValue::ESNull);
    }
    String* s = nd->nodeValue();
    return toJSString(s);
}

static ESValue nodeValueSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* node = originalObj;
    ESValue v = instance->currentExecutionContext()->readArgument(0);
    if (v.isESString()) {
        node->setNodeValue(toBrowserString(v));
    } else if (v.isNull()) {
        node->setNodeValue(nullptr);
    }
    return ESValue();
}

static ESValue textContentGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isDocumentType() || nd->isDocument()) {
        return ESValue(ESValue::ESNull);
    }
    return toJSString(nd->textContent());
}

static ESValue textContentSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    ESValue v = instance->currentExecutionContext()->readArgument(0);
    String* arg_str = toBrowserString(v);
    if (v.isUndefinedOrNull() || !arg_str->length()) {
        originalObj->setTextContent(String::emptyString);
    } else {
        originalObj->setTextContent(arg_str);
    }
    return ESValue();
}

static ESValue cloneNodeFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, Node);
    Node* obj =
        (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
    ESValue arg = instance->currentExecutionContext()->readArgument(0);
    bool deepClone = false;
    if (arg.isBoolean()) {
        deepClone = arg.asBoolean();
    }
    Node* node = obj->cloneNode(deepClone);
    return node->scriptValue();
}

static ESValue hasChildNodesFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, Node);
    Node* obj =
        (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
    bool result = obj->hasChildNodes();
    return ESValue(result);
}

static ESValue isEqualNodeFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, Node);

    ESValue argValue = instance->currentExecutionContext()->readArgument(0);

    if (argValue.isUndefinedOrNull()) {
        return ESValue(false);
    }

    Node* obj =
        (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
    Node* node =
        (Node*)argValue.asESPointer()->asESObject()->extraPointerData();
    bool found = obj->isEqualNode(node);
    return ESValue(found);
}

static ESValue documentPositionDisconnectedGetterFunction(
    ESObject* obj, ESObject* originalObj, ESString* propertyName)
{
    return ESValue(Node::DOCUMENT_POSITION_DISCONNECTED);
}

static ESValue documentPositionPrecedingGetterFunction(ESObject* obj,
                                                       ESObject* originalObj,
                                                       ESString* propertyName)
{
    return ESValue(Node::DOCUMENT_POSITION_PRECEDING);
}

static ESValue documentPositionFollowingGetterFunction(ESObject* obj,
                                                       ESObject* originalObj,
                                                       ESString* propertyName)
{
    return ESValue(Node::DOCUMENT_POSITION_FOLLOWING);
}

static ESValue documentPositionContainsGetterFunction(ESObject* obj,
                                                      ESObject* originalObj,
                                                      ESString* propertyName)
{
    return ESValue(Node::DOCUMENT_POSITION_CONTAINS);
}

static ESValue documentPositionContainedByGetterFunction(ESObject* obj,
                                                         ESObject* originalObj,
                                                         ESString* propertyName)
{
    return ESValue(Node::DOCUMENT_POSITION_CONTAINED_BY);
}

static ESValue documentPositionImplementationSpecificGetterFunction(
    ESObject* obj, ESObject* originalObj, ESString* propertyName)
{
    return ESValue(Node::DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC);
}

static ESValue compareDocumentPositionGetterFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, Node);
    CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0), Node);
    Node* obj =
        (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
    Node* nodeRef = (Node*)instance->currentExecutionContext()
                        ->readArgument(0)
                        .asESPointer()
                        ->asESObject()
                        ->extraPointerData();
    unsigned short pos = obj->compareDocumentPosition(nodeRef);
    return ESValue(pos);
}

static ESValue containsFunction(ESVMInstance* instance)
{
    ESValue thisValue =
        instance->currentExecutionContext()->resolveThisBinding();
    CHECK_TYPEOF(thisValue, Node);
    Node* obj =
        (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
    if (instance->currentExecutionContext()->readArgument(0).isNull()) {
        return ESValue(obj->contains(nullptr));
    }
    CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0), Node);
    Node* nodeRef = (Node*)instance->currentExecutionContext()
                        ->readArgument(0)
                        .asESPointer()
                        ->asESObject()
                        ->extraPointerData();
    bool found = obj->contains(nodeRef);
    return ESValue(found);
}

static ESValue appendChildFunction(ESVMInstance* instance)
{
    try {
        ESValue thisValue =
            instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF_WITH_ERRCODE(thisValue, Node, instance,
                                  DOMException::HIERARCHY_REQUEST_ERR);
        CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0),
                     Node);
        Node* obj =
            (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
        Node* child = (Node*)instance->currentExecutionContext()
                          ->readArgument(0)
                          .asESPointer()
                          ->asESObject()
                          ->extraPointerData();
        obj->appendChild(child);
        return child->scriptValue();
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue removeChildFunction(ESVMInstance* instance)
{
    try {
        ESValue thisValue =
            instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF_WITH_ERRCODE(thisValue, Node, instance,
                                  DOMException::NOT_FOUND_ERR);
        CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0),
                     Node);
        Node* obj =
            (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
        Node* child = (Node*)instance->currentExecutionContext()
                          ->readArgument(0)
                          .asESPointer()
                          ->asESObject()
                          ->extraPointerData();
        Node* n = obj->removeChild(child);
        return n->scriptValue();
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue replaceChildFunction(ESVMInstance* instance)
{
    try {
        ESValue thisValue =
            instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, Node);
        CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0),
                     Node);
        CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(1),
                     Node);
        Node* obj =
            (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();
        Node* node = (Node*)instance->currentExecutionContext()
                         ->readArgument(0)
                         .asESPointer()
                         ->asESObject()
                         ->extraPointerData();
        Node* child = (Node*)instance->currentExecutionContext()
                          ->readArgument(1)
                          .asESPointer()
                          ->asESObject()
                          ->extraPointerData();
        Node* n = obj->replaceChild(node, child);
        return n->scriptValue();
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

static ESValue insertBeforeFunction(ESVMInstance* instance)
{
    try {
        ESValue thisValue =
            instance->currentExecutionContext()->resolveThisBinding();
        CHECK_TYPEOF(thisValue, Node);
        Node* obj =
            (Node*)thisValue.asESPointer()->asESObject()->extraPointerData();

        CHECK_TYPEOF(instance->currentExecutionContext()->readArgument(0),
                     Node);
        Node* node = (Node*)instance->currentExecutionContext()
                         ->readArgument(0)
                         .asESPointer()
                         ->asESObject()
                         ->extraPointerData();

        ESValue arg2 = instance->currentExecutionContext()->readArgument(1);
        Node* child = nullptr;
        if (!arg2.isNull()) {
            CHECK_TYPEOF(arg2, Node);
            child = (Node*)arg2.asESPointer()->asESObject()->extraPointerData();
        }

        Node* n = obj->insertBefore(node, child);
        return n->scriptValue();
    } catch (DOMException* e) {
        ESVMInstance::currentInstance()->throwError(e->scriptValue());
        return ESValue(ESValue::ESNull);
    }
}

ESFunctionObject* bindingNode(ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        Node, fetchData(scriptBindingInstance)->m_eventTarget);

    /* 4.4 Interface Node */
    NodeFunction->asESObject()->defineDataProperty(
        ESString::create("ELEMENT_NODE"), false, true, false,
        ESValue(Node::ELEMENT_NODE));
    NodeFunction->asESObject()->defineDataProperty(
        ESString::create("ATTRIBUTE_NODE"), false, true, false,
        ESValue(Node::ATTRIBUTE_NODE));
    NodeFunction->asESObject()->defineDataProperty(
        ESString::create("TEXT_NODE"), false, true, false,
        ESValue(Node::TEXT_NODE));
    NodeFunction->asESObject()->defineDataProperty(
        ESString::create("CDATA_SECTION_NODE"), false, true, false,
        ESValue(Node::CDATA_SECTION_NODE));
    NodeFunction->asESObject()->defineDataProperty(
        ESString::create("ENTITY_REFERENCE_NODE"), false, true, false,
        ESValue(Node::ENTITY_REFERENCE_NODE));
    NodeFunction->asESObject()->defineDataProperty(
        ESString::create("ENTITY_NODE"), false, true, false,
        ESValue(Node::ENTITY_NODE));
    NodeFunction->asESObject()->defineDataProperty(
        ESString::create("PROCESSING_INSTRUCTION_NODE"), false, true, false,
        ESValue(Node::PROCESSING_INSTRUCTION_NODE));
    NodeFunction->asESObject()->defineDataProperty(
        ESString::create("COMMENT_NODE"), false, true, false,
        ESValue(Node::COMMENT_NODE));
    NodeFunction->asESObject()->defineDataProperty(
        ESString::create("DOCUMENT_NODE"), false, true, false,
        ESValue(Node::DOCUMENT_NODE));
    NodeFunction->asESObject()->defineDataProperty(
        ESString::create("DOCUMENT_TYPE_NODE"), false, true, false,
        ESValue(Node::DOCUMENT_TYPE_NODE));
    NodeFunction->asESObject()->defineDataProperty(
        ESString::create("DOCUMENT_FRAGMENT_NODE"), false, true, false,
        ESValue(Node::DOCUMENT_FRAGMENT_NODE));
    NodeFunction->asESObject()->defineDataProperty(
        ESString::create("NOTATION_NODE"), false, true, false,
        ESValue(Node::NOTATION_NODE));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("ELEMENT_NODE"), false, true, false,
        ESValue(Node::ELEMENT_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("ATTRIBUTE_NODE"), false, true, false,
        ESValue(Node::ATTRIBUTE_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("TEXT_NODE"), false, true, false,
        ESValue(Node::TEXT_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("CDATA_SECTION_NODE"), false, true, false,
        ESValue(Node::CDATA_SECTION_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("ENTITY_REFERENCE_NODE"), false, true, false,
        ESValue(Node::ENTITY_REFERENCE_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("ENTITY_NODE"), false, true, false,
        ESValue(Node::ENTITY_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("PROCESSING_INSTRUCTION_NODE"), false, true, false,
        ESValue(Node::PROCESSING_INSTRUCTION_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("COMMENT_NODE"), false, true, false,
        ESValue(Node::COMMENT_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("DOCUMENT_NODE"), false, true, false,
        ESValue(Node::DOCUMENT_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("DOCUMENT_TYPE_NODE"), false, true, false,
        ESValue(Node::DOCUMENT_TYPE_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("DOCUMENT_FRAGMENT_NODE"), false, true, false,
        ESValue(Node::DOCUMENT_FRAGMENT_NODE));
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("NOTATION_NODE"), false, true, false,
        ESValue(Node::NOTATION_NODE));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(),
        ESString::create("nodeType"), nodeTypeGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(),
        ESString::create("nodeName"), nodeNameGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(),
        ESString::create("ownerDocument"), ownerDocumentGetterFunction,
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(),
        ESString::create("parentNode"), parentNodeGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(),
        ESString::create("parentElement"), parentElementGetterFunction,
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(),
        ESString::create("childNodes"), childNodesGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(),
        ESString::create("nextSibling"), nextSiblingGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(),
        ESString::create("previousSibling"), previousSiblingGetterFunction,
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(),
        ESString::create("firstChild"), firstChildGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(),
        ESString::create("lastChild"), lastChildGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(),
        ESString::create("nodeValue"), nodeValueGetterFunction,
        nodeValueSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(),
        ESString::create("textContent"), textContentGetterFunction,
        textContentSetterFunction);

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("cloneNode"), true, true, true,
        ESFunctionObject::create(nullptr, cloneNodeFunction,
                                 ESString::create("cloneNode"), 0, false));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("hasChildNodes"), true, true, true,
        ESFunctionObject::create(nullptr, hasChildNodesFunction,
                                 ESString::create("hasChildNodes"), 0, false));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("isEqualNode"), true, true, true,
        ESFunctionObject::create(nullptr, isEqualNodeFunction,
                                 ESString::create("isEqualNode"), 1, false));

    NodeFunction->defineAccessorProperty(
        ESString::create("DOCUMENT_POSITION_DISCONNECTED"),
        documentPositionDisconnectedGetterFunction, NULL, false, false, false);

    NodeFunction->defineAccessorProperty(
        ESString::create("DOCUMENT_POSITION_PRECEDING"),
        documentPositionPrecedingGetterFunction, NULL, false, false, false);

    NodeFunction->defineAccessorProperty(
        ESString::create("DOCUMENT_POSITION_FOLLOWING"),
        documentPositionFollowingGetterFunction, NULL, false, false, false);

    NodeFunction->defineAccessorProperty(
        ESString::create("DOCUMENT_POSITION_CONTAINS"),
        documentPositionContainsGetterFunction, NULL, false, false, false);

    NodeFunction->defineAccessorProperty(
        ESString::create("DOCUMENT_POSITION_CONTAINED_BY"),
        documentPositionContainedByGetterFunction, NULL, false, false, false);

    NodeFunction->defineAccessorProperty(
        ESString::create("DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC"),
        documentPositionImplementationSpecificGetterFunction, NULL, false,
        false, false);

    NodeFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineAccessorProperty(
            ESString::create("DOCUMENT_POSITION_DISCONNECTED"),
            documentPositionDisconnectedGetterFunction, NULL, false, false,
            false);

    NodeFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineAccessorProperty(
            ESString::create("DOCUMENT_POSITION_PRECEDING"),
            documentPositionPrecedingGetterFunction, NULL, false, false, false);

    NodeFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineAccessorProperty(
            ESString::create("DOCUMENT_POSITION_FOLLOWING"),
            documentPositionFollowingGetterFunction, NULL, false, false, false);

    NodeFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineAccessorProperty(ESString::create("DOCUMENT_POSITION_CONTAINS"),
                                 documentPositionContainsGetterFunction, NULL,
                                 false, false, false);

    NodeFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineAccessorProperty(
            ESString::create("DOCUMENT_POSITION_CONTAINED_BY"),
            documentPositionContainedByGetterFunction, NULL, false, false,
            false);

    NodeFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineAccessorProperty(
            ESString::create("DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC"),
            documentPositionImplementationSpecificGetterFunction, NULL, false,
            false, false);

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("compareDocumentPosition"), true, true, true,
        ESFunctionObject::create(nullptr, compareDocumentPositionGetterFunction,
                                 ESString::create("compareDocumentPosition"), 1,
                                 false));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("contains"), true, true, true,
        ESFunctionObject::create(nullptr, containsFunction,
                                 ESString::create("contains"), 1, false));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("appendChild"), true, true, true,
        ESFunctionObject::create(nullptr, appendChildFunction,
                                 ESString::create("appendChild"), 1, false));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("removeChild"), true, true, true,
        ESFunctionObject::create(nullptr, removeChildFunction,
                                 ESString::create("removeChild"), 1, false));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("replaceChild"), true, true, true,
        ESFunctionObject::create(nullptr, replaceChildFunction,
                                 ESString::create("replaceChild"), 2, false));

    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ESString::create("insertBefore"), true, true, true,
        ESFunctionObject::create(nullptr, insertBeforeFunction,
                                 ESString::create("insertBefore"), 2, false));
    return NodeFunction;
}
}
