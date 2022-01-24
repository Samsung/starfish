/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/extra/Console.h"
#include "core/inspector/Inspector.h"
#include "core/page/WebBase.h"

namespace Starfish {

Console::Console(WebBase* webBase)
    : m_webBase(webBase)
{
}

void Console::log(String* m)
{
#if defined(STARFISH_ENABLE_INSPECTOR)
    if (m_webBase->inspector()) {
        m_webBase->inspector()->sendInfoMessage(m);
    }
#endif
    m->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
#if defined(STARFISH_TIZEN_PROD_TV)
            STARFISH_LOG_ERROR("console.log: %s", buf);
#else
            STARFISH_LOG_INFO("console.log: %s", buf);
#endif
            return 0;
        },
        nullptr);
}

void Console::info(String* m)
{
#if defined(STARFISH_ENABLE_INSPECTOR)
    if (m_webBase->inspector()) {
        m_webBase->inspector()->sendInfoMessage(m);
    }
#endif

    m->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
#if defined(STARFISH_TIZEN_PROD_TV)
            STARFISH_LOG_ERROR("console.info: %s", buf);
#else
            STARFISH_LOG_INFO("console.info: %s", buf);
#endif
            return 0;
        },
        nullptr);
}

void Console::error(String* m)
{
#if defined(STARFISH_ENABLE_INSPECTOR)
    if (m_webBase->inspector()) {
        m_webBase->inspector()->sendErrorMessage(m);
    }
#endif
    m->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            STARFISH_LOG_ERROR("console.error: %s", buf);
            return 0;
        },
        nullptr);
}

void Console::warn(String* m)
{
#if defined(STARFISH_ENABLE_INSPECTOR)
    if (m_webBase->inspector()) {
        m_webBase->inspector()->sendWarnMessage(m);
    }
#endif
    m->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            STARFISH_LOG_ERROR("console.warn: %s", buf);
            return 0;
        },
        nullptr);
}

void Console::debug(String* m)
{
#if defined(STARFISH_ENABLE_INSPECTOR)
    if (m_webBase->inspector()) {
        m_webBase->inspector()->sendDebugMessage(m);
    }
#endif
    m->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            STARFISH_LOG_ERROR("console.debug: %s", buf);
            return 0;
        },
        nullptr);
}
} // namespace Starfish
