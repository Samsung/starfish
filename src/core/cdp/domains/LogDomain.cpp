/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_CDP)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "LogDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "../CDPSession.h"

namespace Starfish {

void LogDomain::processMessage(CDPCommand& cmd, const std::string& method)
{
    CDPSession* s = m_dispatcher->session();

    if (method == "enable") {
        s->logEnabled = true;
        cmd.sendResultEmpty();
        return;
    }
    if (method == "disable") {
        s->logEnabled = false;
        cmd.sendResultEmpty();
        return;
    }
    if (method == "clear") {
        cmd.sendResultEmpty();
        return;
    }

    cmd.sendError(-32601, "'method' wasn't found");
}

} // namespace Starfish

#endif
