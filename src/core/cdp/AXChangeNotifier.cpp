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
#include "AXChangeNotifier.h"
#include "CDPDispatcher.h"
#include "CDPServer.h"
#include "CDPSession.h"
#include "TargetContext.h"
#include "domains/AccessibilityDomain.h"
#include "core/dom/Document.h"
#include "core/dom/Node.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"

namespace Starfish {

bool g_axNodeChangesObserved = false;

void notifyAXNodeChanged(Node* node)
{
    if (!node) {
        return;
    }
    Document* document = node->document();
    if (!document) {
        return;
    }
    BrowsingContext* context = document->browsingContext();
    if (!context || !context->webView()) {
        return;
    }
    // A page with no client attached has no server, which is the common
    // case and the reason this check comes before anything else.
    CDPServer* server = context->webView()->cdpServer();
    if (!server || !server->dispatcher()) {
        return;
    }
    server->dispatcher()->accessibility()->markNodeDirty(node);
}

void notifyAXLoadComplete(Document* document)
{
    if (!document) {
        return;
    }
    BrowsingContext* context = document->browsingContext();
    if (!context || !context->webView()) {
        return;
    }
    CDPServer* server = context->webView()->cdpServer();
    if (!server || !server->dispatcher()) {
        return;
    }
    TargetContext* target =
        server->dispatcher()->contextForWebView(context->webView());
    if (!target || !target->session) {
        return;
    }
    server->dispatcher()->accessibility()->emitLoadComplete(
        target->session->sessionId);
}

} // namespace Starfish

#endif
