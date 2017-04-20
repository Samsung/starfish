/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "dom/DOMRect.h"
#include "dom/DOMRectList.h"
#include "dom/DOMQuad.h"

namespace StarFish {

DOMRectList::DOMRectList()
    : ScriptWrappable(this)
{
}

DOMRectList::DOMRectList(const std::vector<DOMQuad>& quads)
    : ScriptWrappable(this)
{
    m_list.reserve(quads.size());
    for (size_t i = 0; i < quads.size(); ++i) {
        m_list.push_back(new DOMRect(quads[i].bounds()));
    }
}

unsigned long DOMRectList::length() const
{
    return m_list.size();
}

DOMRect* DOMRectList::item(unsigned long index)
{
    if (index >= m_list.size()) {
        return nullptr;
    }

    return m_list[index];
}
}
