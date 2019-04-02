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

#ifndef __StarfishReadableStreamDefaultController__
#define __StarfishReadableStreamDefaultController__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class ExecutionContext;
class ReadableStream;

class ReadableStreamDefaultController : public ScriptWrappable {
public:
    ReadableStreamDefaultController(ExecutionContext* executionContext);
    ReadableStreamDefaultController(ExecutionContext* executionContext,
                                    ReadableStream* stream);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(ReadableStreamDefaultController)

    void enqueue(ScriptValue chunk);
    void close();
    void error();
    void read(Promise* promise);
    void resolveRead(Promise* promise, ScriptValue value);

private:
    ScriptBindingInstance* m_scriptBindingInstance;
    ReadableStream* m_stream;
    GCDeque<Promise*> m_readPromiseQueue;
    String* m_mimeType;
};
}
#endif
