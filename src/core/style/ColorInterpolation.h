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

class ColorInterpolation {
public:
    // Color spaces of <color-interpolation-method> (css-color-5 #color-mix)
    // and of color() / lab() / lch() / oklab() / oklch() values.
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
        DisplayP3,
        DisplayP3Linear,
        A98Rgb,
        ProphotoRgb,
        Rec2020,
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
    // index of the hue component, or -1 for a rectangular space
    static int hueIndex(Space space);

    // hsl()/hwb() arguments (hue in degrees, others in percent) to sRGB
    static void hslToSrgb(const double hsl[3], double rgb[3]);
    static void hwbToSrgb(const double hwb[3], double rgb[3]);
};

// A <color> kept in a color space with unclipped float components
// (css-color-4 #color-type); a component may be missing (`none`,
// css-color-4 #missing-color-components). Legacy sRGB colors (hex, named,
// rgb(), hsl(), hwb()) are 8-bit Unit::Color instead; `m_legacy` marks a
// value that came from one so it serializes as rgb() again. Holds no
// pointers, so heap instances are allocated pointer-free.
struct ExtendedColor {
    // A non-legacy value is never in Hsl/Hwb/Xyz (kept as Srgb/XyzD65); a
    // legacy hsl()/hwb() with `none` components stays in its own space so
    // that they carry over into color-mix().
    ColorInterpolation::Space m_space;
    unsigned char m_missing; // bit i: component i is none; bit 3: alpha
    bool m_legacy;
    float m_c[3];
    float m_alpha;

    void* operator new(size_t size)
    {
        return GC_MALLOC_ATOMIC(size);
    }
    void* operator new(size_t, void* place)
    {
        return place;
    }
    void* operator new[](size_t size) = delete;

    ExtendedColor()
        : m_space(ColorInterpolation::Srgb)
        , m_missing(0)
        , m_legacy(true)
        , m_alpha(1)
    {
        m_c[0] = m_c[1] = m_c[2] = 0;
    }

    static ExtendedColor fromSrgb8(const Unit::Color& color)
    {
        ExtendedColor c;
        c.m_c[0] = color.R();
        c.m_c[1] = color.G();
        c.m_c[2] = color.B();
        c.m_alpha = color.A();
        return c;
    }

    bool missing(int i) const
    {
        return m_missing & (1 << i);
    }

    bool alphaMissing() const
    {
        return m_missing & (1 << 3);
    }

    // sRGB for painting: missing components are 0, out-of-gamut values are
    // clipped per channel.
    Unit::Color toSrgb8() const;
    // The same from full-precision components, for a parsed legacy color
    // whose 8-bit rounding must not see float storage error.
    static Unit::Color toSrgb8(ColorInterpolation::Space space,
                               const double components[3], double alpha);

    // css-color-4 #serializing-color-values: rgb()/rgba() for a legacy
    // color, otherwise lab()/lch()/oklab()/oklch()/color() with `none`
    // components spelled out.
    String* toString() const;
};

// css-color-4 #interpolation: both colors are converted to the
// interpolation space (a powerless or missing component is taken from the
// other color), premultiplied by alpha, mixed component-wise (hue by the
// chosen arc) and returned in that space, or in sRGB for hsl/hwb.
ExtendedColor mixColors(ColorInterpolation::Space space,
                        ColorInterpolation::HueMethod method,
                        const ExtendedColor& a, double weightA,
                        const ExtendedColor& b, double weightB,
                        double alphaMultiplier);

} // namespace Starfish

#endif
