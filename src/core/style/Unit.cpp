/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "Unit.h"

namespace Starfish {
namespace Unit {
    String* Color::toString() const
    {
        // css-color-4 #serializing-sRGB-values: legacy sRGB colors
        // serialize as rgb()/rgba(); the alpha uses the fewest decimals
        // that round-trip its 8-bit value.
        char buf[64];
        if (m_a == 255) {
            snprintf(buf, sizeof(buf), "rgb(%d, %d, %d)", m_r, m_g, m_b);
        } else {
            char alpha[16] = "0";
            if (m_a) {
                for (int decimals = 1; decimals <= 3; decimals++) {
                    snprintf(alpha, sizeof(alpha), "%.*f", decimals,
                             m_a / 255.0);
                    if ((int)round(atof(alpha) * 255) == m_a) {
                        break;
                    }
                }
            }
            snprintf(buf, sizeof(buf), "rgba(%d, %d, %d, %s)", m_r, m_g, m_b,
                     alpha);
        }
        return String::createASCIIString(buf, strnlen(buf, sizeof(buf)));
    }

    String* Color::toHTMLColorCodeString() const
    {
        char buf[256];
        snprintf(buf, sizeof(buf), "#%02x%02x%02x", m_r, m_g, m_b);
        return String::createASCIIString(buf, strnlen(buf, sizeof(buf)));
    }
} // namespace Unit
} // namespace Starfish
