/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "Unit.h"

namespace StarFish {
namespace Unit {
    String* Color::toString() const
    {
        char buf[256];
        if (m_isHsl) {
            double hVal = 0, sVal = 0, lVal = 0;
            toHsl(&hVal, &sVal, &lVal);
            // floats are rounded to int
            int h = (int)round(hVal * 360);
            int s = (int)round(sVal * 100);
            int l = (int)round(lVal * 100);

            if (m_a == 255) {
                snprintf(buf, sizeof(buf), "hsl(%d, %d%%, %d%%)", h, s, l);
            } else {
                float a = (float)m_a / 255;
                if (a > 0.05) {
                    snprintf(buf, sizeof(buf), "hsla(%d, %d%%, %d%%, %.1f)", h,
                             s, l, a);
                } else {
                    snprintf(buf, sizeof(buf), "hsla(%d, %d%%, %d%%, 0)", h, s,
                             l);
                }
            }
        } else {
            // rgb
            if (m_a == 255) {
                snprintf(buf, sizeof(buf), "rgb(%d, %d, %d)", m_r, m_g, m_b);
            } else {
                float a = (float)m_a / 255;
                if (a > 0.05) {
                    snprintf(buf, sizeof(buf), "rgba(%d, %d, %d, %.1f)", m_r,
                             m_g, m_b, a);
                } else {
                    snprintf(buf, sizeof(buf), "rgba(%d, %d, %d, 0)", m_r, m_g,
                             m_b);
                }
            }
        }

        String* toStr = String::createASCIIString(buf);
        return toStr;
    }
}
}
