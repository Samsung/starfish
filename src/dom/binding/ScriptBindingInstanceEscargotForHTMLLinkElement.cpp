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

ESFunctionObject* bindingHTMLLinkElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        HTMLLinkElement, fetchData(scriptBindingInstance)->htmlElement());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLLinkElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("href"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement() &&
                nd->asElement()->asHTMLElement()->isHTMLLinkElement()) {
                size_t idx = nd->asElement()->hasAttribute(nd->document()
                                                               ->window()
                                                               ->starFish()
                                                               ->staticStrings()
                                                               ->m_href);
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
                nd->asElement()->asHTMLElement()->isHTMLLinkElement()) {
                nd->asElement()->setAttribute(nd->document()
                                                  ->window()
                                                  ->starFish()
                                                  ->staticStrings()
                                                  ->m_href,
                                              toBrowserString(v.toString()));
                return ESValue();
            }
            THROW_ILLEGAL_INVOCATION();
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLLinkElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("rel"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement() &&
                nd->asElement()->asHTMLElement()->isHTMLLinkElement()) {
                return toJSString(
                    nd->asElement()->getAttribute(nd->document()
                                                      ->window()
                                                      ->starFish()
                                                      ->staticStrings()
                                                      ->m_rel));
            }
            THROW_ILLEGAL_INVOCATION();
        },
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement() &&
                nd->asElement()->asHTMLElement()->isHTMLLinkElement()) {
                nd->asElement()->setAttribute(nd->document()
                                                  ->window()
                                                  ->starFish()
                                                  ->staticStrings()
                                                  ->m_rel,
                                              toBrowserString(v.toString()));
                return ESValue();
            }
            THROW_ILLEGAL_INVOCATION();
            return ESValue();
        });

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLLinkElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("type"),
        [](ESVMInstance* instance) -> ESValue {
            GENERATE_THIS_AND_CHECK_TYPE(ScriptWrappable::Type::NodeObject,
                                         Node);
            Node* nd = originalObj;
            if (nd->isElement() && nd->asElement()->isHTMLElement() &&
                nd->asElement()->asHTMLElement()->isHTMLLinkElement()) {
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
                nd->asElement()->asHTMLElement()->isHTMLLinkElement()) {
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

    return HTMLLinkElementFunction;
}
}
