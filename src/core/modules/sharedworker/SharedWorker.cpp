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

#include "StarfishConfig.h"

#include "core/dom/ExecutionContext.h"
#include "core/dom/DOMException.h"
#include "core/dom/MessagePort.h"
#include "core/dom/WebOrigin.h"
#include "core/storage/StorageInternal.h"
#include "core/modules/sharedworker/client/SharedWorkerProcessManager.h"
#include "core/modules/sharedworker/SharedWorkerKey.h"
#include "core/modules/sharedworker/SharedWorker.h"

namespace Starfish {

SharedWorker::SharedWorker(ExecutionContext* executionContext,
                           String* scriptURL,
                           DOMStringOrWorkerOptions nameOrOptions)
    : AbstractWorker(executionContext, scriptURL)
    , m_messagePort(new MessagePort(executionContext))
{
    if (nameOrOptions.isDOMStringValue()) {
        m_options.setName(nameOrOptions.getDOMStringValue());
    } else if (nameOrOptions.isWorkerOptionsValue()) {
        m_options = nameOrOptions.getWorkerOptionsValue();
    }

    Nullable<StorageKey*> storageKey =
        StorageInternal::getStorageKey(executionContext);
    if (!storageKey.hasValue()) {
        throw new DOMException(m_executionContext,
                               DOMException::Code::SECURITY_ERR,
                               "Cannot get storage key");
    }

    SharedWorkerKey key(storageKey.getValue()->serialize(),
                        m_scriptURL->urlString(), m_options.name());
    m_sharedWorkerKey = key.hash;

    IdHash hash;
    // ID to distinguish SharedWorker object in the client process.
    m_clientID = hash(SharedWorkerClientID::generate());

    MessagePort::entangle(m_messagePort, new MessagePort(m_executionContext));

    SharedWorkerProcessManager::instance()->requestConnection(this);
}

MessagePort* SharedWorker::port() const
{
    return m_messagePort;
}

ScriptBindingInstance* SharedWorker::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

} // namespace Starfish

#endif
