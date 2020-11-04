/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __NativeImageData__
#define __NativeImageData__

#include "core/style/Style.h"

namespace Starfish {

class CanvasShadowData;
class Canvas;
class AnimatedGIFNativeImageData;
class CompressedNativeImageData;
class SVGNativeImageData;
class DrawImageInfo;

constexpr size_t ExtraSmallNativeImageSize{ 16 };

class NativeImageData : public gc {
public:
    enum PreserveAspectRatioValue ENSURE_ENUM_UNSIGNED {
        None,
        xMinYMin,
        xMidYMin,
        xMaxYMin,
        xMinYMid,
        xMidYMid,
        xMaxYMid,
        xMinYMax,
        xMidYMax,
        xMaxYMax,
    };
    static NativeImageData* attach(Canvas* canvas);

    virtual bool isAnimatedGIFNativeImageData() const
    {
        return false;
    }

    virtual bool isCompressedNativeImageData() const
    {
        return false;
    }

    virtual bool isSVGNativeImageData() const
    {
        return false;
    }

    AnimatedGIFNativeImageData* asAnimatedGIFNativeImageData()
    {
        STARFISH_ASSERT(isAnimatedGIFNativeImageData());
        return (AnimatedGIFNativeImageData*)this;
    }

    CompressedNativeImageData* asCompressedNativeImageData()
    {
        STARFISH_ASSERT(isCompressedNativeImageData());
        return (CompressedNativeImageData*)this;
    }

    SVGNativeImageData* asSVGNativeImageData()
    {
        STARFISH_ASSERT(isSVGNativeImageData());
        return (SVGNativeImageData*)this;
    }

    bool isEmptyImage()
    {
        size_t h = height();
        size_t w = width();
        size_t s = stride();
        uint8_t* ptr = data();
        for (size_t y = 0; y < h; y++) {
            uint8_t* p = ptr;
            for (size_t x = 0; x < w; x++) {
                if (p[3]) {
                    return false;
                }
                p += 4;
            }
            ptr += s;
        }

        return true;
    }

    virtual size_t bufferSize()
    {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return 0;
    }

    virtual uint8_t* data()
    {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return nullptr;
    }

    virtual void* unwrap()
    {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return nullptr;
    }

    virtual void clear() = 0;
    virtual size_t width() = 0;
    virtual size_t height() = 0;
    virtual size_t stride()
    {
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return 0;
    }

    virtual ~NativeImageData()
    {
        GC_REGISTER_FINALIZER_NO_ORDER(this, NULL, NULL, NULL, NULL);
    }

    virtual bool isAttachableNativeImage()
    {
        return false;
    }

    PreserveAspectRatioValue preserveAspectRatioValue()
    {
        return m_preserveAspectRatioValue;
    }

    void setPreserveAspectRatioValue(PreserveAspectRatioValue v)
    {
        m_preserveAspectRatioValue = v;
    }

    virtual void paintContent(Canvas* canvas, const Unit::Rect& dst,
                              ImageRenderingValue imageRenderingMode)
    {
    }

    virtual void paintContent(Canvas* canvas, const Unit::Rect& src,
                              const Unit::Rect& dst,
                              const DrawImageInfo& borderinfo,
                              ImageRenderingValue imageRenderingMode)
    {
    }

    virtual void paintRepeatContent(Canvas* canvas, const Unit::Rect& dst,
                                    float imageWidth, float imageHeight,
                                    bool xRepeat, bool yRepeat,
                                    ImageRenderingValue imageRenderingMode)
    {
    }

    void* operator new(size_t size) = delete;
    void* operator new[](size_t size) = delete;

#ifdef STARFISH_ENABLE_TEST
    virtual void dumpImage(const char* path)
    {
    }
#endif

protected:
    NativeImageData()
    {
        m_preserveAspectRatioValue = None;
    }
    PreserveAspectRatioValue m_preserveAspectRatioValue : 4;
};
} // namespace Starfish

#endif
