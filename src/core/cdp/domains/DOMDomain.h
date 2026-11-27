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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPDOMDomain__)
#define __StarfishCDPDOMDomain__

#include "../CDPDomain.h"
#include <string>

namespace Starfish {

class CDPDispatcher;
class CDPCommand;

class DOMDomain : public CDPDomain {
public:
    DOMDomain(CDPDispatcher* d)
        : m_dispatcher(d)
    {
    }
    void processMessage(CDPCommand& cmd, const std::string& method) override;

private:
    void handleEnable(CDPCommand& cmd, const std::string& method);
    void handleDisable(CDPCommand& cmd, const std::string& method);
    void handleGetDocument(CDPCommand& cmd, const std::string& method);
    void handleRequestChildNodes(CDPCommand& cmd, const std::string& method);
    void handleQuerySelector(CDPCommand& cmd, const std::string& method);
    void handleQuerySelectorAll(CDPCommand& cmd, const std::string& method);
    void handleDescribeNode(CDPCommand& cmd, const std::string& method);
    void handleGetOuterHTML(CDPCommand& cmd, const std::string& method);
    void handleSetOuterHTML(CDPCommand& cmd, const std::string& method);
    void handleFocus(CDPCommand& cmd, const std::string& method);
    void handleScrollIntoViewIfNeeded(CDPCommand& cmd,
                                      const std::string& method);
    void handleGetAttributes(CDPCommand& cmd, const std::string& method);
    void handleSetAttributeValue(CDPCommand& cmd, const std::string& method);
    void handleRemoveAttribute(CDPCommand& cmd, const std::string& method);
    void handleSetNodeValue(CDPCommand& cmd, const std::string& method);
    void handleRemoveNode(CDPCommand& cmd, const std::string& method);
    void handleResolveNode(CDPCommand& cmd, const std::string& method);
    void handleSetFileInputFiles(CDPCommand& cmd, const std::string& method);
    void handleGetBoxModel(CDPCommand& cmd, const std::string& method);
    void handlePerformSearch(CDPCommand& cmd, const std::string& method);
    void handleGetSearchResults(CDPCommand& cmd, const std::string& method);
    void handleDiscardSearchResults(CDPCommand& cmd, const std::string& method);
    void handleCollectClassNamesFromSubtree(CDPCommand& cmd,
                                            const std::string& method);
    void handleGetNodeForLocation(CDPCommand& cmd, const std::string& method);
    void handleCopyTo(CDPCommand& cmd, const std::string& method);
    void handleGetFlattenedDocument(CDPCommand& cmd, const std::string& method);
    void handleSetNodeName(CDPCommand& cmd, const std::string& method);
    void handleRequestNode(CDPCommand& cmd, const std::string& method);
    void handleMarkUndoableState(CDPCommand& cmd, const std::string& method);
    void handleGetNodeStackTraces(CDPCommand& cmd, const std::string& method);
    void handlePushNodeByPathToFrontend(CDPCommand& cmd,
                                        const std::string& method);
    void handlePushNodesByBackendIdsToFrontend(CDPCommand& cmd,
                                               const std::string& method);

    CDPDispatcher* m_dispatcher;
};

} // namespace Starfish

#endif
