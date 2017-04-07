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
    if (nd->isElement()) {
        if (nd->asElement()->isHTMLElement()) {
            if (nd->asElement()->asHTMLElement()->isHTMLImageElement()) {
                return toJSString(nd->asElement()
                                      ->asHTMLElement()
                                      ->asHTMLImageElement()
                                      ->src());
            }
        }
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue srcSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement()) {
        if (nd->asElement()->isHTMLElement()) {
            if (nd->asElement()->asHTMLElement()->isHTMLImageElement()) {
                ESValue v =
                    instance->currentExecutionContext()->readArgument(0);
                nd->asElement()->asHTMLElement()->asHTMLImageElement()->setSrc(
                    toBrowserString(v));
                return ESValue();
            }
        }
    }
    return ESValue();
}

static ESValue widthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement()) {
        if (nd->asElement()->isHTMLElement()) {
            if (nd->asElement()->asHTMLElement()->isHTMLImageElement()) {
                String* width = nd->asElement()
                                    ->asHTMLElement()
                                    ->asHTMLImageElement()
                                    ->width();
                return ESValue(String::parseInt(width));
            }
        }
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue widthSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;

    ESValue v = instance->currentExecutionContext()->readArgument(0);
    if (!(v.isESString() && v.asESString()->hasOnlyDigit())) {
        return ESValue();
    }

    if (nd->isElement()) {
        if (nd->asElement()->isHTMLElement()) {
            if (nd->asElement()->asHTMLElement()->isHTMLImageElement()) {
                nd->asElement()
                    ->asHTMLElement()
                    ->asHTMLImageElement()
                    ->setWidth(v.toInt32());
                return ESValue();
            }
        }
    }
    THROW_ILLEGAL_INVOCATION();
    return ESValue();
}

static ESValue heightGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;
    if (nd->isElement()) {
        if (nd->asElement()->isHTMLElement()) {
            if (nd->asElement()->asHTMLElement()->isHTMLImageElement()) {
                String* height = nd->asElement()
                                     ->asHTMLElement()
                                     ->asHTMLImageElement()
                                     ->height();
                return ESValue(String::parseInt(height));
            }
        }
    }
    THROW_ILLEGAL_INVOCATION();
}

static ESValue heightSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(Node);
    Node* nd = originalObj;

    ESValue v = instance->currentExecutionContext()->readArgument(0);
    if (!(v.isESString() && v.asESString()->hasOnlyDigit())) {
        return ESValue();
    }

    if (nd->isElement()) {
        if (nd->asElement()->isHTMLElement()) {
            if (nd->asElement()->asHTMLElement()->isHTMLImageElement()) {
                nd->asElement()
                    ->asHTMLElement()
                    ->asHTMLImageElement()
                    ->setHeight(v.toInt32());
                return ESValue();
            }
        }
    }
    THROW_ILLEGAL_INVOCATION();
    return ESValue();
}

ESFunctionObject* bindingHTMLImageElement(
    ScriptBindingInstance* scriptBindingInstance)
{
    DEFINE_FUNCTION_NOT_CONSTRUCTOR_WITH_PARENTFUNC(
        HTMLImageElement, fetchData(scriptBindingInstance)->fnHTMLElement());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLImageElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("src"), srcGetterFunction, srcSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLImageElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("width"), widthGetterFunction, widthSetterFunction);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        HTMLImageElementFunction->protoType().asESPointer()->asESObject(),
        ESString::create("height"), heightGetterFunction, heightSetterFunction);
    return HTMLImageElementFunction;
}
}
