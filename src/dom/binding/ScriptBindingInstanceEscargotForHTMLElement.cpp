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

ESFunctionObject* bindingHTMLElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        HTMLElement, fetchData(scriptBindingInstance)->element());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("dir"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement()) {
                return toJSString(
                    nd->asElement()->getAttribute(nd->document()
                                                      ->window()
                                                      ->starFish()
                                                      ->staticStrings()
                                                      ->m_dir));
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement()) {
                nd->asElement()->setAttribute(nd->document()
                                                  ->window()
                                                  ->starFish()
                                                  ->staticStrings()
                                                  ->m_dir,
                                              toBrowserString(v));
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("offsetWidth"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement()) {
                return ESValue(nd->asElement()->asHTMLElement()->offsetWidth());
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("offsetHeight"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement()) {
                return ESValue(
                    nd->asElement()->asHTMLElement()->offsetHeight());
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
        },
        nullptr);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onclick"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
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
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement()) {
                auto element = nd->asElement()->asHTMLElement();
                auto eventType = element->document()
                                     ->window()
                                     ->starFish()
                                     ->staticStrings()
                                     ->m_click;
                if (v.isObject() || (v.isESPointer() &&
                                     v.asESPointer()->isESFunctionObject())) {
                    element->setAttributeEventListener(eventType, v);
                } else {
                    element->clearAttributeEventListener(eventType);
                }
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
            return ESValue();
        });

    ESFunctionObject* clickFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* obj = originalObj;
            String* eventType = obj->document()
                                    ->window()
                                    ->starFish()
                                    ->staticStrings()
                                    ->m_click.localName();
            Event* e = new Event(eventType, EventInit(true, true));
            obj->dispatchEvent(e);
            return ESValue(ESValue::ESUndefined);
        },
        ESString::create("click"), 1, false);
    HTMLElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("click"), true, true, true,
                             clickFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onmouseover"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
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
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement()) {
                auto element = nd->asElement()->asHTMLElement();
                auto eventType = element->document()
                                     ->window()
                                     ->starFish()
                                     ->staticStrings()
                                     ->m_mouseover;
                if (v.isObject() || (v.isESPointer() &&
                                     v.asESPointer()->isESFunctionObject())) {
                    element->setAttributeEventListener(eventType, v);
                } else {
                    element->clearAttributeEventListener(eventType);
                }
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
            return ESValue();
        });

    ESFunctionObject* mouseoverFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* obj = originalObj;
            String* eventType = obj->document()
                                    ->window()
                                    ->starFish()
                                    ->staticStrings()
                                    ->m_mouseover.localName();
            Event* e = new Event(eventType, EventInit(true, true));
            obj->dispatchEvent(e);
            return ESValue(ESValue::ESUndefined);
        },
        ESString::create("mouseover"), 1, false);
    HTMLElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("mouseover"), true, true, true,
                             mouseoverFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onload"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement()) {
                auto element = nd->asElement()->asHTMLElement();
                auto eventType = element->document()
                                     ->window()
                                     ->starFish()
                                     ->staticStrings()
                                     ->m_load;
                return element->attributeEventListener(eventType);
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement()) {
                auto element = nd->asElement()->asHTMLElement();
                auto eventType = element->document()
                                     ->window()
                                     ->starFish()
                                     ->staticStrings()
                                     ->m_load;
                if (v.isObject() || (v.isESPointer() &&
                                     v.asESPointer()->isESFunctionObject())) {
                    element->setAttributeEventListener(eventType, v);
                } else {
                    element->clearAttributeEventListener(eventType);
                }
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onunload"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
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
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement()) {
                auto element = nd->asElement()->asHTMLElement();
                auto eventType = element->document()
                                     ->window()
                                     ->starFish()
                                     ->staticStrings()
                                     ->m_unload;
                if (v.isObject() || (v.isESPointer() &&
                                     v.asESPointer()->isESFunctionObject())) {
                    element->setAttributeEventListener(eventType, v);
                } else {
                    element->clearAttributeEventListener(eventType);
                }
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onkeydown"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
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
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement()) {
                auto element = nd->asElement()->asHTMLElement();
                auto eventType = element->document()
                                     ->window()
                                     ->starFish()
                                     ->staticStrings()
                                     ->m_keydown;
                if (v.isObject() || (v.isESPointer() &&
                                     v.asESPointer()->isESFunctionObject())) {
                    element->setAttributeEventListener(eventType, v);
                } else {
                    element->clearAttributeEventListener(eventType);
                }
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onkeyup"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
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
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement()) {
                auto element = nd->asElement()->asHTMLElement();
                auto eventType = element->document()
                                     ->window()
                                     ->starFish()
                                     ->staticStrings()
                                     ->m_keyup;
                if (v.isObject() || (v.isESPointer() &&
                                     v.asESPointer()->isESFunctionObject())) {
                    element->setAttributeEventListener(eventType, v);
                } else {
                    element->clearAttributeEventListener(eventType);
                }
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onfocus"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
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
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement()) {
                auto element = nd->asElement()->asHTMLElement();
                auto eventType = element->document()
                                     ->window()
                                     ->starFish()
                                     ->staticStrings()
                                     ->m_focus;
                if (v.isObject() || (v.isESPointer() &&
                                     v.asESPointer()->isESFunctionObject())) {
                    element->setAttributeEventListener(eventType, v);
                } else {
                    element->clearAttributeEventListener(eventType);
                }
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
            return ESValue();
        });

    ESFunctionObject* focusFunction = ESFunctionObject::create(
        NULL,
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* obj = originalObj;
            String* eventType = obj->document()
                                    ->window()
                                    ->starFish()
                                    ->staticStrings()
                                    ->m_focus.localName();
            Event* e = new Event(eventType, EventInit(false, false));
            obj->dispatchEvent(e);
            return ESValue(ESValue::ESUndefined);
        },
        ESString::create("focus"), 1, false);
    HTMLElementFunction->protoType()
        .asESPointer()
        ->asESObject()
        ->defineDataProperty(ESString::create("focus"), true, true, true,
                             focusFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("onerror"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
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
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement()) {
                auto element = nd->asElement()->asHTMLElement();
                auto eventType = element->document()
                                     ->window()
                                     ->starFish()
                                     ->staticStrings()
                                     ->m_error;
                if (v.isObject() || (v.isESPointer() &&
                                     v.asESPointer()->isESFunctionObject())) {
                    element->setAttributeEventListener(eventType, v);
                } else {
                    element->clearAttributeEventListener(eventType);
                }
            } else {
                THROW_ILLEGAL_INVOCATION();
            }
            return ESValue();
        });

    return HTMLElementFunction;
}
}
