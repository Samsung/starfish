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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPCSSDomain__)
#define __StarfishCDPCSSDomain__

#include <string>

namespace Starfish {

class CDPDispatcher;
class CDPCommand;

// CSS domain. getComputedStyleForNode returns a CSS name/value pair array
// gathered from the engine's ComputedStyleCSSStyleDeclaration over a fixed
// property whitelist (the computed declaration is resolved lazily per property,
// so it cannot be enumerated up-front). getInlineStylesForNode reads the
// element's parsed inline style declaration. getMatchedStylesForNode walks the
// StyleResolver's author+UA sheets and reports each style rule whose selector
// matches the element (tested via Element::matches), alongside the element's
// inline style. getStyleSheetText reconstructs a sheet's text by joining its
// rules' cssText (the original source string is discarded after parsing).
// Sheets are identified by their resolver index as "sheet-N". enable/disable
// ack only. Handlers run on the main thread.
class CSSDomain {
public:
    CSSDomain(CDPDispatcher* d)
        : m_dispatcher(d)
    {
    }
    void processMessage(CDPCommand& cmd, const std::string& method);

private:
    CDPDispatcher* m_dispatcher;
};

} // namespace Starfish

#endif
