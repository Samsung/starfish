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

#ifndef __StarfishSocket__
#define __StarfishSocket__

namespace Starfish {

#define SCK_DONTWAIT 1

class Socket {
public:
    class Exception : public std::exception {
    public:
        Exception();
        const char *what() const throw() override;
        int num() const
        {
            return m_err;
        }

    private:
        int m_err;
    };

    virtual ~Socket(){};

    virtual int bind(const char *addr) = 0;
    virtual int connect(const char *addr) = 0;
    virtual int send(const void *buf, size_t len, int flags) = 0;
    virtual int recv(void *buf, size_t len, int flags) = 0;
    virtual int close() = 0;
    virtual int getFd() = 0;
    virtual int shutdown(int howto) = 0;
    virtual void setsockopt(int level, int option, const void *optval,
                            size_t optvallen) = 0;
    virtual void getsockopt(int level, int option, void *optval,
                            size_t *optvallen) = 0;
    virtual short getEvents() = 0;
};

class SocketNN : public Socket, public gc {
public:
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
