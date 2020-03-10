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
#include "core/fetch/stream/ReadableStreamDefaultReader.h"
#include "core/fetch/stream/ReadableStreamDefaultController.h"
#include "core/fetch/stream/ReadableStream.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

ReadableStreamDefaultReader::ReadableStreamDefaultReader(
    ExecutionContext* executionContext, ReadableStream* stream)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_stream(stream)
    , m_pendingCount(0)
    , m_locked(false)
    , m_disturbed(false)
    , m_state(ReadableStreamState::Readable)
    , m_closedPromise(new Promise(executionContext->scriptBindingInstance()))
{
}

ScriptBindingInstance* ReadableStreamDefaultReader::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

Promise* ReadableStreamDefaultReader::read()
{
    Promise* promise = new Promise(scriptBindingInstance());
    m_disturbed = true;

    if (m_state == ReadableStreamState::Closed ||
        m_state == ReadableStreamState::Errored) {
        promise->reject(scriptUndefined());
    } else {
        m_stream->controller()->read(promise);
    }

    return promise;
}

Promise* ReadableStreamDefaultReader::cancel()
{
    Promise* promise = new Promise(scriptBindingInstance());
    m_state = ReadableStreamState::Closed;
    m_disturbed = true;

    if (m_state == ReadableStreamState::Errored) {
        auto error =
            scriptTypeError(scriptBindingInstance(),
                            String::fromUTF8("ReadableStream is error"));
        promise->reject(createScriptValue(error));
    } else {
        promise->fulfill(scriptUndefined());
    }

    return promise;
}

void ReadableStreamDefaultReader::releaseLock()
{
    m_locked = false;
}
}
