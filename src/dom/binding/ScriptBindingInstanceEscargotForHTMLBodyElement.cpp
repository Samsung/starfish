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

namespace StarFish {

using namespace escargot;

static ESValue onLoadGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement() &&
        nd->asElement()->asHTMLElement()->isHTMLBodyElement()) {
        auto element = nd->asElement()->asHTMLElement()->asHTMLBodyElement();
        auto eventType =
            element->document()->window()->starFish()->staticStrings()->m_load;
        return element->document()->window()->attributeEventListener(eventType);
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue onLoadSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement() &&
        nd->asElement()->asHTMLElement()->isHTMLBodyElement()) {
        auto element = nd->asElement()->asHTMLElement()->asHTMLBodyElement();
        auto eventType =
            element->document()->window()->starFish()->staticStrings()->m_load;
        if (v.isObject() ||
            (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
            element->document()->window()->setAttributeEventListener(eventType,
                                                                     v);
        } else {
            element->document()->window()->clearAttributeEventListener(
                eventType);
        }
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
    return ESValue();
}

static ESValue onUnLoadGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement() &&
        nd->asElement()->asHTMLElement()->isHTMLBodyElement()) {
        auto element = nd->asElement()->asHTMLElement()->asHTMLBodyElement();
        auto eventType = element->document()
                             ->window()
                             ->starFish()
                             ->staticStrings()
                             ->m_unload;
        return element->document()->window()->attributeEventListener(eventType);
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue onUnLoadSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject, Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement() &&
        nd->asElement()->asHTMLElement()->isHTMLBodyElement()) {
        auto element = nd->asElement()->asHTMLElement()->asHTMLBodyElement();
        auto eventType = element->document()
                             ->window()
                             ->starFish()
                             ->staticStrings()
                             ->m_unload;
        if (v.isObject() ||
            (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
            element->document()->window()->setAttributeEventListener(eventType,
                                                                     v);
        } else {
            element->document()->window()->clearAttributeEventListener(
                eventType);
        }
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
    return ESValue();
}

ESFunctionObject* bindingHTMLBodyElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        HTMLBodyElement, fetchData(scriptBindingInstance)->htmlElement());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLBodyElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onload"), onLoadGetterFunction, onLoadSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLBodyElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onunload"), onUnLoadGetterFunction,
        onUnLoadSetterFunction);

    return HTMLBodyElementFunction;
}
}
