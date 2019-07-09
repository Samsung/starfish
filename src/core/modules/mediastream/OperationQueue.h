/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_WEBRTC)

#ifndef __StarfishOperationQueue__
#define __StarfishOperationQueue__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

namespace Starfish {
class ExecutionContext;
class Promise;

class OperationQueue : public gc {
    typedef void (*OperationFunction)(Promise*, void*);

    class Operation : public gc {
    public:
        OperationFunction m_f;
        void* m_data;
        void* m_data1;
    };

public:
    OperationQueue(ExecutionContext* executionContext)
        : m_executionContext(executionContext)
    {
    }
    void enqueue(OperationFunction fn, Promise* data, void* data1);

private:
    ExecutionContext* m_executionContext;
    GCDeque<Operation> m_queue;
};
}
#endif
#endif
