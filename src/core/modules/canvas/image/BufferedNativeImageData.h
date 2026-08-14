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

    static BufferedNativeImageData* create(size_t actualDeviceWidth,
                                           size_t actualDeviceHeight);
    static BufferedNativeImageData* create(
        float devicePixelRatio, size_t width,
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
        auto iter = std::find(r.begin(), r.end(), this);
        if (iter != r.end()) {
            r.erase(iter);
        }
#endif
    }
    virtual ~BufferedNativeImageData()
    {
        disposeNativeImageData();
    }

    virtual bool isAttachableNativeImage()
    {
        return false;
    }

    void* operator new(size_t size) = delete;
    void* operator new[](size_t size) = delete;

    // These objects are allocated with a GC disclaim proc
    // (bufferedNativeImageDataClear) that a later reclaim sweep runs on this
    // slot. GC_FREE()ing here would poison the slot (GC_FREED_MEM_MARKER, or a
    // free-list link in a non-debug collector), and the disclaim proc would
    // then dereference that garbage as a vtable and crash. The destructor above
    // has already disposed the decoded buffer, so instead of freeing, clear the
    // vtable slot -- the "already disposed" sentinel the disclaim proc checks
    // -- and let GC reclaim the small object shell.
    void operator delete(void* ptr)
    {
        *reinterpret_cast<size_t*>(ptr) = 0;
    }

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
    }
    bool m_isSeenByGC : 1;
};
} // namespace Starfish

#endif
