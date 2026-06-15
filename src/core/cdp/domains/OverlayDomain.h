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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPOverlayDomain__)
#define __StarfishCDPOverlayDomain__

#include <string>

namespace Starfish {

class CDPDispatcher;
class CDPCommand;

// Overlay domain.
//
// The Overlay domain drives DevTools' on-screen inspection chrome (node
// highlight boxes, the inspect-mode picker, FPS/paint/grid debug overlays).
// This engine runs headless, so there is no visual layer to draw on: every
// drawing-side method (highlightNode/hideHighlight/highlightRect/highlightQuad
// and the setShow* debug-overlay toggles) is acked as a no-op so DevTools
// clients proceed. setInspectMode additionally records the requested mode on
// the session (set->state round-trips); no inspectNodeRequested event is
// emitted since headless has no pointer hit-testing.
//
// getHighlightObjectForTest is the one real method: it computes a node's box
// model (content/padding/border/margin quads from getBoundingClientRect,
// reusing DOM.getBoxModel's logic) plus elementInfo (tagName/className/node
// dimensions) and returns it as the {highlight:{...}} object DevTools renders
// for a node. This is pure computation (no drawing), so it works in headless.
// Handlers run on the main thread.
class OverlayDomain {
public:
    OverlayDomain(CDPDispatcher* d)
        : m_dispatcher(d)
    {
    }
    void processMessage(CDPCommand& cmd, const std::string& method);

private:
    CDPDispatcher* m_dispatcher;
};

} // namespace Starfish

#endif
