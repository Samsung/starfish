/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "binding/ScriptEngineInstance.h"
#include "core/page/BrowsingContext.h"
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
        starFish->locale().getName(),
        starFish->timezoneID()->toUTF8NonGCString().data());
    m_engineInstance->setNewPromiseJobListener([](ExecutionStateRef* state,
                                                  JobRef* job) {
        Window* window = (Window*)state->context()->globalObject()->extraData();
        window->starFish()->messageLoop()->addIdler(
            window->browsingContext(),
            [](size_t, void* data, void* data2) {
                Window* window = (Window*)data;

                if (!window->browsingContext()->isActive()) {
                    return;
                }

                JobRef* job = (JobRef*)data2;
                auto sbresult = job->run();

                if (!sbresult.error->isEmpty()) {
                    STARFISH_LOG_ERROR(
                        "Uncaught %s\n",
                        toBrowserString(window->scriptBindingInstance(),
                                        ValueRef::create(sbresult.error))
                            ->toUTF8NonGCString()
                            .data());
                }
            },
            window, job);
    });
}

void ScriptEngineInstance::dispose()
{
    m_engineInstance->destroy();
}
}
