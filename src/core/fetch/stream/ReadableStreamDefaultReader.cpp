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
#include "core/dom/ExecutionContext.h"
#include "core/fetch/stream/ReadableStreamDefaultReader.h"
#include "core/fetch/stream/ReadableStreamDefaultController.h"
#include "core/fetch/stream/ReadableStream.h"

using namespace Escargot;

namespace Starfish {

DefaultReadRequest::DefaultReadRequest(Promise* promise)
    : m_promise(promise)
{
}

void DefaultReadRequest::chunkSteps(ScriptBindingInstance* instance,
                                    ScriptValue chunk)
{
    ScriptObject result = createEmptyScriptObject(instance);
    setScriptObjectProperty(
        instance, result, createScriptValue(String::createASCIIString("done")),
        createScriptValue(false));
    setScriptObjectProperty(
        instance, result, createScriptValue(String::createASCIIString("value")),
        chunk);

    m_promise->fulfill(createScriptValue(result));
}

void DefaultReadRequest::closeSteps(ScriptBindingInstance* instance,
                                    ScriptValue chunk)
{
    ScriptObject result = createEmptyScriptObject(instance);
    setScriptObjectProperty(
        instance, result, createScriptValue(String::createASCIIString("done")),
        createScriptValue(true));
    setScriptObjectProperty(
        instance, result, createScriptValue(String::createASCIIString("value")),
        chunk);

    m_promise->fulfill(createScriptValue(result));
}

void DefaultReadRequest::errorSteps(ScriptBindingInstance* instance,
                                    ScriptValue error)
{
    m_promise->reject(error);
}

ReadableStreamDefaultReader::ReadableStreamDefaultReader(
    ExecutionContext* executionContext, ReadableStream* stream)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_stream(stream)
    , m_pendingCount(0)
    , m_locked(false)
    , m_closedPromise(new Promise(executionContext->scriptBindingInstance()))
{
}

ScriptBindingInstance* ReadableStreamDefaultReader::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

Promise* ReadableStreamDefaultReader::read()
{
    // https://streams.spec.whatwg.org/#default-reader-read
    Promise* promise = new Promise(scriptBindingInstance());

    if (!m_stream) {
        promise->reject(createScriptValue(
            scriptTypeError(scriptBindingInstance(),
                            String::createASCIIString(
                                "ReadableStreamDefaultReader is released"))));

        return promise;
    } else {
        readDefaultReadRequest(new DefaultReadRequest(promise));
    }

    return promise;
}

void ReadableStreamDefaultReader::readDefaultReadRequest(
    DefaultReadRequest* request)
{
    // https://streams.spec.whatwg.org/#readable-stream-default-reader-read
    STARFISH_ASSERT(m_stream);

    m_stream->setDisturbed(true);

    ScriptBindingInstance* instance = scriptBindingInstance();
    if (m_stream->state() == ReadableStream::State::Closed) {
        request->closeSteps(instance, scriptUndefined());
    } else if (m_stream->state() == ReadableStream::State::Errored) {
        // TODO: 5. Otherwise, if stream.[[state]] is "errored", perform
        // readRequest’s error steps given stream.[[storedError]].
        STARFISH_UNIMPLEMENTED();
    } else {
        m_stream->controller()->pull(request);
    }
}

void ReadableStreamDefaultReader::addDefaultReadRequest(
    DefaultReadRequest* request)
{
    STARFISH_ASSERT(m_stream->state() == ReadableStream::State::Readable);

    m_readRequests.push_back(request);
}

void ReadableStreamDefaultReader::fulfillReadRequest(ScriptValue chunk,
                                                     bool done)
{
    // https://streams.spec.whatwg.org/#readable-stream-fulfill-read-request

    if (m_readRequests.empty()) {
        return;
    }

    if (done) {
        m_readRequests.front()->closeSteps(scriptBindingInstance(), chunk);
    } else {
        m_readRequests.front()->chunkSteps(scriptBindingInstance(), chunk);
    }

    m_readRequests.erase(m_readRequests.begin());
}

Promise* ReadableStreamDefaultReader::cancel()
{
    Promise* promise = new Promise(scriptBindingInstance());

    m_stream->setDisturbed(true);

    if (m_stream->state() == ReadableStream::State::Errored) {
        auto error =
            scriptTypeError(scriptBindingInstance(),
                            String::fromUTF8("ReadableStream is error"));
        promise->reject(createScriptValue(error));
    } else {
        promise->fulfill(scriptUndefined());
    }

    m_stream->setState(ReadableStream::State::Closed);

    return promise;
}

void ReadableStreamDefaultReader::releaseLock()
{
    m_locked = false;
}

void ReadableStreamDefaultReader::runCloseStepsReadRequests()
{
    for (auto& request : m_readRequests) {
        request->closeSteps(scriptBindingInstance(), scriptUndefined());
    }

    m_readRequests.clear();
    m_readRequests.shrink_to_fit();
}

} // namespace Starfish
