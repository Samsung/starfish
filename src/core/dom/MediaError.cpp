/*
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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
#include "core/dom/ExecutionContext.h"
#include "core/dom/MediaError.h"

namespace Starfish {

MediaError::MediaError(ExecutionContext* executionContext, int32_t code,
                       String* message)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(executionContext->scriptBindingInstance())
    , m_code(code)
    , m_message(message)
{
}

int32_t MediaError::code() const
{
    return m_code;
}

String* MediaError::message() const
{
    return m_message ? m_message : String::emptyString;
}

ScriptBindingInstance* MediaError::scriptBindingInstance()
{
    return m_scriptBindingInstance;
}

#if !defined(STARFISH_ENABLE_MULTIMEDIA)
void MediaError::init(ScriptBindingInstance* instance, void* domObjectPointer)
{
    STARFISH_ASSERT_NOT_REACHED();
}

bool MediaError::isMediaError() const
{
    return true;
}
#endif
} // namespace Starfish
