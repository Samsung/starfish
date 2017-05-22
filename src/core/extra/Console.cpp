/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "StarFish.h"
#include "core/extra/Console.h"
#include "core/inspector/Inspector.h"

namespace StarFish {

Console::Console(StarFish* starFish)
    : m_starFish(starFish)
{
}

void Console::log(String* m)
{
#if defined(STARFISH_ENABLE_INSPECTOR)
    if (m_starFish->inspector()) {
        m_starFish->inspector()->sendInfoMessage(m);
    }
#endif
    STARFISH_LOG_INFO("console.log: %s\n", m->utf8Data());
}

void Console::info(String* m)
{
#if defined(STARFISH_ENABLE_INSPECTOR)
    if (m_starFish->inspector()) {
        m_starFish->inspector()->sendInfoMessage(m);
    }
#endif
    STARFISH_LOG_INFO("console.info: %s\n", m->utf8Data());
}

void Console::error(String* m)
{
#if defined(STARFISH_ENABLE_INSPECTOR)
    if (m_starFish->inspector()) {
        m_starFish->inspector()->sendErrorMessage(m);
    }
#endif
    STARFISH_LOG_ERROR("console.error: %s\n", m->utf8Data());
}

void Console::warn(String* m)
{
#if defined(STARFISH_ENABLE_INSPECTOR)
    if (m_starFish->inspector()) {
        m_starFish->inspector()->sendWarnMessage(m);
    }
#endif
    STARFISH_LOG_ERROR("console.warn: %s\n", m->utf8Data());
}
}
