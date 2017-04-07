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

namespace StarFish {

using namespace escargot;

static ESValue srcGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement() &&
        nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
        size_t idx = nd->asElement()->hasAttribute(
            nd->document()->window()->starFish()->staticStrings()->m_src);
        if (idx != SIZE_MAX) {
            return toJSString(
                URL::getURLString(nd->document()->documentURI()->urlString(),
                                  nd->asElement()->getAttribute(idx)));
        }
        return toJSString(String::emptyString);
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue srcSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement() &&
        nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        nd->asElement()->setAttribute(
            nd->document()->window()->starFish()->staticStrings()->m_src,
            toBrowserString(v.toString()));
        return ESValue();
    }
    THROW_ILLEGAL_INVOCATION();
    return ESValue();
}

static ESValue typeGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement() &&
        nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
        return toJSString(nd->asElement()->getAttribute(
            nd->document()->window()->starFish()->staticStrings()->m_type));
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue typeSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement() &&
        nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        nd->asElement()->setAttribute(
            nd->document()->window()->starFish()->staticStrings()->m_type,
            toBrowserString(v.toString()));
        return ESValue();
    }
    THROW_ILLEGAL_INVOCATION();
    return ESValue();
}

static ESValue charsetGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement() &&
        nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
        QualifiedName name =
            QualifiedName(AtomicString::emptyAtomicString(),
                          AtomicString::createAttrAtomicString(
                              nd->document()->window()->starFish(), "charset"));
        size_t idx = nd->asElement()->hasAttribute(name);
        if (idx == SIZE_MAX) {
            return ESString::create("");
        } else {
            String* value = nd->asElement()->getAttribute(idx);
            // STARFISH_ASSERT(value.equals("UTF-8"));
            return toJSString(value);
        }
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue textGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement() &&
        nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
        return toJSString(
            nd->asElement()->asHTMLElement()->asHTMLScriptElement()->text());
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue textSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement() &&
        nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        nd->asElement()->asHTMLElement()->asHTMLScriptElement()->setText(
            toBrowserString(v.toString()));
        return ESValue();
    }
    THROW_ILLEGAL_INVOCATION();
    return ESValue();
}

ESFunctionObject* bindingHTMLScriptElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        HTMLScriptElement, fetchData(scriptBindingInstance)->fnHTMLElement());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLScriptElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("src"), srcGetterFunction, srcSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLScriptElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("type"), typeGetterFunction, typeSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLScriptElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("text"), textGetterFunction, textSetterFunction);

    // TODO : Implement setter
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLScriptElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("charset"), charsetGetterFunction, nullptr);

    return HTMLScriptElementFunction;
}
}
