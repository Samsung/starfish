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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPDomain__)
#define __StarfishCDPDomain__

#include <cstddef>
#include <string>

namespace Starfish {

class CDPCommand;
class CDPSession;
class WebView;

// CDPDispatcher reaches the protocol domains only through this interface, so
// the dispatcher core does not depend on the domain implementations that
// depend on it. The hooks mirror the dispatcher lifecycle; a domain overrides
// the ones it keeps state for.
class CDPDomain {
public:
    virtual ~CDPDomain()
    {
    }
    virtual void processMessage(CDPCommand& cmd, const std::string& method) = 0;

    // Replies still owed to the client will never be delivered.
    virtual void cancelPendingWork()
    {
    }
    // The client disconnected; forget what it set up.
    virtual void resetConnection()
    {
    }
    // The session of a target that outlives the connection is about to be
    // replaced.
    virtual void willResetSession(CDPSession* session, WebView* webView)
    {
    }
    // The target that owns webView is about to be destroyed.
    virtual void willDestroyTarget(WebView* webView)
    {
    }
    // A child frame of the current target finished loading.
    virtual void childFrameLoaded(const std::string& sessionId)
    {
    }
};

template <typename Owner>
using CDPHandlerFn = void (Owner::*)(CDPCommand&, const std::string&);

// A row of a name to handler table. Tables are static arrays so dispatch
// allocates nothing.
template <typename Owner>
struct CDPMethodHandler {
    const char* name;
    CDPHandlerFn<Owner> handler;
};

template <typename Owner, size_t N>
CDPHandlerFn<Owner> findCDPMethodHandler(
    const CDPMethodHandler<Owner> (&table)[N], const std::string& name)
{
    for (const CDPMethodHandler<Owner>& entry : table) {
        if (name == entry.name) {
            return entry.handler;
        }
    }
    return nullptr;
}

} // namespace Starfish

#endif
