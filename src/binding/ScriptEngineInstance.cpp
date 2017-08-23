/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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
#include "StarFish.h"
#include "binding/ScriptEngineInstance.h"
#include "core/page/Window.h"
#include "core/modules/message_loop/MessageLoop.h"

#include <EscargotPublic.h>
using namespace Escargot;

namespace StarFish {

ScriptEngineInstance::ScriptEngineInstance(StarFish* starFish)
    : StarFishHoldable(starFish)
{
    // Set this flag to process const keyword temporary
    setenv("ESCARGOT_TREAT_CONST_AS_VAR", "1", 1);

    Escargot::Globals::initialize();
    m_engineInstance = VMInstanceRef::create(
        starFish->locale().getName(), starFish->timezoneID()->utf8Data());
    m_engineInstance->setNewPromiseJobListener([](ExecutionStateRef* state,
                                                  JobRef* job) {
        Window* window = (Window*)state->context()->globalObject()->extraData();
        window->starFish()->messageLoop()->addIdler(
            window->browsingContext(),
            [](size_t, void* data, void* data2) {
                Window* window = (Window*)data;
                ExecutionStateRef* state = ExecutionStateRef::create(
                    window->scriptBindingInstance()->scriptContext());
                JobRef* job = (JobRef*)data2;
                job->run(state);
                state->destroy();
            },
            window, job);
    });
}

void ScriptEngineInstance::close()
{
    m_engineInstance->destroy();
}
}
