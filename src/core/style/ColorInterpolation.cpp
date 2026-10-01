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

    // Predefined RGB spaces (css-color-4 #predefined), matrices as the exact
    // rationals of the sample code.
    const double kLinearP3ToXyzD65[3][3] = {
        { 608311.0 / 1250200, 189793.0 / 714400, 198249.0 / 1000160 },
        { 35783.0 / 156275, 247089.0 / 357200, 198249.0 / 2500400 },
        { 0.0, 32229.0 / 714400, 5220557.0 / 5000800 },
    };

    const double kXyzD65ToLinearP3[3][3] = {
        { 446124.0 / 178915, -333277.0 / 357830, -72051.0 / 178915 },
        { -14852.0 / 17905, 63121.0 / 35810, 423.0 / 17905 },
        { 11844.0 / 330415, -50337.0 / 660830, 316169.0 / 330415 },
    };

    const double kLinearA98ToXyzD65[3][3] = {
        { 573536.0 / 994567, 263643.0 / 1420810, 187206.0 / 994567 },
        { 591459.0 / 1989134, 6239551.0 / 9945670, 374412.0 / 4972835 },
        { 53769.0 / 1989134, 351524.0 / 4972835, 4929758.0 / 4972835 },
    };

    const double kXyzD65ToLinearA98[3][3] = {
        { 1829569.0 / 896150, -506331.0 / 896150, -308931.0 / 896150 },
        { -851781.0 / 878810, 1648619.0 / 878810, 36519.0 / 878810 },
        { 16779.0 / 1248040, -147721.0 / 1248040, 1266979.0 / 1248040 },
    };

    // ProPhoto is defined against D50
    const double kLinearProphotoToXyzD50[3][3] = {
        { 0.79776664490064230, 0.13518129740053308, 0.03134773412839220 },
        { 0.28807482881940130, 0.71183523424187300, 0.00008993693872564 },
        { 0.0, 0.0, 0.82510460251046020 },
    };

    const double kXyzD50ToLinearProphoto[3][3] = {
        { 1.34578688164715830, -0.25557208737979464, -0.05110186497554526 },
        { -0.54463070512490190, 1.50824774284514680, 0.02052744743642139 },
        { 0.0, 0.0, 1.21196754563894520 },
    };

    const double kLinear2020ToXyzD65[3][3] = {
        { 63426534.0 / 99577255, 20160776.0 / 139408157,
          47086771.0 / 278816314 },
        { 26158966.0 / 99577255, 472592308.0 / 697040785,
          8267143.0 / 139408157 },
        { 0.0, 19567812.0 / 697040785, 295819943.0 / 278816314 },
    };

    const double kXyzD65ToLinear2020[3][3] = {
        { 30757411.0 / 17917100, -6372589.0 / 17917100, -4539589.0 / 17917100 },
        { -19765991.0 / 29648200, 47925759.0 / 29648200, 467509.0 / 29648200 },
        { 792561.0 / 44930125, -1921689.0 / 44930125, 42328811.0 / 44930125 },
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

    void copy3(const double in[3], double out[3])
    {
        out[0] = in[0];
        out[1] = in[1];
        out[2] = in[2];
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

    double signedPow(double c, double exponent)
    {
        double sign = c < 0 ? -1 : 1;
        return sign * std::pow(std::fabs(c), exponent);
    }

    // ProPhoto RGB transfer function (css-color-4 sample code)
    double prophotoToLinear(double c)
    {
        const double et2 = 16.0 / 512;
        return std::fabs(c) <= et2 ? c / 16 : signedPow(c, 1.8);
    }

    double linearToProphoto(double c)
    {
        const double et = 1.0 / 512;
        return std::fabs(c) >= et ? signedPow(c, 1 / 1.8) : 16 * c;
    }

    // ITU-R BT.2020-2 transfer function (css-color-4 #predefined-rec2020)
    double rec2020ToLinear(double c)
    {
        const double alpha = 1.09929682680944;
        const double beta = 0.018053968510807;
        double sign = c < 0 ? -1 : 1;
        double abs = std::fabs(c);
        if (abs < beta * 4.5) {
            return c / 4.5;
        }
        return sign * std::pow((abs + alpha - 1) / alpha, 1 / 0.45);
    }

    double linearToRec2020(double c)
    {
        const double alpha = 1.09929682680944;
        const double beta = 0.018053968510807;
        double sign = c < 0 ? -1 : 1;
        double abs = std::fabs(c);
        if (abs < beta) {
            return 4.5 * c;
        }
        return sign * (alpha * std::pow(abs, 0.45) - (alpha - 1));
    }

    // The transfer function of each RGB space, to and from linear light;
    // display-p3 shares sRGB's curve.
    double rgbToLinear(ColorInterpolation::Space space, double c)
    {
        switch (space) {
        case ColorInterpolation::Srgb:
        case ColorInterpolation::DisplayP3:
            return srgbToLinear(c);
        case ColorInterpolation::SrgbLinear:
        case ColorInterpolation::DisplayP3Linear:
            return c;
        case ColorInterpolation::A98Rgb:
            return signedPow(c, 563.0 / 256);
        case ColorInterpolation::ProphotoRgb:
            return prophotoToLinear(c);
        case ColorInterpolation::Rec2020:
            return rec2020ToLinear(c);
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
            return c;
        }
    }

    double linearToRgb(ColorInterpolation::Space space, double c)
    {
        switch (space) {
        case ColorInterpolation::Srgb:
        case ColorInterpolation::DisplayP3:
            return linearToSrgb(c);
        case ColorInterpolation::SrgbLinear:
        case ColorInterpolation::DisplayP3Linear:
            return c;
        case ColorInterpolation::A98Rgb:
            return signedPow(c, 256.0 / 563);
        case ColorInterpolation::ProphotoRgb:
            return linearToProphoto(c);
        case ColorInterpolation::Rec2020:
            return linearToRec2020(c);
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
            return c;
        }
    }

    // The matrices of an RGB space, or nullptr for a non-RGB one. ProPhoto's
    // are defined against D50 and need the Bradford adaptation.
    struct RgbSpaceMatrices {
        const double (*m_toXyz)[3];
        const double (*m_fromXyz)[3];
        bool m_viaD50;
    };

    const RgbSpaceMatrices* rgbSpaceMatrices(ColorInterpolation::Space space)
    {
        static const RgbSpaceMatrices srgb = { kLinearSrgbToXyzD65,
                                               kXyzD65ToLinearSrgb, false };
        static const RgbSpaceMatrices p3 = { kLinearP3ToXyzD65,
                                             kXyzD65ToLinearP3, false };
        static const RgbSpaceMatrices a98 = { kLinearA98ToXyzD65,
                                              kXyzD65ToLinearA98, false };
        static const RgbSpaceMatrices prophoto = { kLinearProphotoToXyzD50,
                                                   kXyzD50ToLinearProphoto,
                                                   true };
        static const RgbSpaceMatrices rec2020 = { kLinear2020ToXyzD65,
                                                  kXyzD65ToLinear2020, false };
        switch (space) {
        case ColorInterpolation::Srgb:
        case ColorInterpolation::SrgbLinear:
            return &srgb;
        case ColorInterpolation::DisplayP3:
        case ColorInterpolation::DisplayP3Linear:
            return &p3;
        case ColorInterpolation::A98Rgb:
            return &a98;
        case ColorInterpolation::ProphotoRgb:
            return &prophoto;
        case ColorInterpolation::Rec2020:
            return &rec2020;
        default:
            return nullptr;
        }
    }

    void rgbToXyzD65(ColorInterpolation::Space space, const double rgb[3],
                     double xyz[3])
    {
        const RgbSpaceMatrices* m = rgbSpaceMatrices(space);
        double linear[3];
        for (int i = 0; i < 3; i++) {
            linear[i] = rgbToLinear(space, rgb[i]);
        }
        if (m->m_viaD50) {
            double d50[3];
            multiply(m->m_toXyz, linear, d50);
            multiply(kXyzD50ToD65, d50, xyz);
        } else {
            multiply(m->m_toXyz, linear, xyz);
        }
    }

    void xyzD65ToRgb(ColorInterpolation::Space space, const double xyz[3],
                     double rgb[3])
    {
        const RgbSpaceMatrices* m = rgbSpaceMatrices(space);
        double linear[3];
        if (m->m_viaD50) {
            double d50[3];
            multiply(kXyzD65ToD50, xyz, d50);
            multiply(m->m_fromXyz, d50, linear);
        } else {
            multiply(m->m_fromXyz, xyz, linear);
        }
        for (int i = 0; i < 3; i++) {
            rgb[i] = linearToRgb(space, linear[i]);
        }
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
        // computed in percent so that e.g. 30% + 50% stays exact in binary
        double w = hwb[1];
        double b = hwb[2];
        if (w + b >= 100) {
            double gray = w / (w + b);
            rgb[0] = rgb[1] = rgb[2] = gray;
            return;
        }
        double hsl[3] = { hwb[0], 100, 50 };
        hslToRgb(hsl, rgb);
        for (int i = 0; i < 3; i++) {
            rgb[i] = (rgb[i] * (100 - w - b) + w) / 100;
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

    // Every conversion goes through XYZ D65.
    void toXyzD65(ColorInterpolation::Space space, const double in[3],
                  double xyz[3])
    {
        double rgb[3];
        switch (space) {
        case ColorInterpolation::Hsl:
            hslToRgb(in, rgb);
            rgbToXyzD65(ColorInterpolation::Srgb, rgb, xyz);
            return;
        case ColorInterpolation::Hwb:
            hwbToRgb(in, rgb);
            rgbToXyzD65(ColorInterpolation::Srgb, rgb, xyz);
            return;
        case ColorInterpolation::Srgb:
        case ColorInterpolation::SrgbLinear:
        case ColorInterpolation::DisplayP3:
        case ColorInterpolation::DisplayP3Linear:
        case ColorInterpolation::A98Rgb:
        case ColorInterpolation::Rec2020:
        case ColorInterpolation::ProphotoRgb:
            rgbToXyzD65(space, in, xyz);
            return;
        case ColorInterpolation::Xyz:
        case ColorInterpolation::XyzD65:
            copy3(in, xyz);
            return;
        case ColorInterpolation::XyzD50:
            multiply(kXyzD50ToD65, in, xyz);
            return;
        case ColorInterpolation::Lab:
        case ColorInterpolation::Lch: {
            double lab[3], d50[3];
            if (space == ColorInterpolation::Lch) {
                lchToLab(in, lab);
            } else {
                copy3(in, lab);
            }
            labToXyzD50(lab, d50);
            multiply(kXyzD50ToD65, d50, xyz);
            return;
        }
        case ColorInterpolation::Oklab:
        case ColorInterpolation::Oklch: {
            double oklab[3];
            if (space == ColorInterpolation::Oklch) {
                lchToLab(in, oklab);
            } else {
                copy3(in, oklab);
            }
            oklabToXyzD65(oklab, xyz);
            return;
        }
        }
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    void fromXyzD65(ColorInterpolation::Space space, const double xyz[3],
                    double out[3])
    {
        double rgb[3];
        switch (space) {
        case ColorInterpolation::Hsl:
            xyzD65ToRgb(ColorInterpolation::Srgb, xyz, rgb);
            rgbToHsl(rgb, out);
            return;
        case ColorInterpolation::Hwb:
            xyzD65ToRgb(ColorInterpolation::Srgb, xyz, rgb);
            rgbToHwb(rgb, out);
            return;
        case ColorInterpolation::Srgb:
        case ColorInterpolation::SrgbLinear:
        case ColorInterpolation::DisplayP3:
        case ColorInterpolation::DisplayP3Linear:
        case ColorInterpolation::A98Rgb:
        case ColorInterpolation::Rec2020:
        case ColorInterpolation::ProphotoRgb:
            xyzD65ToRgb(space, xyz, out);
            return;
        case ColorInterpolation::Xyz:
        case ColorInterpolation::XyzD65:
            copy3(xyz, out);
            return;
        case ColorInterpolation::XyzD50:
            multiply(kXyzD65ToD50, xyz, out);
            return;
        case ColorInterpolation::Lab:
        case ColorInterpolation::Lch: {
            double d50[3], lab[3];
            multiply(kXyzD65ToD50, xyz, d50);
            xyzD50ToLab(d50, lab);
            if (space == ColorInterpolation::Lch) {
                labToLch(lab, out);
            } else {
                copy3(lab, out);
            }
            return;
        }
        case ColorInterpolation::Oklab:
        case ColorInterpolation::Oklch: {
            double oklab[3];
            xyzD65ToOklab(xyz, oklab);
            if (space == ColorInterpolation::Oklch) {
                labToLch(oklab, out);
            } else {
                copy3(oklab, out);
            }
            return;
        }
        }
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    bool sameFamily(ColorInterpolation::Space a, ColorInterpolation::Space b)
    {
        if (a == b) {
            return true;
        }
        if ((a == ColorInterpolation::Xyz || a == ColorInterpolation::XyzD65) &&
            (b == ColorInterpolation::Xyz || b == ColorInterpolation::XyzD65)) {
            return true;
        }
        return false;
    }

    // Whether the hue of `c` (already in a polar `space`) is powerless
    // (css-color-4 #powerless): the color is achromatic.
    bool huePowerless(ColorInterpolation::Space space, const double c[3])
    {
        switch (space) {
        case ColorInterpolation::Hsl:
            return c[1] == 0 || c[2] <= 0 || c[2] >= 100;
        case ColorInterpolation::Hwb:
            return c[1] + c[2] >= 100;
        case ColorInterpolation::Lch:
        case ColorInterpolation::Oklch:
            return c[1] < 1e-4;
        default:
            return false;
        }
    }

    // css-color-4 #interpolation-missing: the category a component belongs
    // to, so that a missing component carries over to the analogous one of
    // another space.
    enum ComponentCategory {
        Red,
        Green,
        Blue,
        X,
        Y,
        Z,
        Lightness,
        OpponentA,
        OpponentB,
        Chroma,
        Hue,
        Saturation,
        HslLightness,
        Whiteness,
        Blackness,
    };

    ComponentCategory category(ColorInterpolation::Space space, int i)
    {
        switch (space) {
        case ColorInterpolation::Srgb:
        case ColorInterpolation::SrgbLinear:
        case ColorInterpolation::DisplayP3:
        case ColorInterpolation::DisplayP3Linear:
        case ColorInterpolation::A98Rgb:
        case ColorInterpolation::ProphotoRgb:
        case ColorInterpolation::Rec2020:
            return i == 0 ? Red : (i == 1 ? Green : Blue);
        case ColorInterpolation::Xyz:
        case ColorInterpolation::XyzD50:
        case ColorInterpolation::XyzD65:
            return i == 0 ? X : (i == 1 ? Y : Z);
        case ColorInterpolation::Lab:
        case ColorInterpolation::Oklab:
            return i == 0 ? Lightness : (i == 1 ? OpponentA : OpponentB);
        case ColorInterpolation::Lch:
        case ColorInterpolation::Oklch:
            return i == 0 ? Lightness : (i == 1 ? Chroma : Hue);
        case ColorInterpolation::Hsl:
            return i == 0 ? Hue : (i == 1 ? Saturation : HslLightness);
        case ColorInterpolation::Hwb:
            return i == 0 ? Hue : (i == 1 ? Whiteness : Blackness);
        }
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return Red;
    }

    unsigned char analogousMissing(ColorInterpolation::Space from,
                                   unsigned char missing,
                                   ColorInterpolation::Space to)
    {
        unsigned char result = 0;
        for (int j = 0; j < 3; j++) {
            for (int i = 0; i < 3; i++) {
                if ((missing & (1 << i)) &&
                    category(from, i) == category(to, j)) {
                    result |= 1 << j;
                }
            }
        }
        return result;
    }

    // Converts `color` to `space` for interpolation. Missing components are
    // 0 for the conversion and carried over to the analogous components of
    // `space` (css-color-4 #interpolation-missing). A powerless hue becomes
    // missing. Returns the missing mask.
    unsigned char toSpace(ColorInterpolation::Space space,
                          const ExtendedColor& color, double out[3])
    {
        double in[3];
        for (int i = 0; i < 3; i++) {
            in[i] = color.missing(i) ? 0 : color.m_c[i];
        }
        unsigned char missing =
            analogousMissing(color.m_space, color.m_missing & 7, space);
        if (sameFamily(color.m_space, space)) {
            copy3(in, out);
        } else {
            double xyz[3];
            toXyzD65(color.m_space, in, xyz);
            fromXyzD65(space, xyz, out);
        }
        int hue = ColorInterpolation::hueIndex(space);
        if (hue >= 0 && !(missing & (1 << hue)) && huePowerless(space, out)) {
            missing |= 1 << hue;
        }
        return missing;
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
        if (!(v > 0)) {
            return 0;
        }
        if (v >= 1) {
            return 255;
        }
        return (unsigned char)std::lround(v * 255);
    }

    // css-color-4 numbers serialize with enough digits for the tests' epsilon
    // and no trailing zeros
    void appendNumber(StringBuilder& builder, double v)
    {
        char buf[32];
        if (std::fabs(v) < 5e-7) {
            v = 0;
        }
        snprintf(buf, sizeof(buf), "%.6g", v);
        builder.appendString(String::createASCIIString(buf, strlen(buf)));
    }

    void appendComponent(StringBuilder& builder, const ExtendedColor& color,
                         int i)
    {
        if (color.missing(i)) {
            builder.appendString("none");
        } else {
            appendNumber(builder, color.m_c[i]);
        }
    }

} // namespace

bool ColorInterpolation::parseSpace(const std::string& name, Space* space)
{
    struct {
        const char* name;
        Space space;
    } const table[] = {
        { "srgb", Srgb },
        { "srgb-linear", SrgbLinear },
        { "hsl", Hsl },
        { "hwb", Hwb },
        { "lab", Lab },
        { "oklab", Oklab },
        { "lch", Lch },
        { "oklch", Oklch },
        { "xyz", Xyz },
        { "xyz-d50", XyzD50 },
        { "xyz-d65", XyzD65 },
        { "display-p3", DisplayP3 },
        { "display-p3-linear", DisplayP3Linear },
        { "a98-rgb", A98Rgb },
        { "prophoto-rgb", ProphotoRgb },
        { "rec2020", Rec2020 },
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
    case DisplayP3:
        return "display-p3";
    case DisplayP3Linear:
        return "display-p3-linear";
    case A98Rgb:
        return "a98-rgb";
    case ProphotoRgb:
        return "prophoto-rgb";
    case Rec2020:
        return "rec2020";
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

int ColorInterpolation::hueIndex(Space space)
{
    switch (space) {
    case Hsl:
    case Hwb:
        return 0;
    case Lch:
    case Oklch:
        return 2;
    default:
        return -1;
    }
}

bool ColorInterpolation::isPolar(Space space)
{
    return hueIndex(space) >= 0;
}

void ColorInterpolation::hslToSrgb(const double hsl[3], double rgb[3])
{
    hslToRgb(hsl, rgb);
}

void ColorInterpolation::hwbToSrgb(const double hwb[3], double rgb[3])
{
    hwbToRgb(hwb, rgb);
}

Unit::Color ExtendedColor::toSrgb8() const
{
    double in[3];
    for (int i = 0; i < 3; i++) {
        in[i] = missing(i) ? 0 : m_c[i];
    }
    return toSrgb8(m_space, in, alphaMissing() ? 0 : m_alpha);
}

Unit::Color ExtendedColor::toSrgb8(ColorInterpolation::Space space,
                                   const double in[3], double alpha)
{
    double rgb[3];
    switch (space) {
    case ColorInterpolation::Srgb:
        copy3(in, rgb);
        break;
    case ColorInterpolation::Hsl:
        hslToRgb(in, rgb);
        break;
    case ColorInterpolation::Hwb:
        hwbToRgb(in, rgb);
        break;
    default: {
        double xyz[3];
        toXyzD65(space, in, xyz);
        fromXyzD65(ColorInterpolation::Srgb, xyz, rgb);
        break;
    }
    }
    return Unit::Color(toChannel(rgb[0]), toChannel(rgb[1]), toChannel(rgb[2]),
                       toChannel(alpha));
}

String* ExtendedColor::toString() const
{
    if (m_legacy) {
        return toSrgb8().toString();
    }
    STARFISH_ASSERT(m_space != ColorInterpolation::Hsl &&
                    m_space != ColorInterpolation::Hwb &&
                    m_space != ColorInterpolation::Xyz);
    StringBuilder builder;
    switch (m_space) {
    case ColorInterpolation::Lab:
        builder.appendString("lab(");
        break;
    case ColorInterpolation::Lch:
        builder.appendString("lch(");
        break;
    case ColorInterpolation::Oklab:
        builder.appendString("oklab(");
        break;
    case ColorInterpolation::Oklch:
        builder.appendString("oklch(");
        break;
    default: {
        builder.appendString("color(");
        const char* space = ColorInterpolation::spaceName(m_space);
        builder.appendString(space, strlen(space));
        builder.appendChar(' ');
        break;
    }
    }
    for (int i = 0; i < 3; i++) {
        if (i) {
            builder.appendChar(' ');
        }
        appendComponent(builder, *this, i);
    }
    if (alphaMissing()) {
        builder.appendString(" / none");
    } else if (m_alpha < 1) {
        builder.appendString(" / ");
        appendNumber(builder, m_alpha);
    }
    builder.appendChar(')');
    return builder.finalize();
}

ExtendedColor mixColors(ColorInterpolation::Space space,
                        ColorInterpolation::HueMethod method,
                        const ExtendedColor& a, double weightA,
                        const ExtendedColor& b, double weightB,
                        double alphaMultiplier)
{
    double ca[3], cb[3];
    unsigned char missingA = toSpace(space, a, ca);
    unsigned char missingB = toSpace(space, b, cb);
    double alphaA = a.alphaMissing() ? 0 : a.m_alpha;
    double alphaB = b.alphaMissing() ? 0 : b.m_alpha;
    bool alphaMissingA = a.alphaMissing();
    bool alphaMissingB = b.alphaMissing();

    // css-color-4 #interpolation-missing: a missing component takes the
    // other color's value; missing on both sides stays missing.
    unsigned char missing = missingA & missingB;
    for (int i = 0; i < 3; i++) {
        if ((missingA & (1 << i)) && !(missingB & (1 << i))) {
            ca[i] = cb[i];
        } else if ((missingB & (1 << i)) && !(missingA & (1 << i))) {
            cb[i] = ca[i];
        }
    }
    // a missing alpha takes the other color's; missing on both sides is
    // opaque for the mix and missing in the result
    if (alphaMissingA && !alphaMissingB) {
        alphaA = alphaB;
    } else if (alphaMissingB && !alphaMissingA) {
        alphaB = alphaA;
    } else if (alphaMissingA && alphaMissingB) {
        alphaA = alphaB = 1;
    }

    int hue = ColorInterpolation::hueIndex(space);
    if (hue >= 0) {
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

    ExtendedColor result;
    result.m_legacy = false;
    // hsl/hwb results are sRGB colors; `xyz` is an alias of xyz-d65
    if (space == ColorInterpolation::Hsl || space == ColorInterpolation::Hwb) {
        double rgb[3];
        double in[3];
        for (int i = 0; i < 3; i++) {
            in[i] = (missing & (1 << i)) ? 0 : out[i];
        }
        if (space == ColorInterpolation::Hsl) {
            hslToRgb(in, rgb);
        } else {
            hwbToRgb(in, rgb);
        }
        result.m_space = ColorInterpolation::Srgb;
        copy3(rgb, out);
        missing = 0;
    } else if (space == ColorInterpolation::Xyz) {
        result.m_space = ColorInterpolation::XyzD65;
    } else {
        result.m_space = space;
    }
    for (int i = 0; i < 3; i++) {
        result.m_c[i] = out[i];
    }
    result.m_missing = missing;
    if (alphaMissingA && alphaMissingB) {
        result.m_missing |= 1 << 3;
        result.m_alpha = 0;
    } else {
        result.m_alpha = std::max(0.0, std::min(1.0, alpha * alphaMultiplier));
    }
    return result;
}

} // namespace Starfish
