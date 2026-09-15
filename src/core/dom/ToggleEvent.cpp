/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "core/dom/ToggleEvent.h"
#include "core/dom/Element.h"
#include "core/dom/EventTarget.h"

namespace Starfish {
Optional<Element*> ToggleEvent::source()
{
    // HTML ToggleEvent: the source getter retargets source against the
    // event's currentTarget, so a node inside a shadow tree the listener
    // cannot see is reported as that tree's host instead.
    if (!m_source) {
        return NullOption;
    }
    EventTarget* retargeted =
        EventTarget::retarget(m_source.value(), currentTarget());
    STARFISH_ASSERT(retargeted->isNode() && retargeted->asNode()->isElement());
    return retargeted->asNode()->asElement();
}
} // namespace Starfish
