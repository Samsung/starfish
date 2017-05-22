/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "core/modules/canvas/font/Font.h"
#include "core/style/Length.h"

namespace StarFish {
void Length::changeToFixedIfNeeded(Length fontSize, Font* font)
{
    if (!isComputed()) {
        float fSize = 0.0f;
        if (fontSize.isFixed()) {
            fSize = fontSize.fixed();
        }
        if (m_type == EmToBeFixed) {
            m_data = fSize * m_data;
            m_type = Fixed;
        } else if (m_type == ExToBeFixed) {
            m_data = fSize * m_data * font->metrics().m_xheightRate;
            m_type = Fixed;
        }
        // InheritableNumber does not change its value
    }
}

void LengthSize::checkComputed(Length fontSize, Font* font)
{
    m_width.changeToFixedIfNeeded(fontSize, font);
    m_height.changeToFixedIfNeeded(fontSize, font);
}

void LengthPosition::checkComputed(Length fontSize, Font* font)
{
    m_x.changeToFixedIfNeeded(fontSize, font);
    m_y.changeToFixedIfNeeded(fontSize, font);
}

void LengthBox::checkComputed(Length fontSize, Font* font)
{
    m_left.changeToFixedIfNeeded(fontSize, font);
    m_right.changeToFixedIfNeeded(fontSize, font);
    m_top.changeToFixedIfNeeded(fontSize, font);
    m_bottom.changeToFixedIfNeeded(fontSize, font);
}
}
