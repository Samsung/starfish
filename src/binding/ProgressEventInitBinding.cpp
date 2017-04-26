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
#include "dom/ProgressEvent.h"

namespace StarFish {

using namespace escargot;

ProgressEventInit toProgressEventInitFromESValue(ESVMInstance* instance,
                                                 ESValue& from)
{
    if (!from.isObject()) {
        auto msg = ESString::create(
            "Failed to generate ProgressEventInit from non-object");
        instance->throwError(ESValue(TypeError::create(msg)));
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    ESValue arg0 = from.asESPointer()->asESObject()->get(
        ESString::create("lengthComputable"));
    ESValue arg1 =
        from.asESPointer()->asESObject()->get(ESString::create("loaded"));
    ESValue arg2 =
        from.asESPointer()->asESObject()->get(ESString::create("total"));
    ESValue arg3 =
        from.asESPointer()->asESObject()->get(ESString::create("bubbles"));
    ESValue arg4 =
        from.asESPointer()->asESObject()->get(ESString::create("cancelable"));
    ESValue arg5 =
        from.asESPointer()->asESObject()->get(ESString::create("composed"));
    ProgressEventInit result;
    // Handle argument arg0
    bool value0;
    value0 = arg0.toBoolean();

    result.setLengthComputable(value0);
    // Handle argument arg1
    uint64_t value1;
    value1 = arg1.toNumber();

    result.setLoaded(value1);
    // Handle argument arg2
    uint64_t value2;
    value2 = arg2.toNumber();

    result.setTotal(value2);
    // Handle argument arg3
    bool value3 = false;
    if (!arg3.isUndefinedOrNull()) {
        value3 = arg3.toBoolean();
    }
    result.setBubbles(value3);
    // Handle argument arg4
    bool value4 = false;
    if (!arg4.isUndefinedOrNull()) {
        value4 = arg4.toBoolean();
    }
    result.setCancelable(value4);
    // Handle argument arg5
    bool value5 = false;
    if (!arg5.isUndefinedOrNull()) {
        value5 = arg5.toBoolean();
    }
    result.setComposed(value5);
    return result;
}

ESValue toESValueFromProgressEventInit(ESVMInstance* instance,
                                       ProgressEventInit& from)
{
    ESObject* result = ESObject::create();
    // Declare native value (empty when type is void)
    bool value0;
    value0 = from.lengthComputable();
    result->set(ESString::create("lengthComputable"), ESValue(value0));
    // Declare native value (empty when type is void)
    uint64_t value1;
    value1 = from.loaded();
    result->set(ESString::create("loaded"), ESValue(value1));
    // Declare native value (empty when type is void)
    uint64_t value2;
    value2 = from.total();
    result->set(ESString::create("total"), ESValue(value2));
    // Declare native value (empty when type is void)
    bool value3;
    value3 = from.bubbles();
    result->set(ESString::create("bubbles"), ESValue(value3));
    // Declare native value (empty when type is void)
    bool value4;
    value4 = from.cancelable();
    result->set(ESString::create("cancelable"), ESValue(value4));
    // Declare native value (empty when type is void)
    bool value5;
    value5 = from.composed();
    result->set(ESString::create("composed"), ESValue(value5));
    return ESValue(result);
}
}
