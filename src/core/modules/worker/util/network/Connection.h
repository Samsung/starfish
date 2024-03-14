/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_USE_WORKER_PROCESS)
#ifndef __StarfishConnection__
#define __StarfishConnection__

#include "core/modules/worker/util/network/IORunnable.h"
#include <string>

namespace Starfish {

class Socket;

class Connection : public IORunnable::Client {
public:
    Connection();
    Connection(int protocol);

    void send(const char* data, size_t len);
    void onReceived(Socket* socket, const char* data, size_t len) override;
    void onStopped() override;
    Socket* socket() override;

protected:
    Socket* m_socket;

private:
    bool m_blockingMode{ true };
};

} // namespace Starfish

#endif
#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
