/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
    STARFISH_LOG_INFO("console.log: %s\n", m->toUTF8NonGCString().c_str());
}

void Console::info(String* m)
{
#if defined(STARFISH_ENABLE_INSPECTOR)
    if (m_starFish->inspector()) {
        m_starFish->inspector()->sendInfoMessage(m);
    }
#endif
    STARFISH_LOG_INFO("console.info: %s\n", m->toUTF8NonGCString().c_str());
}

void Console::error(String* m)
{
#if defined(STARFISH_ENABLE_INSPECTOR)
    if (m_starFish->inspector()) {
        m_starFish->inspector()->sendErrorMessage(m);
    }
#endif
    STARFISH_LOG_ERROR("console.error: %s\n", m->toUTF8NonGCString().c_str());
}

void Console::warn(String* m)
{
#if defined(STARFISH_ENABLE_INSPECTOR)
    if (m_starFish->inspector()) {
        m_starFish->inspector()->sendWarnMessage(m);
    }
#endif
    STARFISH_LOG_ERROR("console.warn: %s\n", m->toUTF8NonGCString().c_str());
}

void Console::debug(String* m)
{
#if defined(STARFISH_ENABLE_INSPECTOR)
    if (m_starFish->inspector()) {
        m_starFish->inspector()->sendDebugMessage(m);
    }
#endif
    STARFISH_LOG_ERROR("console.debug: %s\n", m->toUTF8NonGCString().data());
}
}
