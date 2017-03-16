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

ESFunctionObject* bindingHTMLScriptElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        HTMLScriptElement, fetchData(scriptBindingInstance)->htmlElement());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLScriptElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("src"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement() &&
                nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
                size_t idx = nd->asElement()->hasAttribute(nd->document()
                                                               ->window()
                                                               ->starFish()
                                                               ->staticStrings()
                                                               ->m_src);
                if (idx != SIZE_MAX) {
                    return toJSString(URL::getURLString(
                        nd->document()->documentURI()->urlString(),
                        nd->asElement()->getAttribute(idx)));
                }
                return toJSString(String::emptyString);
            }
            THROW_ILLEGAL_INVOCATION();
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement() &&
                nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
                nd->asElement()->setAttribute(nd->document()
                                                  ->window()
                                                  ->starFish()
                                                  ->staticStrings()
                                                  ->m_src,
                                              toBrowserString(v.toString()));
                return ESValue();
            }
            THROW_ILLEGAL_INVOCATION();
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLScriptElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("type"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement() &&
                nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
                return toJSString(
                    nd->asElement()->getAttribute(nd->document()
                                                      ->window()
                                                      ->starFish()
                                                      ->staticStrings()
                                                      ->m_type));
            }
            THROW_ILLEGAL_INVOCATION();
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement() &&
                nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
                nd->asElement()->setAttribute(nd->document()
                                                  ->window()
                                                  ->starFish()
                                                  ->staticStrings()
                                                  ->m_type,
                                              toBrowserString(v.toString()));
                return ESValue();
            }
            THROW_ILLEGAL_INVOCATION();
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLScriptElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("charset"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement() &&
                nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
                return toJSString(
                    nd->asElement()->getAttribute(nd->document()
                                                      ->window()
                                                      ->starFish()
                                                      ->staticStrings()
                                                      ->m_charset));
            }
            THROW_ILLEGAL_INVOCATION();
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement() &&
                nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
                nd->asElement()->setAttribute(nd->document()
                                                  ->window()
                                                  ->starFish()
                                                  ->staticStrings()
                                                  ->m_charset,
                                              toBrowserString(v.toString()));
                return ESValue();
            }
            THROW_ILLEGAL_INVOCATION();
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLScriptElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("text"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement() &&
                nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
                return toJSString(nd->asElement()
                                      ->asHTMLElement()
                                      ->asHTMLScriptElement()
                                      ->text());
            }
            THROW_ILLEGAL_INVOCATION();
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement() &&
                nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
                nd->asElement()
                    ->asHTMLElement()
                    ->asHTMLScriptElement()
                    ->setText(toBrowserString(v.toString()));
                return ESValue();
            }
            THROW_ILLEGAL_INVOCATION();
            return ESValue();
        });

    // TODO : Implement setter
    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLScriptElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("charset"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement() &&
                nd->asElement()->asHTMLElement()->isHTMLScriptElement()) {
                QualifiedName name = QualifiedName(
                    AtomicString::emptyAtomicString(),
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
        },
        nullptr);

    return HTMLScriptElementFunction;
}
}
