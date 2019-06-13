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

#include <httplib.h>

#include "StarfishConfig.h"
#include "core/modules/cast/CastConfig.h"
#include "core/modules/cast/DIALRunnable.h"

namespace Starfish {

DIALRunnable::DIALRunnable(MessageLoop* messageLoop, CastConfig* config)
    : BaseRunnable(messageLoop)
    , m_server(new httplib::Server())
    , m_config(config)
{
    STARFISH_ASSERT(messageLoop != nullptr);
    STARFISH_ASSERT(m_server != nullptr);
    STARFISH_ASSERT(config != nullptr);
}

bool DIALRunnable::doRun()
{
    return true;
}

void DIALRunnable::stop()
{
    BaseRunnable::stop();

    m_server->stop();
    delete m_server;
}

} // namespace Starfish

#endif
