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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPFetchDomain__)
#define __StarfishCDPFetchDomain__

#include <string>

namespace Starfish {

class CDPDispatcher;
class CDPCommand;

// Fetch domain (request interception). Constrained by the synthetic, single
// threaded navigation model: only the top-level Document request can be paused,
// and pausing means parking the navigation between message-loop turns rather
// than blocking a real ResourceLoader (there is none). continueRequest resumes
// the parked load to the original URL; fulfillRequest resumes it to a data: URL
// built from the supplied body; failRequest abandons it. Sub-resource requests
// are not intercepted (no hook to pause them on).
class FetchDomain {
public:
    FetchDomain(CDPDispatcher* d)
        : m_dispatcher(d)
    {
    }
    void processMessage(CDPCommand& cmd, const std::string& method);

    // Emit Fetch.requestPaused for a top-level navigation. Returns the
    // interception requestId so the caller can park the navigation under it.
    std::string emitNavigationPaused(const std::string& sessionId,
                                     const std::string& url);

private:
    CDPDispatcher* m_dispatcher;
};

} // namespace Starfish

#endif
