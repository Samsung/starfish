/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SHARED_WORKER)
#ifndef __StarfishSharedWorker__
#define __StarfishSharedWorker__

#include "core/modules/worker/AbstractWorker.h"
#include "binding/generated/DOMStringOrWorkerOptionsUnion.h"

namespace Starfish {

class ExecutionContext;
class MessagePort;

class SharedWorker : public AbstractWorker {
public:
    SharedWorker(
        ExecutionContext* executionContext, String* scriptURL,
        DOMStringOrWorkerOptions nameOrOptions = DOMStringOrWorkerOptions());

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(SharedWorker)

    virtual ExecutionContext* executionContext() const override
    {
        return m_executionContext;
    }

    MessagePort* port() const;

    DEFINE_GETTER(size_t, sharedWorkerKey);
    DEFINE_GETTER(uint32_t, clientID);

private:
    MessagePort* m_messagePort;
    size_t m_sharedWorkerKey;
    uint32_t m_clientID;
};

} // namespace Starfish

#endif
#endif
