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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPStorageDomain__)
#define __StarfishCDPStorageDomain__

#include <string>

namespace Starfish {

class CDPDispatcher;
class CDPCommand;

// Storage domain. Browser-level storage management mirroring Chrome's Storage
// domain. The cookie methods (getCookies/setCookies/clearCookies) share the
// process-wide curl cookie jar with the Network domain by delegating to
// NetworkDomain's static cookie-jar helpers, so cookies set through either
// domain are visible to both. clearDataForOrigin additionally clears the
// per-origin localStorage/sessionStorage areas through the routed WebView's
// StorageNamespace (the same areas DOMStorage and window.localStorage use).
// getStorageKeyForFrame returns the routed page's serialized origin. Handlers
// run on the main thread.
class StorageDomain {
public:
    StorageDomain(CDPDispatcher* d)
        : m_dispatcher(d)
    {
    }
    void processMessage(CDPCommand& cmd, const std::string& method);

private:
    CDPDispatcher* m_dispatcher;
};

} // namespace Starfish

#endif
