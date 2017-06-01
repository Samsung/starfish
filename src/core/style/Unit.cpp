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

#include "StarFishConfig.h"
#include "Unit.h"

namespace StarFish {
namespace Unit {
    String* Color::toString() const
    {
        char buf[256];
        if (m_a == 255) {
            snprintf(buf, sizeof(buf), "rgb(%d, %d, %d)", m_r, m_g, m_b);
        } else {
            float a = (float)m_a / 255;
            if (a > 0.05) {
                snprintf(buf, sizeof(buf), "rgba(%d, %d, %d, %.1f)", m_r, m_g,
                         m_b, a);
            } else {
                snprintf(buf, sizeof(buf), "rgba(%d, %d, %d, 0)", m_r, m_g,
                         m_b);
            }
        }

        String* toStr = String::createASCIIString(buf);
        return toStr;
    }
}
}
