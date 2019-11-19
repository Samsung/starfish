/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_WEBAUDIO)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/webaudio/AudioBuffer.h"

#include "core/dom/ExecutionContext.h"

namespace Starfish {
AudioBuffer::AudioBuffer(ExecutionContext* executionContext,
                         AudioBufferOptions options)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj, void* cd) { ((AudioBuffer*)obj)->~AudioBuffer(); },
        NULL, NULL, NULL);
}

AudioBuffer::AudioBuffer(ExecutionContext* executionContext,
                         std::unique_ptr<uint8_t> buffer, uint32_t length)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
{
    m_buffer = std::move(buffer);
    m_length = length;

    GC_REGISTER_FINALIZER_NO_ORDER(
        this, [](void* obj, void* cd) { ((AudioBuffer*)obj)->~AudioBuffer(); },
        NULL, NULL, NULL);
}

AudioBuffer::~AudioBuffer()
{
}

ScriptBindingInstance* AudioBuffer::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

uint8_t* AudioBuffer::rawBuffer()
{
    return m_buffer.get();
}
} // namespace Starfish

#endif
