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

    // These objects are allocated with a GC disclaim proc (disclaimProc) that
    // a later reclaim sweep runs on this slot. GC_FREE()ing here would poison
    // the slot (GC_FREED_MEM_MARKER, or a free-list link in a non-debug
    // collector), and the disclaim proc would then dereference that garbage
    // as a vtable and crash. The destructor above has already disposed the
    // decoded buffer, so instead of freeing, mark the disclaim-proc sentinel
    // -- see m_gcDisclaimAlive below for why that can't be the vtable slot --
    // and let GC reclaim the small object shell.
    void operator delete(void* ptr)
    {
        reinterpret_cast<BufferedNativeImageData*>(ptr)->m_gcDisclaimAlive = 0;
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
        m_gcDisclaimAlive = 1;
#if !defined(OS_WINDOWS)
        everyNativeImageInstances().push_back(this);
#endif
    }
    bool m_isSeenByGC : 1;

private:
    // The disclaim proc GC_register_disclaim_proc() calls for a slot of this
    // kind. A private static member (not a free function) so it can reach
    // m_gcDisclaimAlive without a friend declaration; it has the plain
    // C-callback signature GC_register_disclaim_proc() expects.
    static int GC_CALLBACK disclaimProc(void* obj);

    // Liveness sentinel for disclaimProc (BufferedNativeImageData.cpp).
    // Deliberately NOT word 0 (the vtable pointer): the instant disclaimProc
    // returns 0, GC_reclaim_generic() (GCutil reclaim.c) claims word 0 as the
    // free-list link and only then zeroes the rest of the object via
    // GC_clear_block(), which explicitly skips word 0. So on the next sweep
    // over a slot still sitting unreclaimed on the free list, word 0 holds
    // that link (an ordinary, usually non-zero pointer), not our sentinel --
    // disclaimProc would misclassify the slot as live and dispatch
    // disposeNativeImageData() through a bogus vtable. m_gcDisclaimAlive lives
    // elsewhere in the object, which GC_clear_block *does* zero on every pass
    // (first disposal or a later re-visit alike), so it reads back 0
    // reliably from then on. 1 while the object is live, 0 once disposed;
    // never read/written anywhere else.
    size_t m_gcDisclaimAlive;
};
} // namespace Starfish

#endif
