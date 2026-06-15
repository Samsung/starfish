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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPDOMStorageDomain__)
#define __StarfishCDPDOMStorageDomain__

#include <string>

namespace Starfish {

class CDPDispatcher;
class CDPCommand;
class StorageInternal;

// DOMStorage domain. Exposes the per-origin localStorage/sessionStorage areas
// of the routed target's WebView to CDP clients. The storageId.isLocalStorage
// flag selects the local vs session StorageNamespace; the area is resolved
// through the same StorageNamespace::storageInternal(WebOrigin*) path the
// window.localStorage / window.sessionStorage bindings use, so reads and writes
// are shared with page script. enable/disable ack only. Handlers run on the
// main thread.
class DOMStorageDomain {
public:
    DOMStorageDomain(CDPDispatcher* d)
        : m_dispatcher(d)
    {
    }
    void processMessage(CDPCommand& cmd, const std::string& method);

private:
    // Resolve the StorageInternal for the storageId in cmd's params, using the
    // routed target's document origin. Returns nullptr (and sends a CDP error)
    // when the page has no document / opaque origin.
    StorageInternal* resolveArea(CDPCommand& cmd);

    CDPDispatcher* m_dispatcher;
};

} // namespace Starfish

#endif
