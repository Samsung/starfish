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
        if (cmd.params() && cmd.params()->HasMember("mode") &&
            (*cmd.params())["mode"].IsString()) {
            s->overlayInspectMode = (*cmd.params())["mode"].GetString();
        }
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
        NodeRegistry* reg = m_dispatcher->nodeRegistry();
        int nodeId = 0;
        if (cmd.params() && cmd.params()->HasMember("nodeId") &&
            (*cmd.params())["nodeId"].IsInt()) {
            nodeId = (*cmd.params())["nodeId"].GetInt();
        }
        Node* node = reg->lookup(nodeId);
        if (!node || !node->isElement()) {
            cmd.sendError(-32000, "Could not find node with given id");
            return;
        }
        Element* el = node->asElement();
        DOMRect* rect = el->getBoundingClientRect();
        double x = rect->x();
        double y = rect->y();
        double w = rect->width();
        double h = rect->height();

        rapidjson::Document outd;
        rapidjson::Document::AllocatorType& alloc = outd.GetAllocator();

        // Clockwise quad [x1,y1, x2,y2, x3,y3, x4,y4] from a box rect, matching
        // DOM.getBoxModel.
        auto makeQuad = [&alloc](double qx, double qy, double qw,
                                 double qh) -> rapidjson::Value {
            rapidjson::Value q(rapidjson::kArrayType);
            q.PushBack(qx, alloc);
            q.PushBack(qy, alloc);
            q.PushBack(qx + qw, alloc);
            q.PushBack(qy, alloc);
            q.PushBack(qx + qw, alloc);
            q.PushBack(qy + qh, alloc);
            q.PushBack(qx, alloc);
            q.PushBack(qy + qh, alloc);
            return q;
        };

        rapidjson::Value highlight(rapidjson::kObjectType);
        // content/padding/border/margin: MVP uses the border-box rect for all
        // four, same as DOM.getBoxModel (no per-edge inset breakdown).
        highlight.AddMember("content", makeQuad(x, y, w, h), alloc);
        highlight.AddMember("padding", makeQuad(x, y, w, h), alloc);
        highlight.AddMember("border", makeQuad(x, y, w, h), alloc);
        highlight.AddMember("margin", makeQuad(x, y, w, h), alloc);

        // elementInfo: tagName / className / node dimensions, the node metadata
        // DevTools shows in the highlight tooltip.
        rapidjson::Value elementInfo(rapidjson::kObjectType);
        String* nodeName = node->nodeName();
        std::string tagName =
            nodeName ? nodeName->toUTF8NonGCString() : std::string();
        elementInfo.AddMember(
            "tagName", rapidjson::Value(tagName.c_str(), tagName.size(), alloc),
            alloc);
        std::string className;
        Optional<String*> cls = el->getAttribute(String::fromUTF8("class", 5));
        if (cls.hasValue() && cls.value()) {
            className = cls.value()->toUTF8NonGCString();
        }
        elementInfo.AddMember(
            "className",
            rapidjson::Value(className.c_str(), className.size(), alloc),
            alloc);
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
