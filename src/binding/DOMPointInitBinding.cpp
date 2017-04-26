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
#include "dom/DOMPoint.h"

namespace StarFish {

using namespace escargot;

DOMPointInit toDOMPointInitFromESValue(ESVMInstance* instance, ESValue& from)
{
    if (!from.isObject()) {
        auto msg =
            ESString::create("Failed to generate DOMPointInit from non-object");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    ESValue arg0 = from.asESPointer()->asESObject()->get(ESString::create("x"));
    ESValue arg1 = from.asESPointer()->asESObject()->get(ESString::create("y"));
    ESValue arg2 = from.asESPointer()->asESObject()->get(ESString::create("z"));
    ESValue arg3 = from.asESPointer()->asESObject()->get(ESString::create("w"));
    DOMPointInit result;
    // Handle argument arg0
    double value0 = 0;
    if (!arg0.isUndefinedOrNull()) {
        value0 = arg0.toNumber();
    }
    result.setX(value0);
    // Handle argument arg1
    double value1 = 0;
    if (!arg1.isUndefinedOrNull()) {
        value1 = arg1.toNumber();
    }
    result.setY(value1);
    // Handle argument arg2
    double value2 = 0;
    if (!arg2.isUndefinedOrNull()) {
        value2 = arg2.toNumber();
    }
    result.setZ(value2);
    // Handle argument arg3
    double value3 = 1;
    if (!arg3.isUndefinedOrNull()) {
        value3 = arg3.toNumber();
    }
    result.setW(value3);
    return result;
}

ESValue toESValueFromDOMPointInit(ESVMInstance* instance, DOMPointInit& from)
{
    ESObject* result = ESObject::create();
    // Declare native value (empty when type is void)
    double value0;
    value0 = from.x();
    result->set(ESString::create("x"), ESValue(value0));
    // Declare native value (empty when type is void)
    double value1;
    value1 = from.y();
    result->set(ESString::create("y"), ESValue(value1));
    // Declare native value (empty when type is void)
    double value2;
    value2 = from.z();
    result->set(ESString::create("z"), ESValue(value2));
    // Declare native value (empty when type is void)
    double value3;
    value3 = from.w();
    result->set(ESString::create("w"), ESValue(value3));
    return ESValue(result);
}
}
