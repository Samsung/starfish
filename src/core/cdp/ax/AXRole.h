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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPAXRole__)
#define __StarfishCDPAXRole__

#include <string>

namespace Starfish {

class Element;

// The ARIA role of an element. An explicit role attribute wins; otherwise
// the role comes from the HTML-AAM mapping for the tag. Returns an empty
// string for an element that carries no role of its own.
std::string axRoleForElement(Element* element, const std::string& tag);

// Whether the author asked for the element's own semantics to be dropped
// with role="none" or role="presentation".
bool axHasPresentationalRole(Element* element);

// The numeric role Chromium reports as chromeRole. The values are the
// ax::mojom::Role enum, which clients read as a stable machine-facing id.
int axChromeRoleForRole(const std::string& role);

// Roles whose accessible name comes from the element's own content, and
// roles that may not be named at all. Both groups come from the ARIA role
// definitions Chromium consumes.
bool axRoleSupportsNameFromContents(const std::string& role);
bool axRoleIsNameProhibited(const std::string& role);

} // namespace Starfish

#endif
