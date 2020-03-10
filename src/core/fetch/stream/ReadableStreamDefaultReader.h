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

#ifndef __StarfishReadableStreamDefaultReader__
#define __StarfishReadableStreamDefaultReader__

#include "binding/ScriptWrappable.h"

namespace Starfish {

enum class ReadableStreamState { Readable, Closed, Errored };

class ReadableStreamDefaultReader : public ScriptWrappable {
public:
    ReadableStreamDefaultReader(ExecutionContext* executionContext,
                                ReadableStream* stream);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(ReadableStreamDefaultReader)

    ExecutionContext* executionContext()
    {
        return m_executionContext;
    }

    Promise* read();
    Promise* cancel();

    void releaseLock();
    bool locked()
    {
        return m_locked;
    }
    void setLocked(bool locked)
    {
        m_locked = locked;
    }

    bool disturbed()
    {
        return m_disturbed;
    }
    void setDisturbed(bool disturbed)
    {
        m_disturbed = disturbed;
    }

    ReadableStreamState state()
    {
        return m_state;
    }
    void setState(ReadableStreamState state)
    {
        m_state = state;
    }

    Promise* closed()
    {
        return m_closedPromise;
    }

private:
    ExecutionContext* m_executionContext;
    ReadableStream* m_stream;
    size_t m_pendingCount;
    bool m_locked;
    bool m_disturbed;
    ReadableStreamState m_state;
    Promise* m_closedPromise;
};
}

#endif
