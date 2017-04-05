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

static ESValue domRectFunction(ESVMInstance* instance)
{
    int cnt = instance->currentExecutionContext()->argumentCount();
    (cnt > 4) ? cnt = 4 : cnt;
    DOMRect* rect = nullptr;
    double args[4] = {
        0,
    };
    if (cnt == 0) {
        rect = DOMRect::create();
    } else {
        for (int i = 0; i < cnt; ++i) {
            args[i] =
                instance->currentExecutionContext()->readArgument(i).toNumber();
        }
        if (cnt == 1) {
            rect = DOMRect::create(args[0]);
        } else if (cnt == 2) {
            rect = DOMRect::create(args[0], args[1]);
        } else if (cnt == 3) {
            rect = DOMRect::create(args[0], args[1], args[2]);
        } else {
            rect = DOMRect::create(args[0], args[1], args[2], args[3]);
        }
    }
    return rect->scriptValue();
}

static ESValue xGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRect);
    DOMRect* rect = originalObj;
    return ESValue(rect->x());
}

static ESValue xSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRect);
    DOMRect* rect = originalObj;
    if (rect != nullptr) {
        rect->setX(v.toNumber());
        return ESValue();
    }
    THROW_ILLEGAL_INVOCATION();
    return ESValue();
}

static ESValue yGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRect);
    DOMRect* rect = originalObj;
    return ESValue(rect->y());
}

static ESValue ySetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRect);
    DOMRect* rect = originalObj;
    if (rect != nullptr) {
        rect->setY(v.toNumber());
        return ESValue();
    }
    THROW_ILLEGAL_INVOCATION();
    return ESValue();
}

static ESValue widthGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRect);
    DOMRect* rect = originalObj;
    return ESValue(rect->width());
}

static ESValue widthSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRect);
    DOMRect* rect = originalObj;
    if (rect != nullptr) {
        rect->setWidth(v.toNumber());
        return ESValue();
    }
    THROW_ILLEGAL_INVOCATION();
    return ESValue();
}

static ESValue heightGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRect);
    DOMRect* rect = originalObj;
    return ESValue(rect->height());
}

static ESValue heightSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMRect);
    DOMRect* rect = originalObj;
    if (rect != nullptr) {
        rect->setHeight(v.toNumber());
        return ESValue();
    }
    THROW_ILLEGAL_INVOCATION();
    return ESValue();
}

ESFunctionObject* bindingDOMRect(ScriptBindingInstance* scriptBindingInstance)
{
    ESString* DOMRectString = ESString::create("DOMRect");
    auto fnDOMRect = ESFunctionObject::create(NULL, domRectFunction,
                                              DOMRectString, 0, true, true);

    fnDOMRect->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    fnDOMRect->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    fnDOMRect->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)->domRectReadOnly()->protoType());
    fnDOMRect->set__proto__(
        fetchData(scriptBindingInstance)->domRectReadOnly());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnDOMRect->protoType().asESPointer()->asESObject(),
        ESString::create("x"), xGetterFunction, xSetterFunction, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnDOMRect->protoType().asESPointer()->asESObject(),
        ESString::create("y"), yGetterFunction, ySetterFunction, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnDOMRect->protoType().asESPointer()->asESObject(),
        ESString::create("width"), widthGetterFunction, widthSetterFunction,
        true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnDOMRect->protoType().asESPointer()->asESObject(),
        ESString::create("height"), heightGetterFunction, heightSetterFunction,
        true, true);

    return fnDOMRect;
}
}
