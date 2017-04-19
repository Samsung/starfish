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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/DOMException.h"
#include "dom/Node.h"

namespace StarFish {

using namespace escargot;

// Implement for attributes
static ESValue nodeTypeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    // Declare return value (empty when void)
    uint32_t result;
    result = originalObj->nodeType();
    // Return ESValue from native value
    return ESValue(result);
}

static ESValue nodeNameGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    // Declare return value (empty when void)
    String* result = String::emptyString;
    result = originalObj->nodeName();
    // Return ESValue from native value
    return toJSString(result);
}

static ESValue ownerDocumentGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    // Declare return value (empty when void)
    Document* result = nullptr;
    result = originalObj->ownerDocument();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue parentNodeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    // Declare return value (empty when void)
    Node* result = nullptr;
    result = originalObj->parentNode();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue parentElementGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    // Declare return value (empty when void)
    Element* result = nullptr;
    result = originalObj->parentElement();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue childNodesGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    // Declare return value (empty when void)
    NodeList* result = nullptr;
    result = originalObj->childNodes();
    // Return ESValue from native value
    STARFISH_ASSERT(result != nullptr);
    return result->scriptValue();
}

static ESValue firstChildGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    // Declare return value (empty when void)
    Node* result = nullptr;
    result = originalObj->firstChild();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue lastChildGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    // Declare return value (empty when void)
    Node* result = nullptr;
    result = originalObj->lastChild();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue previousSiblingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    // Declare return value (empty when void)
    Node* result = nullptr;
    result = originalObj->previousSibling();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue nextSiblingGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    // Declare return value (empty when void)
    Node* result = nullptr;
    result = originalObj->nextSibling();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return result->scriptValue();
}

static ESValue nodeValueGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    // Declare return value (empty when void)
    // FIXME temporally use raw String*
    String* result = String::emptyString;
    result = originalObj->nodeValue();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return toJSString(result);
}

static ESValue nodeValueSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    // FIXME temporally use raw String*
    String* value0 = String::emptyString;
    if (!arg0.isUndefinedOrNull()) {
        value0 = toBrowserString(arg0);
    }
    originalObj->setNodeValue(value0);
    return ESValue();
}

static ESValue textContentGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    // Declare return value (empty when void)
    // FIXME temporally use raw String*
    String* result = String::emptyString;
    result = originalObj->textContent();
    // Return ESValue from native value
    if (result == nullptr) {
        return ESValue(ESValue::ESNull);
    }
    return toJSString(result);
}

static ESValue textContentSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
    // Handle argument arg0
    // FIXME temporally use raw String*
    String* value0 = String::emptyString;
    if (!arg0.isUndefinedOrNull()) {
        value0 = toBrowserString(arg0);
    }
    originalObj->setTextContent(value0);
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
        Node, fetchData(scriptBindingInstance)->m_fnEventTarget);

    // Bind for constants
    ESString* ELEMENT_NODEString = ESString::create("ELEMENT_NODE");
    ESValue ELEMENT_NODEValue = ESValue(1);
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ELEMENT_NODEString, false, true, false, ELEMENT_NODEValue);

    NodeFunction->asESObject()->defineDataProperty(
        ELEMENT_NODEString, false, true, false, ELEMENT_NODEValue);

    ESString* ATTRIBUTE_NODEString = ESString::create("ATTRIBUTE_NODE");
    ESValue ATTRIBUTE_NODEValue = ESValue(2);
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ATTRIBUTE_NODEString, false, true, false, ATTRIBUTE_NODEValue);

    NodeFunction->asESObject()->defineDataProperty(
        ATTRIBUTE_NODEString, false, true, false, ATTRIBUTE_NODEValue);

    ESString* TEXT_NODEString = ESString::create("TEXT_NODE");
    ESValue TEXT_NODEValue = ESValue(3);
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        TEXT_NODEString, false, true, false, TEXT_NODEValue);

    NodeFunction->asESObject()->defineDataProperty(TEXT_NODEString, false, true,
                                                   false, TEXT_NODEValue);

    ESString* CDATA_SECTION_NODEString = ESString::create("CDATA_SECTION_NODE");
    ESValue CDATA_SECTION_NODEValue = ESValue(4);
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        CDATA_SECTION_NODEString, false, true, false, CDATA_SECTION_NODEValue);

    NodeFunction->asESObject()->defineDataProperty(
        CDATA_SECTION_NODEString, false, true, false, CDATA_SECTION_NODEValue);

    ESString* ENTITY_REFERENCE_NODEString =
        ESString::create("ENTITY_REFERENCE_NODE");
    ESValue ENTITY_REFERENCE_NODEValue = ESValue(5);
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ENTITY_REFERENCE_NODEString, false, true, false,
        ENTITY_REFERENCE_NODEValue);

    NodeFunction->asESObject()->defineDataProperty(ENTITY_REFERENCE_NODEString,
                                                   false, true, false,
                                                   ENTITY_REFERENCE_NODEValue);

    ESString* ENTITY_NODEString = ESString::create("ENTITY_NODE");
    ESValue ENTITY_NODEValue = ESValue(6);
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        ENTITY_NODEString, false, true, false, ENTITY_NODEValue);

    NodeFunction->asESObject()->defineDataProperty(
        ENTITY_NODEString, false, true, false, ENTITY_NODEValue);

    ESString* PROCESSING_INSTRUCTION_NODEString =
        ESString::create("PROCESSING_INSTRUCTION_NODE");
    ESValue PROCESSING_INSTRUCTION_NODEValue = ESValue(7);
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        PROCESSING_INSTRUCTION_NODEString, false, true, false,
        PROCESSING_INSTRUCTION_NODEValue);

    NodeFunction->asESObject()->defineDataProperty(
        PROCESSING_INSTRUCTION_NODEString, false, true, false,
        PROCESSING_INSTRUCTION_NODEValue);

    ESString* COMMENT_NODEString = ESString::create("COMMENT_NODE");
    ESValue COMMENT_NODEValue = ESValue(8);
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        COMMENT_NODEString, false, true, false, COMMENT_NODEValue);

    NodeFunction->asESObject()->defineDataProperty(
        COMMENT_NODEString, false, true, false, COMMENT_NODEValue);

    ESString* DOCUMENT_NODEString = ESString::create("DOCUMENT_NODE");
    ESValue DOCUMENT_NODEValue = ESValue(9);
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        DOCUMENT_NODEString, false, true, false, DOCUMENT_NODEValue);

    NodeFunction->asESObject()->defineDataProperty(
        DOCUMENT_NODEString, false, true, false, DOCUMENT_NODEValue);

    ESString* DOCUMENT_TYPE_NODEString = ESString::create("DOCUMENT_TYPE_NODE");
    ESValue DOCUMENT_TYPE_NODEValue = ESValue(10);
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        DOCUMENT_TYPE_NODEString, false, true, false, DOCUMENT_TYPE_NODEValue);

    NodeFunction->asESObject()->defineDataProperty(
        DOCUMENT_TYPE_NODEString, false, true, false, DOCUMENT_TYPE_NODEValue);

    ESString* DOCUMENT_FRAGMENT_NODEString =
        ESString::create("DOCUMENT_FRAGMENT_NODE");
    ESValue DOCUMENT_FRAGMENT_NODEValue = ESValue(11);
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        DOCUMENT_FRAGMENT_NODEString, false, true, false,
        DOCUMENT_FRAGMENT_NODEValue);

    NodeFunction->asESObject()->defineDataProperty(DOCUMENT_FRAGMENT_NODEString,
                                                   false, true, false,
                                                   DOCUMENT_FRAGMENT_NODEValue);

    ESString* NOTATION_NODEString = ESString::create("NOTATION_NODE");
    ESValue NOTATION_NODEValue = ESValue(12);
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        NOTATION_NODEString, false, true, false, NOTATION_NODEValue);

    NodeFunction->asESObject()->defineDataProperty(
        NOTATION_NODEString, false, true, false, NOTATION_NODEValue);

    ESString* DOCUMENT_POSITION_DISCONNECTEDString =
        ESString::create("DOCUMENT_POSITION_DISCONNECTED");
    ESValue DOCUMENT_POSITION_DISCONNECTEDValue = ESValue(0x01);
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        DOCUMENT_POSITION_DISCONNECTEDString, false, true, false,
        DOCUMENT_POSITION_DISCONNECTEDValue);

    NodeFunction->asESObject()->defineDataProperty(
        DOCUMENT_POSITION_DISCONNECTEDString, false, true, false,
        DOCUMENT_POSITION_DISCONNECTEDValue);

    ESString* DOCUMENT_POSITION_PRECEDINGString =
        ESString::create("DOCUMENT_POSITION_PRECEDING");
    ESValue DOCUMENT_POSITION_PRECEDINGValue = ESValue(0x02);
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        DOCUMENT_POSITION_PRECEDINGString, false, true, false,
        DOCUMENT_POSITION_PRECEDINGValue);

    NodeFunction->asESObject()->defineDataProperty(
        DOCUMENT_POSITION_PRECEDINGString, false, true, false,
        DOCUMENT_POSITION_PRECEDINGValue);

    ESString* DOCUMENT_POSITION_FOLLOWINGString =
        ESString::create("DOCUMENT_POSITION_FOLLOWING");
    ESValue DOCUMENT_POSITION_FOLLOWINGValue = ESValue(0x04);
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        DOCUMENT_POSITION_FOLLOWINGString, false, true, false,
        DOCUMENT_POSITION_FOLLOWINGValue);

    NodeFunction->asESObject()->defineDataProperty(
        DOCUMENT_POSITION_FOLLOWINGString, false, true, false,
        DOCUMENT_POSITION_FOLLOWINGValue);

    ESString* DOCUMENT_POSITION_CONTAINSString =
        ESString::create("DOCUMENT_POSITION_CONTAINS");
    ESValue DOCUMENT_POSITION_CONTAINSValue = ESValue(0x08);
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        DOCUMENT_POSITION_CONTAINSString, false, true, false,
        DOCUMENT_POSITION_CONTAINSValue);

    NodeFunction->asESObject()->defineDataProperty(
        DOCUMENT_POSITION_CONTAINSString, false, true, false,
        DOCUMENT_POSITION_CONTAINSValue);

    ESString* DOCUMENT_POSITION_CONTAINED_BYString =
        ESString::create("DOCUMENT_POSITION_CONTAINED_BY");
    ESValue DOCUMENT_POSITION_CONTAINED_BYValue = ESValue(0x10);
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        DOCUMENT_POSITION_CONTAINED_BYString, false, true, false,
        DOCUMENT_POSITION_CONTAINED_BYValue);

    NodeFunction->asESObject()->defineDataProperty(
        DOCUMENT_POSITION_CONTAINED_BYString, false, true, false,
        DOCUMENT_POSITION_CONTAINED_BYValue);

    ESString* DOCUMENT_POSITION_IMPLEMENTATION_SPECIFICString =
        ESString::create("DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC");
    ESValue DOCUMENT_POSITION_IMPLEMENTATION_SPECIFICValue = ESValue(0x20);
    NodeFunction->protoType().asESPointer()->asESObject()->defineDataProperty(
        DOCUMENT_POSITION_IMPLEMENTATION_SPECIFICString, false, true, false,
        DOCUMENT_POSITION_IMPLEMENTATION_SPECIFICValue);

    NodeFunction->asESObject()->defineDataProperty(
        DOCUMENT_POSITION_IMPLEMENTATION_SPECIFICString, false, true, false,
        DOCUMENT_POSITION_IMPLEMENTATION_SPECIFICValue);

    // Bind for attributes
    ESString* nodeTypeString = ESString::create("nodeType");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(), nodeTypeString,
        nodeTypeGetterFunction, nullptr);

    ESString* nodeNameString = ESString::create("nodeName");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(), nodeNameString,
        nodeNameGetterFunction, nullptr);

    ESString* ownerDocumentString = ESString::create("ownerDocument");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(),
        ownerDocumentString, ownerDocumentGetterFunction, nullptr);

    ESString* parentNodeString = ESString::create("parentNode");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(), parentNodeString,
        parentNodeGetterFunction, nullptr);

    ESString* parentElementString = ESString::create("parentElement");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(),
        parentElementString, parentElementGetterFunction, nullptr);

    ESString* childNodesString = ESString::create("childNodes");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(), childNodesString,
        childNodesGetterFunction, nullptr);

    ESString* firstChildString = ESString::create("firstChild");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(), firstChildString,
        firstChildGetterFunction, nullptr);

    ESString* lastChildString = ESString::create("lastChild");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(), lastChildString,
        lastChildGetterFunction, nullptr);

    ESString* previousSiblingString = ESString::create("previousSibling");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(),
        previousSiblingString, previousSiblingGetterFunction, nullptr);

    ESString* nextSiblingString = ESString::create("nextSibling");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(),
        nextSiblingString, nextSiblingGetterFunction, nullptr);

    ESString* nodeValueString = ESString::create("nodeValue");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(), nodeValueString,
        nodeValueGetterFunction, nodeValueSetterFunction);

    ESString* textContentString = ESString::create("textContent");
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        NodeFunction->protoType().asESPointer()->asESObject(),
        textContentString, textContentGetterFunction,
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
