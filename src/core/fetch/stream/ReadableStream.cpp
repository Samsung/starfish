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
#include "core/fetch/stream/ReadableStream.h"
#include "core/dom/DOMException.h"
#include "core/dom/Document.h"
#include <EscargotPublic.h>
using namespace Escargot;

namespace Starfish {

ReadableStream::ReadableStream(Document* document, ReadableStreamBuffer* buffer)
    : ScriptWrappable(this, document->executionContext())
    , DocumentHoldable(document)
    , m_scriptBindingInstance(document->scriptBindingInstance())
    , m_controller(new ReadableStreamDefaultController(document, this))
    , m_reader(new ReadableStreamDefaultReader(document, this))
    , m_streamBuffer(buffer)
{
}

ReadableStream::ReadableStream(Document* document,
                               ScriptObject underlyingSource)
    : ReadableStream(document)
{
    ContextRef* context = m_scriptBindingInstance->scriptContext();
    ExecutionStateRef* state = ExecutionStateRef::create(context);

    auto object = underlyingSource->asObject();
    auto startkey = ValueRef::create(StringRef::fromASCII("start"));

    if (object->hasOwnProperty(state, startkey)) {
        auto startFunction = object->getOwnProperty(state, startkey);
        if (startFunction->isFunction()) {
            ScriptValue argv[] = { m_controller->scriptValue() };

            callScriptFunction(m_scriptBindingInstance, startFunction, argv, 1,
                               scriptUndefined());
        }
    }
}

ReadableStream::ReadableStream(Document* document,
                               ScriptObject underlyingSource,
                               ScriptValue options)
    : ReadableStream(document, underlyingSource)
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

ReadableStreamDefaultReader* ReadableStream::getReader()
{
    auto state = m_reader->state();
    if (state == ReadableStreamState::Closed ||
        state == ReadableStreamState::Errored || locked()) {
        throw new DOMException(document(), DOMException::Code::SCRIPT_TYPE_ERR);
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

bool ReadableStream::disturbed()
{
    return m_reader->disturbed();
}

bool ReadableStream::isDisturbedOrLocked()
{
    return m_reader->disturbed() || m_reader->locked();
}

Promise* ReadableStream::cancel()
{
    return m_reader->cancel();
}

void ReadableStream::close()
{
    m_reader->setState(ReadableStreamState::Closed);
}

void ReadableStream::resolveData(Promise* promise,
                                 ScriptBindingInstance* instance, BodyType type)
{
    if (m_streamBuffer->size() > 0) {
        m_streamBuffer->resolveWithType(promise, instance, type);
        m_streamBuffer->clear();
    } else {
        promise->fulfill(createScriptValue(String::emptyString));
    }
}
}
