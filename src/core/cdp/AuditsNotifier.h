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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPAuditsNotifier__)
#define __StarfishCDPAuditsNotifier__

#include <string>

namespace Starfish {

class CDPDispatcher;
class Document;
class Element;

// The top-level document finished loading. Check it for problems that the
// Audits domain reports and send each one as Audits.issueAdded. Issues are
// also kept, so a client that enables Audits later receives them.
void notifyAuditsDocumentLoaded(Document* document);

// The HTML parser closed `select` because an <input> start tag came inside
// it. The element never becomes a child, so only the parser can report it.
void notifyAuditsSelectClosedByInput(Element* select);

// Send the kept issues to a session that just enabled the Audits domain.
void replayAuditsIssues(CDPDispatcher* dispatcher,
                        const std::string& sessionId);

} // namespace Starfish

#endif
