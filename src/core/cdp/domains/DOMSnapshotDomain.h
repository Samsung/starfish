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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPDOMSnapshotDomain__)
#define __StarfishCDPDOMSnapshotDomain__

#include <string>

namespace Starfish {

class CDPDispatcher;
class CDPCommand;

// DOMSnapshot domain. captureSnapshot walks the main document pre-order and
// emits the CDP flattened form: a single shared strings table plus per-document
// parallel arrays (nodes.*) keyed by document order, with a layout sub-table
// for element nodes carrying their getBoundingClientRect bounds and the
// requested computedStyles (resolved via Element::getComputedStyle).
// getSnapshot returns the legacy nested form built from the same walk. Handlers
// run on the main thread. enable/disable ack only.
class DOMSnapshotDomain {
public:
    DOMSnapshotDomain(CDPDispatcher* d)
        : m_dispatcher(d)
    {
    }
    void processMessage(CDPCommand& cmd, const std::string& method);

private:
    CDPDispatcher* m_dispatcher;
};

} // namespace Starfish

#endif
