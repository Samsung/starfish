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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPAXFrameIdSource__)
#define __StarfishCDPAXFrameIdSource__

#include <string>

namespace Starfish {

class BrowsingContext;

// The protocol frame id of a browsing context, for the frameId of a root AX
// node. The Page domain owns frame ids and implements this, so the AX code
// does not depend on the domains built on top of it.
class AXFrameIdSource {
public:
    // Empty when the context is not a frame of the current target.
    virtual std::string frameIdForBrowsingContext(BrowsingContext* context) = 0;

protected:
    ~AXFrameIdSource()
    {
    }
};

} // namespace Starfish

#endif
