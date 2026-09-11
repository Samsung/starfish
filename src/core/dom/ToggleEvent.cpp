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
#include "core/dom/ShadowRoot.h"

namespace Starfish {
Optional<Element*> ToggleEvent::source()
{
    // HTML's ToggleEvent source getter retargets across shadow boundaries.
    auto source = m_source;
    while (source) {
        Node* root = source->getRootNode();
        if (!root->isShadowRoot()) {
            return source;
        }
        Node* target = currentTarget() && currentTarget()->isNode()
                           ? currentTarget()->asNode()
                           : nullptr;
        while (target) {
            if (root->contains(target)) {
                return source;
            }
            auto targetRoot = target->getRootNode();
            if (!targetRoot->isShadowRoot()) {
                break;
            }
            target = targetRoot->asShadowRoot()->host();
        }
        source = root->asShadowRoot()->host();
    }
    return NullOption;
}
} // namespace Starfish
