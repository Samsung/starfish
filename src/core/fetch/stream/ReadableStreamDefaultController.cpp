/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "binding/ScriptBindingInstance.h"
#include "core/dom/DOMException.h"
#include "core/fetch/stream/ReadableStream.h"
#include "core/fetch/stream/ReadableStreamDefaultController.h"
#include "core/fetch/stream/ReadableStreamDefaultReader.h"
#include "core/fetch/stream/ReadableStreamBuffer.h"
#include "core/dom/ExecutionContext.h"
#include <EscargotPublic.h>
using namespace Escargot;

namespace Starfish {

ReadableStreamDefaultController::ReadableStreamDefaultController(
    ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(executionContext->scriptBindingInstance())
    , m_stream(new ReadableStream(executionContext))
    , m_mimeType(String::emptyString)
{
    // This constructor cannot be used directly.
    throw new DOMException(executionContext,
                           DOMException::Code::SCRIPT_TYPE_ERR);
}

ReadableStreamDefaultController::ReadableStreamDefaultController(
    ExecutionContext* executionContext, ReadableStream* stream)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(executionContext->scriptBindingInstance())
    , m_stream(stream)
    , m_mimeType(String::emptyString)
{
}

ScriptBindingInstance* ReadableStreamDefaultController::scriptBindingInstance()
{
    return m_scriptBindingInstance;
}

void ReadableStreamDefaultController::enqueue(ScriptValue chunk)
{
    if (chunk->isString()) {
        auto stringObject = chunk->asString();

        m_stream->reader()->fulfillReadRequest(chunk, false);
    }
}

void ReadableStreamDefaultController::close()
{
    m_stream->setState(ReadableStream::State::Closed);
}

void ReadableStreamDefaultController::error()
{
    if (m_stream->state() != ReadableStream::State::Readable) {
        return;
    }

    m_stream->streamBuffer()->clear();

    m_stream->setState(ReadableStream::State::Errored);
}

void ReadableStreamDefaultController::pull(DefaultReadRequest* request)
{
    // https://streams.spec.whatwg.org/#rs-default-controller-private-pull

    ReadableStreamBuffer* streamBuffer = m_stream->streamBuffer();

    if (!streamBuffer->empty()) {
        ScriptValue chunk = streamBuffer->dequeueValue(scriptBindingInstance());

        // TODO: 2-2. If this.[[closeRequested]] is true
        if (streamBuffer->empty()) {
            m_stream->streamBuffer()->clear();
            m_stream->close();
        }
        // TODO: 2-3. Otherwise, perform !
        // ReadableStreamDefaultControllerCallPullIfNeeded(this).

        request->chunkSteps(scriptBindingInstance(), chunk);

    } else {
        m_stream->reader()->addDefaultReadRequest(request);
        // TODO: 3-2.Perform !
        // ReadableStreamDefaultControllerCallPullIfNeeded(this).
    }
}

} // namespace Starfish
