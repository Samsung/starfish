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
#include "OverlayDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "../CDPSession.h"
#include "../NodeRegistry.h"
#include "core/dom/Node.h"
#include "core/dom/Element.h"
#include "core/dom/DOMRect.h"

#include "rapidjson/document.h"

namespace Starfish {

void OverlayDomain::processMessage(CDPCommand& cmd, const std::string& method)
{
    CDPSession* s = m_dispatcher->session();

    if (method == "enable") {
        s->overlayEnabled = true;
        cmd.sendResultEmpty();
        return;
    }
    if (method == "disable") {
        s->overlayEnabled = false;
        cmd.sendResultEmpty();
        return;
    }

    if (method == "setInspectMode") {
        // Record the requested inspect mode (set->state round-trips). Headless
        // has no pointer hit-testing, so no Overlay.inspectNodeRequested is
        // emitted; the highlightConfig is ignored (no visual overlay).
        s->overlayInspectMode = paramString(cmd, "mode", s->overlayInspectMode);
        cmd.sendResultEmpty();
        return;
    }

    if (method == "getHighlightObjectForTest") {
        // Real method: compute the node's box model + element info and return
        // it as the {highlight:{...}} object DevTools renders. Pure computation
        // (no drawing), so it works in headless. Reuses DOM.getBoxModel's
        // logic: getBoundingClientRect() runs layout if needed and yields
        // CSS-px, viewport-relative coordinates -- the space CDP clients
        // expect.
        Element* el = elementForNodeIdParam(m_dispatcher, cmd);
        if (!el) {
            return;
        }
        DOMRect* rect = el->getBoundingClientRect();
        double x = rect->x();
        double y = rect->y();
        double w = rect->width();
        double h = rect->height();

        rapidjson::Document outd;
        rapidjson::Document::AllocatorType& alloc = outd.GetAllocator();

        rapidjson::Value highlight(rapidjson::kObjectType);
        // content/padding/border/margin: MVP uses the border-box rect for all
        // four, same as DOM.getBoxModel (no per-edge inset breakdown).
        highlight.AddMember("content", cdpQuad(x, y, w, h, alloc), alloc);
        highlight.AddMember("padding", cdpQuad(x, y, w, h, alloc), alloc);
        highlight.AddMember("border", cdpQuad(x, y, w, h, alloc), alloc);
        highlight.AddMember("margin", cdpQuad(x, y, w, h, alloc), alloc);

        // elementInfo: tagName / className / node dimensions, the node metadata
        // DevTools shows in the highlight tooltip.
        rapidjson::Value elementInfo(rapidjson::kObjectType);
        String* nodeName = el->nodeName();
        std::string tagName =
            nodeName ? nodeName->toUTF8NonGCString() : std::string();
        addStringMember(elementInfo, "tagName", tagName, alloc);
        std::string className;
        Optional<String*> cls = el->getAttribute(String::fromUTF8("class", 5));
        if (cls.hasValue() && cls.value()) {
            className = cls.value()->toUTF8NonGCString();
        }
        addStringMember(elementInfo, "className", className, alloc);
        elementInfo.AddMember("idValue", "", alloc);
        elementInfo.AddMember("nodeWidth", (int)(w + 0.5), alloc);
        elementInfo.AddMember("nodeHeight", (int)(h + 0.5), alloc);
        highlight.AddMember("elementInfo", elementInfo, alloc);

        rapidjson::Value result(rapidjson::kObjectType);
        result.AddMember("highlight", highlight, alloc);
        cmd.sendResult(result, outd);
        return;
    }

    // Everything else is a drawing-side method with no visual layer in
    // headless:
    //   highlightNode / hideHighlight / highlightRect / highlightQuad
    //   setShowFPSCounter / setShowPaintRects / setShowDebugBorders /
    //   setShowScrollBottleneckRects / setShowViewportSizeOnResize /
    //   setShowGridOverlays / setShowFlexOverlays
    // Acked (no throw) so DevTools handshakes proceed.
    cmd.sendResultEmpty();
}

} // namespace Starfish

#endif
