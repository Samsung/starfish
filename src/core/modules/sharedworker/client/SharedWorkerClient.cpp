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

#include "core/modules/worker/WorkerConfig.h"
#include "core/modules/worker/util/network/SocketNN.h"
#include "core/modules/sharedworker/SharedWorker.h"
#include "core/modules/sharedworker/client/SharedWorkerClient.h"

namespace Starfish {

SharedWorkerClient::SharedWorkerClient(PerProcess* perProcess,
                                       const std::string& ipcAddress)
    : SharedWorkerConnection(perProcess, ipcAddress, SocketNN::kRequestProtocol)
{
}

void SharedWorkerClient::start()
{
    connect();
}

void SharedWorkerClient::requestConnection(SharedWorker* sharedWorker)
{
    // TODO: Request a connection to the server.
}

void SharedWorkerClient::requestClose(SharedWorker* sharedWorker)
{
    // TODO: Request a close to the server.
}

} // namespace Starfish

#endif
