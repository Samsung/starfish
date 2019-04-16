/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
    , m_readPromiseQueue()
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
    , m_readPromiseQueue()
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

        if (m_readPromiseQueue.size() > 0) {
            resolveRead(m_readPromiseQueue.front(),
                        ValueRef::create(stringObject));
            m_readPromiseQueue.pop_front();
        }
    }
}

void ReadableStreamDefaultController::close()
{
    m_stream->reader()->setState(ReadableStreamState::Closed);
}

void ReadableStreamDefaultController::error()
{
    if (m_stream->reader()->state() != ReadableStreamState::Readable) {
        return;
    }

    m_stream->streamBuffer()->clear();
    m_readPromiseQueue.clear();

    m_stream->reader()->setState(ReadableStreamState::Errored);
}

void ReadableStreamDefaultController::read(Promise* promise)
{
    auto streamBuffer = m_stream->streamBuffer();
    size_t bufferSize = streamBuffer->size();
    if (bufferSize > 0) {
        void* buffer = calloc(1, bufferSize);
        memcpy(buffer, streamBuffer->data(), bufferSize);
        resolveRead(promise, createArrayBuffer(scriptBindingInstance(), buffer,
                                               bufferSize));

        streamBuffer->clear();
    } else {
        m_readPromiseQueue.push_back(promise);
    }
}

void ReadableStreamDefaultController::resolveRead(Promise* promise,
                                                  ScriptValue value)
{
    ContextRef* context = m_scriptBindingInstance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(context);
    ScriptObject obj = ObjectRef::create(state);

    bool result = false;
    if (m_stream->reader()->state() == ReadableStreamState::Readable) {
        result = true;
    }

    obj->set(state, ValueRef::create(StringRef::fromASCII("done")),
             ValueRef::create(result));
    obj->set(state, ValueRef::create(StringRef::fromASCII("value")), value);
    promise->fulfill(createScriptValue(obj));
}
}
