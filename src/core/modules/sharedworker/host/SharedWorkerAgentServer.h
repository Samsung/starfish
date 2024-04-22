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

#if defined(STARFISH_ENABLE_SHARED_WORKER) && defined(STARFISH_WEBWORKER_HOST)
#ifndef __StarfishSharedWorkerAgentServer__
#define __StarfishSharedWorkerAgentServer__

#include "core/modules/sharedworker/SharedWorkerConnection.h"

namespace Starfish {

class PerProcess;

class SharedWorkerAgentServer final : public SharedWorkerConnection {
public:
    SharedWorkerAgentServer(PerProcess* perProcess, const std::string& address);

    void start();

private:
};

} // namespace Starfish

#endif
#endif
