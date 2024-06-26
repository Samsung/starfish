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

#ifndef __StarfishReadableStream__
#define __StarfishReadableStream__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class ReadableStreamDefaultController;
class ReadableStreamDefaultReader;
class ReadableStreamBuffer;
enum class BodyType;

class ReadableStream : public ScriptWrappable {
public:
    enum State : uint8_t { Readable, Closed, Errored };

    ReadableStream(ExecutionContext* executionContext);
    ReadableStream(ExecutionContext* executionContext,
                   ScriptObject underlyingSource);
    ReadableStream(ExecutionContext* executionContext,
                   ScriptObject underlyingSource, ScriptValue options);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(ReadableStream)

    ExecutionContext* executionContext();

    ReadableStreamDefaultReader* getReader();
    ReadableStreamDefaultReader* reader()
    {
        return m_reader;
    }

    ReadableStreamDefaultController* controller()
    {
        return m_controller;
    }

    ReadableStreamBuffer* streamBuffer()
    {
        return m_streamBuffer;
    }

    bool locked();
    void lock();
    void releaseLock();
    bool isDisturbedOrLocked();
    Promise* cancel();
    void close();
    void resolveData(Promise* promise, ExecutionContext* executionContext,
                     BodyType type);

    DEFINE_GETTER_SETTER(State, state, State);
    DEFINE_GETTER_SETTER(bool, disturbed, Disturbed);

protected:
private:
    ReadableStreamDefaultController* m_controller;
    ReadableStreamDefaultReader* m_reader;
    ReadableStreamBuffer* m_streamBuffer;
    State m_state;
    bool m_disturbed;

    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(ReadableStream, m_controller));
        GC_set_bit(desc, GC_WORD_OFFSET(ReadableStream, m_reader));
        GC_set_bit(desc, GC_WORD_OFFSET(ReadableStream, m_streamBuffer));
    }
};
} // namespace Starfish

#endif
