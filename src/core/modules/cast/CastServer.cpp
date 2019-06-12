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

#ifdef STARFISH_ENABLE_CAST_SERVICE

#include "StarfishConfig.h"

#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/AdaptedThread.h"
#include "core/modules/threading/ThreadPool.h"

#include "core/modules/cast/SSDPRunnable.h"
#include "core/modules/cast/CastServer.h"

#define CAST_SERVER_THREAD_POOL_SIZE 5

namespace Starfish {

CastServer* CastServer::m_instance = nullptr;

CastServer* CastServer::instance()
{
    if (m_instance == nullptr) {
        m_instance = new CastServer();
    }

    STARFISH_ASSERT(m_instance != nullptr);
    return m_instance;
}

void CastServer::destroy()
{
    if (m_instance != nullptr) {
        m_instance->~CastServer();
        m_instance = nullptr;
    }
}

CastServer::CastServer()
{
    m_messageLoop = new MessageLoop();
    m_threadPool = new ThreadPool(CAST_SERVER_THREAD_POOL_SIZE, m_messageLoop);
    m_discoveryThread = new AdaptedThread(m_threadPool);
    m_appControlThread = new AdaptedThread(m_threadPool);

    m_ssdp = new SSDPRunnable(m_messageLoop);
}

bool CastServer::start()
{
    // start discovering services
    m_discoveryThread->start(m_ssdp);

    // TODO: run application server

    return false;
}

} // namespace daeyeon

#endif
