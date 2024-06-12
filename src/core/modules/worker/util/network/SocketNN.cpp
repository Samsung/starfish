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

#ifdef STARFISH_USE_WORKER_PROCESS

#include "StarfishConfig.h"

#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/pubsub.h>
#include <nanomsg/reqrep.h>
#include <nanomsg/bus.h>

#include "core/modules/worker/util/network/SocketNN.h"

namespace Starfish {

const int SocketNN::kBusProtocol = NN_BUS;
const int SocketNN::kPairProtocol = NN_PAIR;

SocketNN::Exception::Exception()
    : Socket::Exception::Exception(nn_errno())
{
}

const char *SocketNN::Exception::what() const throw()
{
    return nn_strerror(num());
}

SocketNN::SocketNN(int domain, int protocol)
    : m_fd(-1)
    , m_events(0)
{
    m_fd = nn_socket(domain, protocol);

    if (UNLIKELY(m_fd < 0)) {
        throw SocketNN::Exception();
    }

    if (protocol == NN_PAIR || protocol == NN_REQ || protocol == NN_REP) {
        m_events = NN_POLLIN | NN_POLLOUT;
    } else if (protocol == NN_PUB || protocol == NN_PUSH) {
        m_events = NN_POLLOUT;
    } else if (protocol == NN_SUB || protocol == NN_PULL ||
               protocol == NN_BUS) {
        m_events = NN_POLLIN;
    } else {
        STARFISH_ASSERT_NOT_REACHED();
        errno = 0;
        throw SocketNN::Exception();
    }
}

SocketNN::~SocketNN()
{
    nn_close(m_fd);
}

void SocketNN::setsockopt(int level, int option, const void *optval,
                          size_t optvallen)
{
    int res = nn_setsockopt(m_fd, level, option, optval, optvallen);
    if (UNLIKELY(res != 0)) {
        throw SocketNN::Exception();
    }
}

void SocketNN::getsockopt(int level, int option, void *optval,
                          size_t *optvallen)
{
    int res = nn_getsockopt(m_fd, level, option, optval, optvallen);
    if (UNLIKELY(res != 0)) {
        throw SocketNN::Exception();
    }
}

int SocketNN::bind(const char *addr)
{
    int res = nn_bind(m_fd, addr);
    if (UNLIKELY(res < 0)) {
        throw SocketNN::Exception();
    }
    return res;
}

int SocketNN::close()
{
    int res = nn_close(m_fd);
    if (UNLIKELY(res < 0)) {
        throw SocketNN::Exception();
    }
    return res;
}

int SocketNN::connect(const char *addr)
{
    int res = nn_connect(m_fd, addr);
    if (UNLIKELY(res < 0)) {
        throw SocketNN::Exception();
    }

    return res;
}

int SocketNN::shutdown(int howto)
{
    int res = nn_shutdown(m_fd, howto);
    if (UNLIKELY(res != 0)) {
        throw SocketNN::Exception();
    }
    return res;
}

int SocketNN::send(const void *buf, size_t len, int flags)
{
    int res = nn_send(m_fd, buf, len, flags);
    if (UNLIKELY(res < 0)) {
        if (UNLIKELY(nn_errno() != EAGAIN)) {
            throw SocketNN::Exception();
        }
        return -1;
    }
    return res;
}

int SocketNN::recv(void *buf, size_t len, int flags)
{
    int res = nn_recv(m_fd, buf, len, flags);
    if (UNLIKELY(res < 0)) {
        STARFISH_LOG_INFO("cannot receive message: %s\n",
                          nn_strerror(nn_errno()));
        if (UNLIKELY(nn_errno() != EAGAIN)) {
            throw SocketNN::Exception();
        }
        return -1;
    }
    return res;
}

int SocketNN::getFd()
{
    return m_fd;
};

short SocketNN::getEvents()
{
    return m_events;
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
