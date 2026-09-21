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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPAccessibilityDomain__)
#define __StarfishCDPAccessibilityDomain__

#include <memory>
#include <string>

namespace Starfish {

class AXNode;
class AXTree;
class CDPDispatcher;
class CDPCommand;
class Document;
class Node;

class AccessibilityDomain {
public:
    AccessibilityDomain(CDPDispatcher* d)
        : m_dispatcher(d)
    {
    }
    void processMessage(CDPCommand& cmd, const std::string& method);

    // The document finished loading. Chromium resets what it knows the
    // client holds and announces the new root, so a client that was
    // attached across the navigation starts from a known node.
    void emitLoadComplete(const std::string& sessionId);

    // An AX-relevant DOM change. The node is reported only if the client
    // was handed it earlier; the event is coalesced by a short timer,
    // because one DOM operation dirties several nodes.
    void markNodeDirty(Node* domNode);

private:
    Optional<Document*> documentFor(CDPCommand& cmd);
    // A fresh tree per command. Nothing is kept between commands, so a page
    // no client is inspecting carries no accessibility data at all.
    std::unique_ptr<AXTree> buildTree(Document* document);
    static AXNode* nearestAXAncestor(AXTree* tree, Node* domNode);
    // Static Timer callback for the coalescing window; `data` is a malloc'd
    // record that re-resolves the session, which may be gone by then.
    static void onDirtyFlushTimer(void* data);

    CDPDispatcher* m_dispatcher;
};

} // namespace Starfish

#endif
