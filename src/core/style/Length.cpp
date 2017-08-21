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
#include "core/modules/canvas/font/Font.h"
#include "core/style/Length.h"

namespace StarFish {
void Length::changeToFixedIfNeeded(Length curFontSize, Length rootFontSize,
                                   Font* font)
{
    if (!isComputed()) {
        if (m_type == EmToBeFixed) {
            float fSize = curFontSize.isFixed() ? curFontSize.fixed() : 0.0f;
            m_data = fSize * m_data;
            m_type = Fixed;
        } else if (m_type == ExToBeFixed) {
            float fSize = curFontSize.isFixed() ? curFontSize.fixed() : 0.0f;
            m_data = fSize * m_data * font->metrics().m_xheightRate;
            m_type = Fixed;
        } else if (m_type == RemToBeFixed) {
            float fSize = rootFontSize.isFixed() ? rootFontSize.fixed() : 0.0f;
            m_data = fSize * m_data;
            m_type = Fixed;
        }
        // InheritableNumber does not change its value
    }
}

void LengthSize::checkComputed(Length curFontSize, Length rootFontSize,
                               Font* font)
{
    m_width.changeToFixedIfNeeded(curFontSize, rootFontSize, font);
    m_height.changeToFixedIfNeeded(curFontSize, rootFontSize, font);
}

void LengthPosition::checkComputed(Length curFontSize, Length rootFontSize,
                                   Font* font)
{
    m_x.changeToFixedIfNeeded(curFontSize, rootFontSize, font);
    m_y.changeToFixedIfNeeded(curFontSize, rootFontSize, font);
}

void LengthBox::checkComputed(Length curFontSize, Length rootFontSize,
                              Font* font)
{
    m_left.changeToFixedIfNeeded(curFontSize, rootFontSize, font);
    m_right.changeToFixedIfNeeded(curFontSize, rootFontSize, font);
    m_top.changeToFixedIfNeeded(curFontSize, rootFontSize, font);
    m_bottom.changeToFixedIfNeeded(curFontSize, rootFontSize, font);
}
}
