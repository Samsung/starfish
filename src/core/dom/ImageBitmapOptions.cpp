/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software{} you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation{} either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY{} without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library{} if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "StarfishConfig.h"
#include "core/dom/ImageBitmapOptions.h"

namespace Starfish {
bool ImageBitmapOptions::stringToImageOrientation(String* imageOrientation,
                                                  ImageOrientation& out)
{
    STARFISH_ASSERT(imageOrientation != nullptr);
    if (imageOrientation->equals("none", 4)) {
        out = ImageOrientation::None;
        return true;
    } else if (imageOrientation->equals("flipY", 5)) {
        out = ImageOrientation::FlipY;
        return true;
    }
    return false;
}

String* ImageBitmapOptions::imageOrientationToString(
    ImageOrientation imageOrientation)
{
    if (imageOrientation == ImageOrientation::None) {
        return String::createASCIIString("none");
    } else if (imageOrientation == ImageOrientation::FlipY) {
        return String::createASCIIString("flipY");
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::createASCIIString("none");
}

bool ImageBitmapOptions::stringToPremultiplyAlpha(String* premultiplyAlpha,
                                                  PremultiplyAlpha& out)
{
    STARFISH_ASSERT(premultiplyAlpha != nullptr);
    if (premultiplyAlpha->equals("none", 4)) {
        out = PremultiplyAlpha::None;
        return true;
    } else if (premultiplyAlpha->equals("premultiply", 11)) {
        out = PremultiplyAlpha::Premultiply;
        return true;
    } else if (premultiplyAlpha->equals("default", 7)) {
        out = PremultiplyAlpha::Default;
        return true;
    }
    return false;
}

String* ImageBitmapOptions::premultiplyAlphaToString(
    PremultiplyAlpha premultiplyAlpha)
{
    if (premultiplyAlpha == PremultiplyAlpha::None) {
        return String::createASCIIString("none");
    } else if (premultiplyAlpha == PremultiplyAlpha::Premultiply) {
        return String::createASCIIString("premultiply");
    } else if (premultiplyAlpha == PremultiplyAlpha::Default) {
        return String::createASCIIString("default");
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::createASCIIString("default");
}

bool ImageBitmapOptions::stringToColorSpaceConversion(
    String* colorSpaceConversion, ColorSpaceConversion& out)
{
    STARFISH_ASSERT(colorSpaceConversion != nullptr);
    if (colorSpaceConversion->equals("none", 4)) {
        out = ColorSpaceConversion::None;
        return true;
    } else if (colorSpaceConversion->equals("default", 7)) {
        out = ColorSpaceConversion::Default;
        return true;
    }
    return false;
}

String* ImageBitmapOptions::colorSpaceConversionToString(
    ColorSpaceConversion colorSpaceConversion)
{
    if (colorSpaceConversion == ColorSpaceConversion::None) {
        return String::createASCIIString("none");
    } else if (colorSpaceConversion == ColorSpaceConversion::Default) {
        return String::createASCIIString("default");
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::createASCIIString("default");
}

bool ImageBitmapOptions::stringToResizeQuality(String* resizeQuality,
                                               ResizeQuality& out)
{
    STARFISH_ASSERT(resizeQuality != nullptr);
    if (resizeQuality->equals("pixelated", 9)) {
        out = ResizeQuality::Pixelated;
        return true;
    } else if (resizeQuality->equals("low", 3)) {
        out = ResizeQuality::Low;
        return true;
    } else if (resizeQuality->equals("medium", 6)) {
        out = ResizeQuality::Medium;
        return true;
    } else if (resizeQuality->equals("high", 4)) {
        out = ResizeQuality::high;
        return true;
    }
    return false;
}

String* ImageBitmapOptions::resizeQualityToString(ResizeQuality resizeQuality)
{
    if (resizeQuality == ResizeQuality::Pixelated) {
        return String::createASCIIString("pixelated");
    } else if (resizeQuality == ResizeQuality::Low) {
        return String::createASCIIString("low");
    } else if (resizeQuality == ResizeQuality::Medium) {
        return String::createASCIIString("medium");
    } else if (resizeQuality == ResizeQuality::high) {
        return String::createASCIIString("high");
    }
    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return String::createASCIIString("low");
}

String* ImageBitmapOptions::imageOrientation()
{
    return imageOrientationToString(m_imageOrientation);
}

void ImageBitmapOptions::setImageOrientation(String* imageOrientation)
{
    ImageOrientation value;
    if (stringToImageOrientation(imageOrientation, value)) {
        m_imageOrientation = value;
    }
}

String* ImageBitmapOptions::premultiplyAlpha()
{
    return premultiplyAlphaToString(m_premultiplyAlpha);
}

void ImageBitmapOptions::setPremultiplyAlpha(String* premultiplyAlpha)
{
    PremultiplyAlpha value;
    if (stringToPremultiplyAlpha(premultiplyAlpha, value)) {
        m_premultiplyAlpha = value;
    }
}

String* ImageBitmapOptions::colorSpaceConversion()
{
    return colorSpaceConversionToString(m_colorSpaceConversion);
}

void ImageBitmapOptions::setColorSpaceConversion(String* colorSpaceConversion)
{
    ColorSpaceConversion value;
    if (stringToColorSpaceConversion(colorSpaceConversion, value)) {
        m_colorSpaceConversion = value;
    }
}

String* ImageBitmapOptions::resizeQuality()
{
    return resizeQualityToString(m_resizeQuality);
}

void ImageBitmapOptions::setResizeQuality(String* resizeQuality)
{
    ResizeQuality value;
    if (stringToResizeQuality(resizeQuality, value)) {
        m_resizeQuality = value;
    }
}
} // namespace Starfish
