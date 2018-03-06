/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#include "StarFishConfig.h"
#include "core/dom/DOMRect.h"
#include "core/dom/DOMRectList.h"
#include "core/dom/DOMQuad.h"
#include "core/dom/Document.h"

namespace StarFish {

DOMRectList::DOMRectList(Document* document)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(document->scriptBindingInstance())
{
}

DOMRectList::DOMRectList(Document* document, const GCVector<DOMQuad*>& quads)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(document->scriptBindingInstance())
{
    m_list.reserve(quads.size());
    for (size_t i = 0; i < quads.size(); ++i) {
        m_list.push_back(quads[i]->getBounds());
    }
}

uint32_t DOMRectList::length() const
{
    return m_list.size();
}

DOMRect* DOMRectList::item(uint32_t index)
{
    if (index >= m_list.size()) {
        return nullptr;
    }

    return m_list[index];
}
}
