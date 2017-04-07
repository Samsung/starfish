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

static ESValue dirGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        return toJSString(nd->asElement()->getAttribute(
            nd->document()->window()->starFish()->staticStrings()->m_dir));
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue dirSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        nd->asElement()->setAttribute(
            nd->document()->window()->starFish()->staticStrings()->m_dir,
            toBrowserString(v));
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
    return ESValue();
}

static ESValue offsetWidthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        return ESValue(nd->asElement()->asHTMLElement()->offsetWidth());
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue offsetHeightGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        return ESValue(nd->asElement()->asHTMLElement()->offsetHeight());
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue onClickGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        auto element = nd->asElement()->asHTMLElement();
        return element->attributeEventListener(element->document()
                                                   ->window()
                                                   ->starFish()
                                                   ->staticStrings()
                                                   ->m_click);
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue onClickSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        auto element = nd->asElement()->asHTMLElement();
        auto eventType =
            element->document()->window()->starFish()->staticStrings()->m_click;
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        if (v.isObject() ||
            (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
            element->setAttributeEventListener(eventType, v);
        } else {
            element->clearAttributeEventListener(eventType);
        }
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
    return ESValue();
}

static ESValue clickFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* obj = originalObj;
    String* eventType = obj->document()
                            ->window()
                            ->starFish()
                            ->staticStrings()
                            ->m_click.localName();
    Event* e = new Event(eventType, EventInit(true, true));
    obj->dispatchEvent(e);
    return ESValue(ESValue::ESUndefined);
}

static ESValue onMouseOverGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        auto element = nd->asElement()->asHTMLElement();
        return element->attributeEventListener(element->document()
                                                   ->window()
                                                   ->starFish()
                                                   ->staticStrings()
                                                   ->m_mouseover);
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue onMouseOverSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        auto element = nd->asElement()->asHTMLElement();
        auto eventType = element->document()
                             ->window()
                             ->starFish()
                             ->staticStrings()
                             ->m_mouseover;
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        if (v.isObject() ||
            (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
            element->setAttributeEventListener(eventType, v);
        } else {
            element->clearAttributeEventListener(eventType);
        }
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
    return ESValue();
}

static ESValue mouseOverFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* obj = originalObj;
    String* eventType = obj->document()
                            ->window()
                            ->starFish()
                            ->staticStrings()
                            ->m_mouseover.localName();
    Event* e = new Event(eventType, EventInit(true, true));
    obj->dispatchEvent(e);
    return ESValue(ESValue::ESUndefined);
}

static ESValue onLoadGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        auto element = nd->asElement()->asHTMLElement();
        auto eventType =
            element->document()->window()->starFish()->staticStrings()->m_load;
        return element->attributeEventListener(eventType);
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue onLoadSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        auto element = nd->asElement()->asHTMLElement();
        auto eventType =
            element->document()->window()->starFish()->staticStrings()->m_load;
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        if (v.isObject() ||
            (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
            element->setAttributeEventListener(eventType, v);
        } else {
            element->clearAttributeEventListener(eventType);
        }
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
    return ESValue();
}

static ESValue onUnLoadGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        auto element = nd->asElement()->asHTMLElement();
        auto eventType = element->document()
                             ->window()
                             ->starFish()
                             ->staticStrings()
                             ->m_unload;
        return element->attributeEventListener(eventType);
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue onUnLoadSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        auto element = nd->asElement()->asHTMLElement();
        auto eventType = element->document()
                             ->window()
                             ->starFish()
                             ->staticStrings()
                             ->m_unload;
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        if (v.isObject() ||
            (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
            element->setAttributeEventListener(eventType, v);
        } else {
            element->clearAttributeEventListener(eventType);
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
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        auto element = nd->asElement()->asHTMLElement();
        return element->attributeEventListener(element->document()
                                                   ->window()
                                                   ->starFish()
                                                   ->staticStrings()
                                                   ->m_keydown);
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue onKeyDownSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        auto element = nd->asElement()->asHTMLElement();
        auto eventType = element->document()
                             ->window()
                             ->starFish()
                             ->staticStrings()
                             ->m_keydown;
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        if (v.isObject() ||
            (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
            element->setAttributeEventListener(eventType, v);
        } else {
            element->clearAttributeEventListener(eventType);
        }
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
    return ESValue();
}

static ESValue onKeyUpGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        auto element = nd->asElement()->asHTMLElement();
        return element->attributeEventListener(element->document()
                                                   ->window()
                                                   ->starFish()
                                                   ->staticStrings()
                                                   ->m_keyup);
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue onKeyUpSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        auto element = nd->asElement()->asHTMLElement();
        auto eventType =
            element->document()->window()->starFish()->staticStrings()->m_keyup;
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        if (v.isObject() ||
            (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
            element->setAttributeEventListener(eventType, v);
        } else {
            element->clearAttributeEventListener(eventType);
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
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        auto element = nd->asElement()->asHTMLElement();
        return element->attributeEventListener(element->document()
                                                   ->window()
                                                   ->starFish()
                                                   ->staticStrings()
                                                   ->m_focus);
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue onFocusSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        auto element = nd->asElement()->asHTMLElement();
        auto eventType =
            element->document()->window()->starFish()->staticStrings()->m_focus;
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        if (v.isObject() ||
            (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
            element->setAttributeEventListener(eventType, v);
        } else {
            element->clearAttributeEventListener(eventType);
        }
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
    return ESValue();
}

static ESValue focusFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* obj = originalObj;
    String* eventType = obj->document()
                            ->window()
                            ->starFish()
                            ->staticStrings()
                            ->m_focus.localName();
    Event* e = new Event(eventType, EventInit(false, false));
    obj->dispatchEvent(e);
    return ESValue(ESValue::ESUndefined);
}

static ESValue onErrorGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        auto element = nd->asElement()->asHTMLElement();
        return element->attributeEventListener(element->document()
                                                   ->window()
                                                   ->starFish()
                                                   ->staticStrings()
                                                   ->m_error);
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
}

static ESValue onErrorSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement() && nd->asElement()->isHTMLElement()) {
        auto element = nd->asElement()->asHTMLElement();
        auto eventType =
            element->document()->window()->starFish()->staticStrings()->m_error;
        ESValue v = instance->currentExecutionContext()->readArgument(0);
        if (v.isObject() ||
            (v.isESPointer() && v.asESPointer()->isESFunctionObject())) {
            element->setAttributeEventListener(eventType, v);
        } else {
            element->clearAttributeEventListener(eventType);
        }
    } else {
        THROW_ILLEGAL_INVOCATION();
    }
    return ESValue();
}

ESFunctionObject* bindingHTMLElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        HTMLElement, fetchData(scriptBindingInstance)->fnElement());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("dir"), dirGetterFunction, dirSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("offsetWidth"), offsetWidthGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("offsetHeight"), offsetHeightGetterFunction, nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onclick"), onClickGetterFunction,
        onClickSetterFunction);

    HTMLElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("click"), true, true, true,
                             ESFunctionObject::create(NULL, clickFunction,
                                                      ESString::create("click"),
                                                      1, false));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onmouseover"), onMouseOverGetterFunction,
        onMouseOverSetterFunction);

    HTMLElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(
            ESString::create("mouseover"), true, true, true,
            ESFunctionObject::create(NULL, mouseOverFunction,
                                     ESString::create("mouseover"), 1, false));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onload"), onLoadGetterFunction, onLoadSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onunload"), onUnLoadGetterFunction,
        onUnLoadSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onkeydown"), onKeyDownGetterFunction,
        onKeyDownSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onkeyup"), onKeyUpGetterFunction,
        onKeyUpSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onfocus"), onFocusGetterFunction,
        onFocusSetterFunction);

    HTMLElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("focus"), true, true, true,
                             ESFunctionObject::create(NULL, focusFunction,
                                                      ESString::create("focus"),
                                                      1, false));

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onerror"), onErrorGetterFunction,
        onErrorSetterFunction);

    return HTMLElementFunction;
}
}
