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

#if defined(STARFISH_USE_WORKER_PROCESS) && !defined(__StarfishSocketNN__)
#define __StarfishSocketNN__
#include "core/modules/networking/Socket.h"

namespace Starfish {

#define SCK_WAIT 0
#define SCK_DONTWAIT 1

class SocketNN : public Socket {
public:
    class Exception : public Socket::Exception {
    public:
        Exception();
        const char *what() const throw() override;
    };
    SocketNN(int domain, int protocol);
    virtual ~SocketNN();

    int bind(const char *addr) override;
    int connect(const char *addr) override;
    int send(const void *buf, size_t len, int flags) override;
    int recv(void *buf, size_t len, int flags) override;
    int close() override;
    int getFd() override;
    int shutdown(int howto) override;
    void setsockopt(int level, int option, const void *optval,
                    size_t optvallen) override;
    void getsockopt(int level, int option, void *optval,
                    size_t *optvallen) override;
    short getEvents() override;

private:
    int m_fd;
    short m_events;
};

} // namespace Starfish

#endif
