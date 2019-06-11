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

#if defined(STARFISH_ENABLE_CAST_SERVICE) && !defined(__StarfishCastServer__)
#define __StarfishCastServer__

namespace Starfish {

class IThread;
class ThreadPool;
class MessageLoop;
class SSDPServer;

class CastServer final : public gc {
public:
    static CastServer* instance();
    void destroy();
    bool start();

private:
    CastServer();
    ~CastServer() = default;

    static CastServer* m_instance;

    IThread* m_ssdpThread{ nullptr };
    IThread* m_cpThread{ nullptr };
    SSDPServer* m_ssdpServer{ nullptr };

    ThreadPool* m_threadPool{ nullptr };
    MessageLoop* m_messageLoop{ nullptr };
};

} // namespace Starfish
#endif
