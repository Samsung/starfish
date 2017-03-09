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
#include "platform/window/Window.h"
#include "animation/Animation.h"

namespace StarFish {

void AnimationTask::execute()
{
    // TODO
    // STARFISH_ASSERT_NOT_REACHED();
}

void AnimationExecutor::registerAnimation(AnimationTask* newTask)
{
    // TODO
    // m_animationList.push_back(newTask);
}

void AnimationExecutor::cancelAnimation()
{
    // TODO
}

void AnimationExecutor::startIfNeeds()
{
    if (m_isAlive || m_platformAnimator) {
        return;
    }

    m_isAlive = true;
    m_platformAnimator = ecore_animator_add(
        [](void* user_data) -> Eina_Bool {
            AnimationExecutor* executor = (AnimationExecutor*)user_data;
            if (executor->isAlive()) {
                executor->step();
                return ECORE_CALLBACK_RENEW;
            }
            return ECORE_CALLBACK_CANCEL;
        },
        this);
}

void AnimationExecutor::stopIfNeeds()
{
    if (!m_isAlive) {
        return;
    }
    m_isAlive = false;
    if (m_platformAnimator) {
        ecore_animator_del(m_platformAnimator);
    }
}

void AnimationExecutor::step()
{
    STARFISH_ASSERT(m_isAlive);
    if (m_animationList.size() == 0) {
        m_isAlive = false;
        return;
    }
    for (size_t i = 0; i < m_animationList.size(); i++) {
        m_animationList[i]->execute();
    }
}
}
