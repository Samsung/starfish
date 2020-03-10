/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */
#if defined(STARFISH_WEBWORKER_HOST)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/worker/host/WorkerGlobalScope.h"
#include "core/dom/ExecutionContext.h"

#include <EscargotPublic.h>
using namespace Escargot;

namespace Starfish {

struct TimeOutData : public gc {
    TimeOutData(GlobalScope* globalScope)
        : listener(nullptr)
        , globalScope(globalScope)
    {
        STARFISH_ASSERT(globalScope != nullptr);
    }
    void* listener;
    GCVector<ScriptValue> argVector;
    GlobalScope* globalScope;
};

static void timeoutHandler(void* data)
{
    STARFISH_ASSERT(data != nullptr);
    TimeOutData* td = (TimeOutData*)data;
    FunctionObjectRef* fn = (FunctionObjectRef*)td->listener;
    ScriptBindingInstance* instance =
        td->globalScope->executionContext()->scriptBindingInstance();

    callScriptFunction(instance, ValueRef::create(fn), td->argVector.data(),
                       td->argVector.size(), scriptUndefined());
}

ValueRef* setTimeoutWorkerGlobalScopeFunction(ExecutionStateRef* state,
                                              ValueRef* thisValue, size_t argc,
                                              NULLABLE ValueRef** argv,
                                              bool isNewExpression)
{
    STARFISH_ASSERT(state != nullptr);
    STARFISH_ASSERT(thisValue != nullptr);
    GENERATE_WORKERGLOBALSCOPE();
    size_t argCount = argc;
    if (argCount < 1) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "1", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "setTimeout",
                        "WorkerGlobalScope", reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    int32_t result;
    TimeOutData* td = new TimeOutData(originalObj);
    ValueRef* arg1 = (argc > 1) ? argv[1] : ValueRef::createUndefined();

    // Handle ellipsis arguments from index2
    for (size_t i = 2; i < argCount; i++) {
        td->argVector.push_back(argv[i]);
    }
    // Handle argument arg1
    STARFISH_ASSERT(arg1 != nullptr);
    int32_t value1 = 0;
    if (!arg1->isUndefinedOrNull()) {
        value1 = arg1->toInt32(state);
    }
    // Handle argument arg0
    STARFISH_ASSERT(argv[0] != nullptr);
    if (argv[0]->isFunction()) {
        td->listener = argv[0]->asObject();
    } else {
        String* bodyStr = toBrowserString(state, argv[0]);
        String* name[] = { String::emptyString };
        bool error = false;
        td->listener = createScriptFunction(
            originalObj->scriptBindingInstance(), name, 1, bodyStr, error);
    }

    // Call native function (nargs: 3)
    result = originalObj->setTimeout(timeoutHandler, value1, td);

    // Return ValueRef* from native value
    return ValueRef::create(result);
}

ValueRef* setIntervalWorkerGlobalScopeFunction(ExecutionStateRef* state,
                                               ValueRef* thisValue, size_t argc,
                                               NULLABLE ValueRef** argv,
                                               bool isNewExpression)
{
    STARFISH_ASSERT(state != nullptr);
    STARFISH_ASSERT(thisValue != nullptr);
    GENERATE_WORKERGLOBALSCOPE();
    size_t argCount = argc;
    if (argCount < 2) {
        char buffer[2];
        snprintf(buffer, 2, "%zu", argCount);
        COMPOSE_MESSAGE(reason, ARGS_NOT_ENOUGH, "2", buffer);
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "setInterval",
                        "WorkerGlobalScope", reason);
        THROW_EXCEPTION(msg);
    }
    // Declare native value (empty when type is void)
    int32_t result;
    TimeOutData* td = new TimeOutData(originalObj);
    ValueRef* arg1 = (argc > 1) ? argv[1] : ValueRef::createUndefined();

    // Handle ellipsis arguments from index2
    for (size_t i = 2; i < argCount; i++) {
        td->argVector.push_back(argv[i]);
    }
    // Handle argument arg1
    STARFISH_ASSERT(arg1 != nullptr);
    int32_t value1 = 0;
    if (!arg1->isUndefinedOrNull()) {
        value1 = arg1->toInt32(state);
    }
    // Handle argument arg0
    STARFISH_ASSERT(argv[0] != nullptr);
    if (argv[0]->isFunction()) {
        td->listener = argv[0]->asObject();
    } else {
        String* bodyStr = toBrowserString(state, argv[0]);
        String* name[] = { String::emptyString };
        bool error = false;
        td->listener = createScriptFunction(
            originalObj->scriptBindingInstance(), name, 1, bodyStr, error);
    }

    // Call native function (nargs: 3)
    result = originalObj->setInterval(timeoutHandler, value1, td);

    // Return ValueRef* from native value
    return ValueRef::create(result);
}
}

#endif
