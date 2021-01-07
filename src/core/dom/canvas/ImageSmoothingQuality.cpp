/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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
#include "core/style/Style.h"
#include "core/dom/canvas/ImageSmoothingQuality.h"

namespace Starfish {

ImageRenderingValue toImageRenderingValue(bool imageSmoothingEnabled,
                                          ImageSmoothingQuality quality)
{
    ImageRenderingValue ret =
        ImageRenderingValue::ImageRenderingCrispEdgesValue;

    if (imageSmoothingEnabled == true) {
        if (quality == ImageSmoothingQuality::Low ||
            quality == ImageSmoothingQuality::Medium) {
            ret = ImageRenderingValue::ImageRenderingAutoValue;
        } else if (quality == ImageSmoothingQuality::High) {
            ret = ImageRenderingValue::ImageRenderingPixelatedValue;
        }
    }
    return ret;
}
} // namespace Starfish
