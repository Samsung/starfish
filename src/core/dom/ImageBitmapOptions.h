/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishImageBitmapOptions__
#define __StarfishImageBitmapOptions__

namespace Starfish {

enum class ImageSmoothingQuality;

enum class ImageOrientation { None, FlipY };
enum class PremultiplyAlpha { None, Premultiply, Default };
enum class ColorSpaceConversion { None, Default };
enum class ResizeQuality { Pixelated, Low, Medium, High };

struct ImageBitmapOptions {
public:
    STARFISH_MAKE_STACK_ALLOCATED();
    static bool stringToImageOrientation(String* imageOrientation,
                                         ImageOrientation& out);
    static String* imageOrientationToString(ImageOrientation imageOrientation);

    static bool stringToPremultiplyAlpha(String* premultiplyAlpha,
                                         PremultiplyAlpha& out);
    static String* premultiplyAlphaToString(PremultiplyAlpha premultiplyAlpha);

    static bool stringToColorSpaceConversion(String* colorSpaceConversion,
                                             ColorSpaceConversion& out);
    static String* colorSpaceConversionToString(
        ColorSpaceConversion colorSpaceConversion);

    static bool stringToResizeQuality(String* resizeQuality,
                                      ResizeQuality& out);
    static String* resizeQualityToString(ResizeQuality resizeQuality);

    ImageSmoothingQuality toImageRenderingValue();

    // Interface ImageBitmapOptions
    String* imageOrientation();
    void setImageOrientation(String* imageOrientation);

    String* premultiplyAlpha();
    void setPremultiplyAlpha(String* premultiplyAlpha);

    String* colorSpaceConversion();
    void setColorSpaceConversion(String* colorSpaceConversion);

    DEFINE_GETTER_SETTER_WITH_HASFLAG(uint32_t, resizeWidth, ResizeWidth);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(uint32_t, resizeHeight, ResizeHeight);

    String* resizeQuality();
    void setResizeQuality(String* resizeQuality);

    // Others
    ResizeQuality getResizeQuality()
    {
        return m_resizeQuality;
    }

    ImageOrientation getImageOrientation()
    {
        return m_imageOrientation;
    }

private:
    ImageOrientation m_imageOrientation{ ImageOrientation::None };
    PremultiplyAlpha m_premultiplyAlpha{ PremultiplyAlpha::Default };
    ColorSpaceConversion m_colorSpaceConversion{
        ColorSpaceConversion::Default
    };
    uint32_t m_resizeWidth{ 0 };
    bool m_hasResizeWidth{ false };
    uint32_t m_resizeHeight{ 0 };
    bool m_hasResizeHeight{ false };
    ResizeQuality m_resizeQuality{ ResizeQuality::Low };
};

} // namespace Starfish
#endif
