/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __NativeImageData__
#define __NativeImageData__

namespace Starfish {

class CanvasShadowData;
class Canvas;
class AnimatedGIFNativeImageData;
class CompressedNativeImageData;
class SVGNativeImageData;

class NativeImageData : public gc {
    friend class ResourceLoader;

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

    static int nativeImageDataGCKind();
    static std::vector<NativeImageData*>& everyNativeImageInstances();

    static NativeImageData* create(size_t actualDeviceWidth,
                                   size_t actualDeviceHeight);
    static NativeImageData* create(float devicePixelRatio, size_t width,
                                   size_t height); // this function will apply
                                                   // device-pixel-ratio to
                                                   // width, height
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

    virtual void pruneInternalDataIfPossible()
    {
    }
    virtual void disposeNativeImageData()
    {
#if !defined(OS_WINDOWS)
        auto& r = everyNativeImageInstances();
        r.erase(std::find(r.begin(), r.end(), this));
#endif
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
        m_isSeenByGC = false;
        m_preserveAspectRatioValue = None;
#if !defined(OS_WINDOWS)
        everyNativeImageInstances().push_back(this);
#endif
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           NativeImageData* self =
                                               (NativeImageData*)obj;
                                           self->disposeNativeImageData();
                                       },
                                       NULL, NULL, NULL);
    }
    bool m_isSeenByGC : 1;
    PreserveAspectRatioValue m_preserveAspectRatioValue : 4;
};
} // namespace Starfish

#endif
