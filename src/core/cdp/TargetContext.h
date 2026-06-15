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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPTargetContext__)
#define __StarfishCDPTargetContext__

namespace Starfish {

class WebView;
class CDPSession;
class NodeRegistry;
class RemoteObjectStore;

// Per-target (per-WebView) CDP state. One TargetContext exists per page/tab.
// Plain malloc heap (not GC). The two GC-pointer members
// (nodeRegistry/remoteObjectStore) are rooted via GC_add_roots by the
// CDPDispatcher when the context is registered (see CDPDispatcher.cpp).
struct TargetContext {
    WebView* webView;                     // owning WebView for this target
    CDPSession* session;                  // plain; per-target session state
    NodeRegistry* nodeRegistry;           // GC; rooted
    RemoteObjectStore* remoteObjectStore; // GC; rooted
    bool ownsWebView;                     // true for createTarget-spawned tabs
};

} // namespace Starfish

#endif
