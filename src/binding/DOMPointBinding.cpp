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
#include "binding/escargot/ScriptBindingInstanceDataEscargot.h"

#include "dom/DOMPoint.h"
#include "dom/DOMException.h"

namespace StarFish {

using namespace escargot;

static ESValue domPointFunction(ESVMInstance* instance)
{
    int cnt = instance->currentExecutionContext()->argumentCount();
    if (cnt == 0) {
        DOMPoint* point = new DOMPoint();
        return point->scriptValue();
    } else if (cnt == 1) {
        DOMPoint* point = nullptr;
        ESValue arg = instance->currentExecutionContext()->readArgument(0);
        if (arg.isUndefinedOrNull()) {
            point = new DOMPoint();
        } else if (arg.isObject()) {
            DOMPointInit initPoint;
            ESValue value;
            value = arg.asESPointer()->asESObject()->get(ESString::create("x"));
            if (!value.isUndefined()) {
                initPoint.x = value.toNumber();
            }
            value = arg.asESPointer()->asESObject()->get(ESString::create("y"));
            if (!value.isUndefined()) {
                initPoint.y = value.toNumber();
            }
            value = arg.asESPointer()->asESObject()->get(ESString::create("z"));
            if (!value.isUndefined()) {
                initPoint.z = value.toNumber();
            }
            value = arg.asESPointer()->asESObject()->get(ESString::create("w"));
            if (!value.isUndefined()) {
                initPoint.w = value.toNumber();
            }
            point = new DOMPoint(initPoint);
        }
        return point->scriptValue();
    } else {
        (cnt > 4) ? cnt = 4 : cnt;
        DOMPoint* point = nullptr;
        double args[4] = { 0, 0, 0, 1 }; // x, y, z, w
        for (int i = 0; i < cnt; ++i) {
            ESValue value =
                instance->currentExecutionContext()->readArgument(i);
            if (!value.isUndefined()) {
                args[i] = value.toNumber();
            }
        }
        point = new DOMPoint(args[0], args[1], args[2], args[3]);
        return point->scriptValue();
    }
}

static ESValue xGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPoint);
    DOMPoint* point = originalObj;
    return ESValue(point->x());
}

static ESValue xSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPoint);
    DOMPoint* point = originalObj;
    ESValue v = instance->currentExecutionContext()->readArgument(0);
    point->setX(v.toNumber());
    return ESValue();
}

static ESValue yGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPoint);
    DOMPoint* point = originalObj;
    return ESValue(point->y());
}

static ESValue ySetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPoint);
    DOMPoint* point = originalObj;
    ESValue v = instance->currentExecutionContext()->readArgument(0);
    point->setY(v.toNumber());
    return ESValue();
}

static ESValue zGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPoint);
    DOMPoint* point = originalObj;
    return ESValue(point->z());
}

static ESValue zSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPoint);
    DOMPoint* point = originalObj;
    ESValue v = instance->currentExecutionContext()->readArgument(0);
    point->setZ(v.toNumber());
    return ESValue();
}

static ESValue wGetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPoint);
    DOMPoint* point = originalObj;
    return ESValue(point->w());
}

static ESValue wSetterFunction(ESVMInstance* instance)
{
    GENERATE_THIS_AND_CHECK_TYPE(DOMPoint);
    DOMPoint* point = originalObj;
    ESValue v = instance->currentExecutionContext()->readArgument(0);
    point->setW(v.toNumber());
    return ESValue();
}

ESFunctionObject* bindingDOMPoint(ScriptBindingInstance* scriptBindingInstance)
{
    ESString* DOMPointString = ESString::create("DOMPoint");
    auto fnDomPoint = ESFunctionObject::create(NULL, domPointFunction,
                                               DOMPointString, 0, true, true);

    fnDomPoint->defineAccessorProperty(
        ESVMInstance::currentInstance()->strings().prototype.string(),
        ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false,
        false, false);
    fnDomPoint->protoType()
        .asESPointer()
        ->asESObject()
        ->forceNonVectorHiddenClass(false);
    fnDomPoint->protoType().asESPointer()->asESObject()->set__proto__(
        fetchData(scriptBindingInstance)->fnDOMPointReadOnly()->protoType());
    fnDomPoint->set__proto__(
        fetchData(scriptBindingInstance)->fnDOMPointReadOnly());

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnDomPoint->protoType().asESPointer()->asESObject(),
        ESString::create("x"), xGetterFunction, xSetterFunction, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnDomPoint->protoType().asESPointer()->asESObject(),
        ESString::create("y"), yGetterFunction, ySetterFunction, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnDomPoint->protoType().asESPointer()->asESObject(),
        ESString::create("z"), zGetterFunction, zSetterFunction, true, true);

    defineNativeAccessorPropertyButNeedToGenerateJSFunction(
        fnDomPoint->protoType().asESPointer()->asESObject(),
        ESString::create("w"), wGetterFunction, wSetterFunction, true, true);

    return fnDomPoint;
}
}
