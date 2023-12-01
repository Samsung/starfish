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
#include "ResizeObserverEntry.h"
#include "core/dom/DOMRectReadOnly.h"

namespace Starfish {

ResizeObserverEntry::ResizeObserverEntry(ExecutionContext* executionContext,
                                         DOMRectReadOnly* contentRect,
                                         Element* target)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_contentRect(contentRect)
    , m_target(target)
{
}

ScriptBindingInstance* ResizeObserverEntry::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

DOMRectReadOnly* ResizeObserverEntry::contentRect() const
{
    return m_contentRect;
}

Element* ResizeObserverEntry::target() const
{
    return m_target;
}

} // namespace Starfish
