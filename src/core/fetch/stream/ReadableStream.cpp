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
#include "core/fetch/stream/ReadableStream.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/fetch/stream/ReadableStreamDefaultController.h"
#include "core/fetch/stream/ReadableStreamDefaultReader.h"
#include "core/fetch/stream/ReadableStreamBuffer.h"
#include <EscargotPublic.h>
using namespace Escargot;

namespace Starfish {

ReadableStream::ReadableStream(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_controller(new ReadableStreamDefaultController(executionContext, this))
    , m_reader(new ReadableStreamDefaultReader(executionContext, this))
    , m_streamBuffer(new ReadableStreamBuffer())
    , m_state(State::Readable)
    , m_disturbed(false)
{
}

ReadableStream::ReadableStream(ExecutionContext* executionContext,
                               ScriptObject underlyingSource)
    : ReadableStream(executionContext)
{
    ContextRef* context =
        executionContext->scriptBindingInstance()->scriptContext();

    Evaluator::execute(
        context,
        [](ExecutionStateRef* state, ExecutionContext* executionContext,
           ScriptObject underlyingSource, ReadableStream* self) -> ValueRef* {
            auto object = underlyingSource->asObject();
            auto startkey = StringRef::createFromASCII("start");

            if (object->hasOwnProperty(state, startkey)) {
                auto startFunction = object->getOwnProperty(state, startkey);
                if (startFunction->isCallable()) {
                    ScriptValue argv[] = { self->m_controller->scriptValue() };

                    callScriptFunction(
                        executionContext->scriptBindingInstance(),
                        startFunction, argv, 1, scriptUndefined());
                }
            }
            return ValueRef::createUndefined();
        },
        executionContext, underlyingSource, this);
}

ReadableStream::ReadableStream(ExecutionContext* executionContext,
                               ScriptObject underlyingSource,
                               ScriptValue options)
    : ReadableStream(executionContext, underlyingSource)
{
    // TODO: handle options
}

void* ReadableStream::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(ReadableStream));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(ReadableStream)] = { 0 };
        ReadableStream::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(ReadableStream));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

ScriptBindingInstance* ReadableStream::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

ExecutionContext* ReadableStream::executionContext()
{
    return m_reader->executionContext();
}

ReadableStreamDefaultReader* ReadableStream::getReader()
{
    if (m_state == State::Closed || m_state == State::Errored || locked()) {
        throw new DOMException(executionContext(),
                               DOMException::Code::SCRIPT_TYPE_ERR);
    }

    lock();
    return m_reader;
}

bool ReadableStream::locked()
{
    return m_reader->locked();
}

void ReadableStream::lock()
{
    m_reader->setLocked(true);
}

void ReadableStream::releaseLock()
{
    m_reader->releaseLock();
}

bool ReadableStream::isDisturbedOrLocked()
{
    return m_disturbed || m_reader->locked();
}

Promise* ReadableStream::cancel()
{
    return m_reader->cancel();
}

void ReadableStream::close()
{
    m_state = ReadableStream::State::Closed;

    m_reader->runCloseStepsReadRequests();
}

void ReadableStream::resolveData(Promise* promise,
                                 ExecutionContext* executionContext,
                                 BodyType type)
{
    if (m_streamBuffer->size() > 0) {
        m_streamBuffer->resolveWithType(promise, executionContext, type);
        m_streamBuffer->clear();
    } else {
        promise->fulfill(createScriptValue(String::emptyString));
    }
}
} // namespace Starfish
