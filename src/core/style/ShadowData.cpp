/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "ShadowData.h"
#include "core/modules/canvas/CanvasShadowData.h"

namespace StarFish {

CanvasShadowData ShadowData::toCanvasShadowData(Frame* owner)
{
    STARFISH_ASSERT(owner);
    LayoutUnit unused;
    float offsetX = m_offsetX.specifiedValue(unused, owner);
    float offsetY = m_offsetY.specifiedValue(unused, owner);
    float radius = 0.0f;
    if (m_radius.isFixed() && m_radius.fixed()) {
        radius = m_radius.fixed();
    }
    CanvasShadowData ret(offsetX, offsetY, radius, m_color, m_hasColor);
    return ret;
}

CanvasShadowDataList ShadowDataList::toCanvasShadowDataList(Frame* owner)
{
    STARFISH_ASSERT(owner);

    CanvasShadowDataList ret;
    ret.reserve(size());

    for (size_t i = 0; i < size(); i++) {
        ret.push_back(at(i).toCanvasShadowData(owner));
    }
    return ret;
}
}
