/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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
#include "core/dom/IntersectionObserverEntry.h"

namespace Starfish {
IntersectionObserverEntry::IntersectionObserverEntry(
    ExecutionContext* executionContext,
    const IntersectionObserverEntryInit& intersectionObserverEntryInit)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
{
    initialize(intersectionObserverEntryInit);
}

ScriptBindingInstance* IntersectionObserverEntry::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

void IntersectionObserverEntry::initialize(
    const IntersectionObserverEntryInit& init)
{
    m_time = init.m_time;
    if (init.hasRootBounds()) {
        m_rootBounds = new DOMRectReadOnly(m_executionContext,
                                           init.rootBounds().getValue());
    }

    if (init.hasBoundingClientRect()) {
        m_boundingClientRect =
            new DOMRectReadOnly(m_executionContext, init.boundingClientRect());
    } else {
        m_boundingClientRect =
            new DOMRectReadOnly(m_executionContext, 0, 0, 0, 0);
    }

    if (init.hasIntersectionRect()) {
        m_intersectionRect =
            new DOMRectReadOnly(m_executionContext, init.intersectionRect());
    } else {
        m_intersectionRect =
            new DOMRectReadOnly(m_executionContext, 0, 0, 0, 0);
    }

    if (init.hasIsIntersecting()) {
        m_isIntersecting = init.isIntersecting();
    }

    if (init.hasIntersectionRatio()) {
        m_intersectionRatio = init.intersectionRatio();
    }

    if (init.hasTarget()) {
        m_target = init.target();
    }
}

} // namespace Starfish
