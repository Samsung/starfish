/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Renata Hodovan <reni@inf.u-szeged.hu>
 * Copyright (C) 2017-2022 Apple Inc. All rights reserved.
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __StarfishFilterTurbulence__
#define __StarfishFilterTurbulence__

#include "core/modules/canvas/filter/FilterPrimitive.h"
#include "core/dom/svg/SVGFETurbulenceElement.h"
#include <array>

// Architecture-specific SIMD type definitions
#if defined(STARFISH_X86) || defined(STARFISH_X86_64)
#include <xmmintrin.h>
typedef __m128 SimdFloat4;
#elif defined(STARFISH_ARM_NEON)
#include <arm_neon.h>
typedef float32x4_t SimdFloat4;
#else
// Fallback: use std::array<float, 4> for non-SIMD architectures
typedef std::array<float, 4> SimdFloat4;
#endif

namespace Starfish {

/**
 * @brief The FilterTurbulence class implements the filter primitive for the
 * <feTurbulence> element.
 *
 * This class is responsible for generating the turbulence image based on the
 * attributes of the SVGFETurbulenceElement.
 */
class FilterTurbulence : public FilterPrimitive {
private:
    // Produces results in the range [1, 2**31 - 2]. Algorithm is:
    // r = (a * r) mod m where a = s_randAmplitude = 16807 and
    // m = s_randMaximum = 2**31 - 1 = 2147483647, r = seed.
    // See [Park & Miller], CACM vol. 31 no. 10 p. 1195, Oct. 1988
    // To test: the algorithm should produce the result 1043618065
    // as the 10,000th generated number if the original seed is 1.
    static const int s_perlinNoise = 4096;
    static const long s_randMaximum = 2147483647; // 2**31 - 1
    static const int s_randAmplitude = 16807;     // 7**5; primitive root of m
    static const int s_randQ = 127773;            // m / a
    static const int s_randR = 2836;              // m % a

    static const int s_blockSize = 256;
    static const int s_blockMask = s_blockSize - 1;

    struct PaintingData {
        // Compute pseudo random number.
        long random()
        {
            long result =
                s_randAmplitude * (seed % s_randQ) - s_randR * (seed / s_randQ);
            if (result <= 0)
                result += s_randMaximum;
            seed = result;
            return result;
        }

        int type;
        float baseFrequencyX;
        float baseFrequencyY;
        int numOctaves;
        long seed;
        bool stitchTiles;
        Unit::IntSize paintingSize;

        std::array<int, 2 * s_blockSize + 2> latticeSelector;
        std::array<std::array<std::array<float, 2>, 2 * s_blockSize + 2>, 4>
            gradient;
    };

    struct StitchData {
        int width{ 0 }; // How much to subtract to wrap for stitching.
        int wrapX{ 0 }; // Minimum value to wrap.
        int height{ 0 };
        int wrapY{ 0 };
    };

    static inline float smoothCurve(float t)
    {
        return t * t * (3 - 2 * t);
    }
    static inline float linearInterpolation(float t, float a, float b)
    {
        return a + t * (b - a);
    }

    static PaintingData initPaintingData(int type, float baseFrequencyX,
                                         float baseFrequencyY, int numOctaves,
                                         long seed, bool stitchTiles,
                                         const Unit::IntSize& paintingSize);
    static StitchData computeStitching(Unit::IntSize tileSize,
                                       float& baseFrequencyX,
                                       float& baseFrequencyY, bool stitchTiles);

    static SimdFloat4 noise2D(const PaintingData&, const StitchData&,
                              const Unit::FloatPoint& noiseVector);
    static void toIntBasedColorComponents(SimdFloat4 floatComponents,
                                          uint8_t* buffer);
    static void calculateTurbulenceValueForPoint(const PaintingData&,
                                                 StitchData,
                                                 const Unit::FloatPoint&,
                                                 uint8_t* buffer);

public:
    FilterTurbulence(Filter* filter,
                     SVGFilterPrimitiveStandardAttributes* element);
    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void apply(const Unit::Rect& subRegionInFloat,
                       Filter::FilterApplyContext& ctx) override;

    std::pair<float, float> computeRadiusXY(
        const LayoutSize& targetSize,
        const std::pair<float, float>& viewportScale);

    virtual Filter::FilterBias computeBias(ComputeBiasContext& ctx) override;

    virtual bool canSubRegionExpandFrameRect() override
    {
        return true;
    }
};
} // namespace Starfish
#endif
