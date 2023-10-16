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

#ifndef __BufferedNativeImageData__
#define __BufferedNativeImageData__

#include "core/style/Style.h"
#include "core/modules/canvas/image/NativeImageData.h"

namespace Starfish {

class CanvasShadowData;
class Canvas;
class AnimatedGIFNativeImageData;
class CompressedNativeImageData;
class SVGNativeImageData;
struct DrawImageInfo;

class BufferedNativeImageData : public NativeImageData {
    friend class ResourceLoader;

public:
    static int nativeImageDataGCKind();
    static std::vector<BufferedNativeImageData*>& everyNativeImageInstances();

    static NativeImageData* create(size_t actualDeviceWidth,
                                   size_t actualDeviceHeight);
    static NativeImageData* create(float devicePixelRatio, size_t width,
                                   size_t height); // this function will apply
                                                   // device-pixel-ratio to
                                                   // width, height
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
    virtual ~BufferedNativeImageData()
    {
        GC_REGISTER_FINALIZER_NO_ORDER(this, NULL, NULL, NULL, NULL);
    }

    virtual bool isAttachableNativeImage()
    {
        return false;
    }

    void* operator new(size_t size) = delete;
    void* operator new[](size_t size) = delete;

#ifdef STARFISH_ENABLE_TEST
    virtual void dumpImage(const char* path)
    {
    }
#endif

protected:
    BufferedNativeImageData()
    {
        m_isSeenByGC = false;
#if !defined(OS_WINDOWS)
        everyNativeImageInstances().push_back(this);
#endif
        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                BufferedNativeImageData* self = (BufferedNativeImageData*)obj;
                self->disposeNativeImageData();
            },
            NULL, NULL, NULL);
    }
    bool m_isSeenByGC : 1;
};
} // namespace Starfish

#endif
