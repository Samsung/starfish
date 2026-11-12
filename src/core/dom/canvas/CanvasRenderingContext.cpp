/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_ENABLE_CANVAS

#include "StarfishConfig.h"
#include "core/dom/canvas/HTMLCanvasElement.h"
#include "core/dom/canvas/CanvasRenderingContext.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/canvas/Compositor.h"

namespace Starfish {

ScriptBindingInstance* CanvasRenderingContext::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

void CanvasRenderingContext::calculateDimension(uint32_t& outWidth,
                                                uint32_t& outHeight,
                                                const uint32_t elementWidth,
                                                const uint32_t elementHeight)
{
    calculateDimension(executionContext()->starfish(), outWidth, outHeight,
                       elementWidth, elementHeight);
}

void CanvasRenderingContext::calculateDimension(Starfish* starfish,
                                                uint32_t& outWidth,
                                                uint32_t& outHeight,
                                                const uint32_t elementWidth,
                                                const uint32_t elementHeight)
{
    uint64_t width = elementWidth;
    uint64_t height = elementHeight;

    if (elementWidth == 0 || isInfOrNan(elementWidth)) {
        width = 1;
    }

    if (elementHeight == 0 || isInfOrNan(elementHeight)) {
        height = 1;
    }

    uint64_t maxTextureSize = Compositor::maximumTextureSize(starfish);
    uint64_t maxTextureArea = maxTextureSize * maxTextureSize;

    if (width * height >= maxTextureArea) {
        double total = width + height;
        // Bound the bitmap dimensions before conversion: a nonempty canvas
        // must not acquire a zero dimension through truncation, and rounding
        // a large area must not overflow the integer conversion.
        double scaledWidth =
            maxTextureArea * (static_cast<double>(width) / total);
        double scaledHeight =
            maxTextureArea * (static_cast<double>(height) / total);
        width = std::max(1.0, std::min<double>(maxTextureSize, scaledWidth));
        height = std::max(1.0, std::min<double>(maxTextureSize, scaledHeight));
    }

    width = std::min(width, maxTextureSize);
    height = std::min(height, maxTextureSize);

    STARFISH_ASSERT(width != 0);
    STARFISH_ASSERT(height != 0);

    outWidth = width;
    outHeight = height;
}

} // namespace Starfish

#endif
