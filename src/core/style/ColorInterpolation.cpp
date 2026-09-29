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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/style/ColorInterpolation.h"

#include <cmath>

namespace Starfish {

namespace {

    // Matrices and constants from the css-color-4 sample code
    // (https://drafts.csswg.org/css-color-4/#color-conversion-code).

    const double kLinearSrgbToXyzD65[3][3] = {
        { 0.41239079926595934, 0.357584339383878, 0.1804807884018343 },
        { 0.21263900587151027, 0.715168678767756, 0.07219231536073371 },
        { 0.01933081871559182, 0.11919477979462598, 0.9505321522496607 },
    };

    const double kXyzD65ToLinearSrgb[3][3] = {
        { 3.2409699419045226, -1.537383177570094, -0.4986107602930034 },
        { -0.9692436362808796, 1.8759675015077202, 0.04155505740717559 },
        { 0.05563007969699366, -0.20397695888897652, 1.0569715142428786 },
    };

    const double kXyzD65ToD50[3][3] = {
        { 1.0479298208405488, 0.022946793341019088, -0.05019222954313557 },
        { 0.029627815688159344, 0.990434484573249, -0.01707382502938514 },
        { -0.009243058152591178, 0.015055144896577895, 0.7518742899580008 },
    };

    const double kXyzD50ToD65[3][3] = {
        { 0.9554734527042182, -0.023098536874261423, 0.0632593086610217 },
        { -0.028369706963208136, 1.0099954580058226, 0.021041398966943008 },
        { 0.012314001688319899, -0.020507696433477912, 1.3303659366080753 },
    };

    const double kXyzD65ToOklabLms[3][3] = {
        { 0.8190224379967030, 0.3619062600528904, -0.1288737815209879 },
        { 0.0329836539323885, 0.9292868615863434, 0.0361446663127164 },
        { 0.0481771893596242, 0.2642395317527308, 0.6335478284694309 },
    };

    const double kOklabLmsToOklab[3][3] = {
        { 0.2104542683093140, 0.7936177747023054, -0.0040720430116193 },
        { 1.9779985324311684, -2.4285922420485799, 0.4505937096174110 },
        { 0.0259040424655478, 0.7827717124575296, -0.8086757549230774 },
    };

    const double kOklabToOklabLms[3][3] = {
        { 1.0000000000000000, 0.3963377773761749, 0.2158037573099136 },
        { 1.0000000000000000, -0.1055613458156586, -0.0638541728258133 },
        { 1.0000000000000000, -0.0894841775298119, -1.2914855480194092 },
    };

    const double kOklabLmsToXyzD65[3][3] = {
        { 1.2268798758459243, -0.5578149944602171, 0.2813910456659647 },
        { -0.0405757452148008, 1.1122868032803170, -0.0717110580655164 },
        { -0.0763729366746601, -0.4214933324022432, 1.5869240198367816 },
    };

    // D50 reference white
    const double kD50[3] = { 0.3457 / 0.3585, 1.0,
                             (1.0 - 0.3457 - 0.3585) / 0.3585 };

    const double kPi = 3.14159265358979323846;

    void multiply(const double m[3][3], const double in[3], double out[3])
    {
        double r[3];
        for (int i = 0; i < 3; i++) {
            r[i] = m[i][0] * in[0] + m[i][1] * in[1] + m[i][2] * in[2];
        }
        out[0] = r[0];
        out[1] = r[1];
        out[2] = r[2];
    }

    double srgbToLinear(double c)
    {
        double sign = c < 0 ? -1 : 1;
        double abs = std::fabs(c);
        if (abs <= 0.04045) {
            return c / 12.92;
        }
        return sign * std::pow((abs + 0.055) / 1.055, 2.4);
    }

    double linearToSrgb(double c)
    {
        double sign = c < 0 ? -1 : 1;
        double abs = std::fabs(c);
        if (abs <= 0.0031308) {
            return c * 12.92;
        }
        return sign * (1.055 * std::pow(abs, 1 / 2.4) - 0.055);
    }

    double normalizeHue(double h)
    {
        h = std::fmod(h, 360.0);
        return h < 0 ? h + 360.0 : h;
    }

    void rgbToHsl(const double rgb[3], double hsl[3])
    {
        double r = rgb[0], g = rgb[1], b = rgb[2];
        double max = std::max(r, std::max(g, b));
        double min = std::min(r, std::min(g, b));
        double l = (max + min) / 2;
        double d = max - min;
        double h = 0, s = 0;
        if (d != 0) {
            s = (l == 0 || l == 1) ? 0 : (max - l) / std::min(l, 1 - l);
            if (max == r) {
                h = (g - b) / d + (g < b ? 6 : 0);
            } else if (max == g) {
                h = (b - r) / d + 2;
            } else {
                h = (r - g) / d + 4;
            }
            h *= 60;
        }
        hsl[0] = normalizeHue(h);
        hsl[1] = s * 100;
        hsl[2] = l * 100;
    }

    void hslToRgb(const double hsl[3], double rgb[3])
    {
        double h = normalizeHue(hsl[0]);
        double s = hsl[1] / 100;
        double l = hsl[2] / 100;
        double a = s * std::min(l, 1 - l);
        for (int i = 0; i < 3; i++) {
            double n = i == 0 ? 0 : (i == 1 ? 8 : 4);
            double k = std::fmod(n + h / 30, 12.0);
            rgb[i] =
                l - a * std::max(-1.0, std::min(std::min(k - 3, 9 - k), 1.0));
        }
    }

    void rgbToHwb(const double rgb[3], double hwb[3])
    {
        double hsl[3];
        rgbToHsl(rgb, hsl);
        hwb[0] = hsl[0];
        hwb[1] = std::min(rgb[0], std::min(rgb[1], rgb[2])) * 100;
        hwb[2] = (1 - std::max(rgb[0], std::max(rgb[1], rgb[2]))) * 100;
    }

    void hwbToRgb(const double hwb[3], double rgb[3])
    {
        double w = hwb[1] / 100;
        double b = hwb[2] / 100;
        if (w + b >= 1) {
            double gray = w / (w + b);
            rgb[0] = rgb[1] = rgb[2] = gray;
            return;
        }
        double hsl[3] = { hwb[0], 100, 50 };
        hslToRgb(hsl, rgb);
        for (int i = 0; i < 3; i++) {
            rgb[i] = rgb[i] * (1 - w - b) + w;
        }
    }

    void xyzD50ToLab(const double xyz[3], double lab[3])
    {
        const double e = 216.0 / 24389.0;
        const double k = 24389.0 / 27.0;
        double f[3];
        for (int i = 0; i < 3; i++) {
            double t = xyz[i] / kD50[i];
            f[i] = t > e ? std::cbrt(t) : (k * t + 16) / 116;
        }
        lab[0] = 116 * f[1] - 16;
        lab[1] = 500 * (f[0] - f[1]);
        lab[2] = 200 * (f[1] - f[2]);
    }

    void labToXyzD50(const double lab[3], double xyz[3])
    {
        const double e = 216.0 / 24389.0;
        const double k = 24389.0 / 27.0;
        double fy = (lab[0] + 16) / 116;
        double fx = lab[1] / 500 + fy;
        double fz = fy - lab[2] / 200;
        double fx3 = fx * fx * fx;
        double fz3 = fz * fz * fz;
        xyz[0] = (fx3 > e ? fx3 : (116 * fx - 16) / k) * kD50[0];
        xyz[1] =
            (lab[0] > k * e ? std::pow((lab[0] + 16) / 116, 3) : lab[0] / k) *
            kD50[1];
        xyz[2] = (fz3 > e ? fz3 : (116 * fz - 16) / k) * kD50[2];
    }

    void xyzD65ToOklab(const double xyz[3], double oklab[3])
    {
        double lms[3];
        multiply(kXyzD65ToOklabLms, xyz, lms);
        for (int i = 0; i < 3; i++) {
            lms[i] = std::cbrt(lms[i]);
        }
        multiply(kOklabLmsToOklab, lms, oklab);
    }

    void oklabToXyzD65(const double oklab[3], double xyz[3])
    {
        double lms[3];
        multiply(kOklabToOklabLms, oklab, lms);
        for (int i = 0; i < 3; i++) {
            lms[i] = lms[i] * lms[i] * lms[i];
        }
        multiply(kOklabLmsToXyzD65, lms, xyz);
    }

    void labToLch(const double lab[3], double lch[3])
    {
        double c = std::hypot(lab[1], lab[2]);
        double h = std::atan2(lab[2], lab[1]) * 180 / kPi;
        lch[0] = lab[0];
        lch[1] = c;
        lch[2] = normalizeHue(h);
    }

    void lchToLab(const double lch[3], double lab[3])
    {
        double h = lch[2] * kPi / 180;
        lab[0] = lch[0];
        lab[1] = lch[1] * std::cos(h);
        lab[2] = lch[1] * std::sin(h);
    }

    int hueIndex(ColorInterpolation::Space space)
    {
        switch (space) {
        case ColorInterpolation::Hsl:
        case ColorInterpolation::Hwb:
            return 0;
        case ColorInterpolation::Lch:
        case ColorInterpolation::Oklch:
            return 2;
        default:
            return -1;
        }
    }

    // Converts an sRGB color to `space`; returns whether the hue component is
    // powerless (css-color-4 #interpolation-missing: it then takes the other
    // color's hue).
    bool toSpace(ColorInterpolation::Space space, const Unit::Color& color,
                 double out[3])
    {
        double rgb[3] = { color.R(), color.G(), color.B() };
        switch (space) {
        case ColorInterpolation::Srgb:
            out[0] = rgb[0];
            out[1] = rgb[1];
            out[2] = rgb[2];
            return false;
        case ColorInterpolation::Hsl:
            rgbToHsl(rgb, out);
            return out[1] == 0 || out[2] == 0 || out[2] == 100;
        case ColorInterpolation::Hwb:
            rgbToHwb(rgb, out);
            return out[1] + out[2] >= 100;
        default:
            break;
        }

        double linear[3];
        for (int i = 0; i < 3; i++) {
            linear[i] = srgbToLinear(rgb[i]);
        }
        if (space == ColorInterpolation::SrgbLinear) {
            out[0] = linear[0];
            out[1] = linear[1];
            out[2] = linear[2];
            return false;
        }

        double xyz[3];
        multiply(kLinearSrgbToXyzD65, linear, xyz);
        switch (space) {
        case ColorInterpolation::Xyz:
        case ColorInterpolation::XyzD65:
            out[0] = xyz[0];
            out[1] = xyz[1];
            out[2] = xyz[2];
            return false;
        case ColorInterpolation::XyzD50:
            multiply(kXyzD65ToD50, xyz, out);
            return false;
        case ColorInterpolation::Lab:
        case ColorInterpolation::Lch: {
            double d50[3], lab[3];
            multiply(kXyzD65ToD50, xyz, d50);
            xyzD50ToLab(d50, lab);
            if (space == ColorInterpolation::Lab) {
                out[0] = lab[0];
                out[1] = lab[1];
                out[2] = lab[2];
                return false;
            }
            labToLch(lab, out);
            return out[1] < 1e-4;
        }
        case ColorInterpolation::Oklab:
        case ColorInterpolation::Oklch: {
            double oklab[3];
            xyzD65ToOklab(xyz, oklab);
            if (space == ColorInterpolation::Oklab) {
                out[0] = oklab[0];
                out[1] = oklab[1];
                out[2] = oklab[2];
                return false;
            }
            labToLch(oklab, out);
            return out[1] < 1e-4;
        }
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
            return false;
        }
    }

    void fromSpace(ColorInterpolation::Space space, const double in[3],
                   double rgb[3])
    {
        double xyz[3];
        switch (space) {
        case ColorInterpolation::Srgb:
            rgb[0] = in[0];
            rgb[1] = in[1];
            rgb[2] = in[2];
            return;
        case ColorInterpolation::Hsl:
            hslToRgb(in, rgb);
            return;
        case ColorInterpolation::Hwb:
            hwbToRgb(in, rgb);
            return;
        case ColorInterpolation::SrgbLinear:
            for (int i = 0; i < 3; i++) {
                rgb[i] = linearToSrgb(in[i]);
            }
            return;
        case ColorInterpolation::Xyz:
        case ColorInterpolation::XyzD65:
            xyz[0] = in[0];
            xyz[1] = in[1];
            xyz[2] = in[2];
            break;
        case ColorInterpolation::XyzD50:
            multiply(kXyzD50ToD65, in, xyz);
            break;
        case ColorInterpolation::Lab:
        case ColorInterpolation::Lch: {
            double lab[3], d50[3];
            if (space == ColorInterpolation::Lch) {
                lchToLab(in, lab);
            } else {
                lab[0] = in[0];
                lab[1] = in[1];
                lab[2] = in[2];
            }
            labToXyzD50(lab, d50);
            multiply(kXyzD50ToD65, d50, xyz);
            break;
        }
        case ColorInterpolation::Oklab:
        case ColorInterpolation::Oklch: {
            double oklab[3];
            if (space == ColorInterpolation::Oklch) {
                lchToLab(in, oklab);
            } else {
                oklab[0] = in[0];
                oklab[1] = in[1];
                oklab[2] = in[2];
            }
            oklabToXyzD65(oklab, xyz);
            break;
        }
        }
        double linear[3];
        multiply(kXyzD65ToLinearSrgb, xyz, linear);
        for (int i = 0; i < 3; i++) {
            rgb[i] = linearToSrgb(linear[i]);
        }
    }

    // css-color-4 #hue-interpolation
    void adjustHues(ColorInterpolation::HueMethod method, double* h1,
                    double* h2)
    {
        *h1 = normalizeHue(*h1);
        *h2 = normalizeHue(*h2);
        double d = *h2 - *h1;
        switch (method) {
        case ColorInterpolation::Shorter:
            if (d > 180) {
                *h1 += 360;
            } else if (d < -180) {
                *h2 += 360;
            }
            break;
        case ColorInterpolation::Longer:
            if (0 < d && d < 180) {
                *h1 += 360;
            } else if (-180 < d && d <= 0) {
                *h2 += 360;
            }
            break;
        case ColorInterpolation::Increasing:
            if (*h2 < *h1) {
                *h2 += 360;
            }
            break;
        case ColorInterpolation::Decreasing:
            if (*h1 < *h2) {
                *h1 += 360;
            }
            break;
        }
    }

    unsigned char toChannel(double v)
    {
        if (v <= 0) {
            return 0;
        }
        if (v >= 1) {
            return 255;
        }
        return (unsigned char)std::lround(v * 255);
    }

} // namespace

bool ColorInterpolation::parseSpace(const std::string& name, Space* space)
{
    struct {
        const char* name;
        Space space;
    } const table[] = {
        { "srgb", Srgb },      { "srgb-linear", SrgbLinear },
        { "hsl", Hsl },        { "hwb", Hwb },
        { "lab", Lab },        { "oklab", Oklab },
        { "lch", Lch },        { "oklch", Oklch },
        { "xyz", Xyz },        { "xyz-d50", XyzD50 },
        { "xyz-d65", XyzD65 },
    };
    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        if (name == table[i].name) {
            *space = table[i].space;
            return true;
        }
    }
    return false;
}

bool ColorInterpolation::parseHueMethod(const std::string& name,
                                        HueMethod* method)
{
    if (name == "shorter") {
        *method = Shorter;
    } else if (name == "longer") {
        *method = Longer;
    } else if (name == "increasing") {
        *method = Increasing;
    } else if (name == "decreasing") {
        *method = Decreasing;
    } else {
        return false;
    }
    return true;
}

const char* ColorInterpolation::spaceName(Space space)
{
    switch (space) {
    case Srgb:
        return "srgb";
    case SrgbLinear:
        return "srgb-linear";
    case Hsl:
        return "hsl";
    case Hwb:
        return "hwb";
    case Lab:
        return "lab";
    case Oklab:
        return "oklab";
    case Lch:
        return "lch";
    case Oklch:
        return "oklch";
    case Xyz:
        return "xyz";
    case XyzD50:
        return "xyz-d50";
    case XyzD65:
        return "xyz-d65";
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return "";
}

const char* ColorInterpolation::hueMethodName(HueMethod method)
{
    switch (method) {
    case Shorter:
        return "shorter";
    case Longer:
        return "longer";
    case Increasing:
        return "increasing";
    case Decreasing:
        return "decreasing";
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return "";
}

bool ColorInterpolation::isPolar(Space space)
{
    return hueIndex(space) >= 0;
}

Unit::Color ColorInterpolation::mix(Space space, HueMethod method,
                                    const Unit::Color& a, double weightA,
                                    const Unit::Color& b, double weightB,
                                    double alphaMultiplier)
{
    double ca[3], cb[3];
    bool hueMissingA = toSpace(space, a, ca);
    bool hueMissingB = toSpace(space, b, cb);
    double alphaA = a.A();
    double alphaB = b.A();

    int hue = hueIndex(space);
    if (hue >= 0) {
        if (hueMissingA && !hueMissingB) {
            ca[hue] = cb[hue];
        } else if (hueMissingB && !hueMissingA) {
            cb[hue] = ca[hue];
        }
        adjustHues(method, &ca[hue], &cb[hue]);
    }

    double alpha = alphaA * weightA + alphaB * weightB;
    double out[3];
    for (int i = 0; i < 3; i++) {
        if (i == hue) {
            out[i] = ca[i] * weightA + cb[i] * weightB;
        } else {
            // premultiplied interpolation (css-color-4 #interpolation-alpha)
            double p = ca[i] * alphaA * weightA + cb[i] * alphaB * weightB;
            out[i] = alpha > 0 ? p / alpha : 0;
        }
    }
    if (hue >= 0) {
        out[hue] = normalizeHue(out[hue]);
    }

    double rgb[3];
    fromSpace(space, out, rgb);
    return Unit::Color(toChannel(rgb[0]), toChannel(rgb[1]), toChannel(rgb[2]),
                       toChannel(alpha * alphaMultiplier));
}

} // namespace Starfish
