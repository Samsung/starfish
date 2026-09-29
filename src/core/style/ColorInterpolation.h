/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishColorInterpolation__
#define __StarfishColorInterpolation__

#include "core/style/Unit.h"

namespace Starfish {

// Color interpolation per css-color-4 #interpolation: both colors are
// converted to the interpolation color space, premultiplied by alpha,
// mixed component-wise (hue by the chosen arc), and converted back to
// sRGB. Results outside the sRGB gamut are clipped per channel.
class ColorInterpolation {
public:
    // <color-interpolation-method> color spaces (css-color-5 #color-mix)
    enum Space : unsigned char {
        Srgb,
        SrgbLinear,
        Hsl,
        Hwb,
        Lab,
        Oklab,
        Lch,
        Oklch,
        Xyz,
        XyzD50,
        XyzD65,
    };

    enum HueMethod : unsigned char {
        Shorter,
        Longer,
        Increasing,
        Decreasing,
    };

    // `name` is ASCII-lowercased
    static bool parseSpace(const std::string& name, Space* space);
    static bool parseHueMethod(const std::string& name, HueMethod* method);
    static const char* spaceName(Space space);
    static const char* hueMethodName(HueMethod method);
    static bool isPolar(Space space);

    // Weights sum to 1; `alphaMultiplier` scales the resulting alpha
    // (color-mix() percentages adding up to less than 100%).
    static Unit::Color mix(Space space, HueMethod method, const Unit::Color& a,
                           double weightA, const Unit::Color& b, double weightB,
                           double alphaMultiplier);
};

} // namespace Starfish

#endif
