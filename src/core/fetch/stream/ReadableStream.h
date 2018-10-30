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

#ifndef __StarfishReadableStream__
#define __StarfishReadableStream__

#include "binding/ScriptWrappable.h"
#include "core/fetch/stream/ReadableStreamDefaultController.h"
#include "core/fetch/stream/ReadableStreamDefaultReader.h"
#include "core/fetch/stream/ReadableStreamBuffer.h"

namespace Starfish {

class ReadableStream : public ScriptWrappable, public DocumentHoldable {
public:
    ReadableStream(Document* document,
                   ReadableStreamBuffer* buffer = new ReadableStreamBuffer());
    ReadableStream(Document* document, ScriptObject underlyingSource);
    ReadableStream(Document* document, ScriptObject underlyingSource,
                   ScriptValue options);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return m_scriptBindingInstance;
    }
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isReadableStream() const override;

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
    void setLocked(bool lock);
    void releaseLock();
    bool disturbed();
    bool isDisturbedOrLocked();
    Promise* cancel();
    void close();
    void resolveData(Promise* promise, ScriptBindingInstance* instance,
                     BodyType type);

protected:
private:
    ScriptBindingInstance* m_scriptBindingInstance;
    ReadableStreamDefaultController* m_controller;
    ReadableStreamDefaultReader* m_reader;
    ReadableStreamBuffer* m_streamBuffer;

    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(ReadableStream, m_controller));
        GC_set_bit(desc, GC_WORD_OFFSET(ReadableStream, m_reader));
    }
};
}

#endif
