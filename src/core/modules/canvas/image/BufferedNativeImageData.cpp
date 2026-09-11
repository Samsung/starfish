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

#ifndef ESCARGOT
#define ESCARGOT // for GCutil
#endif
#include "StarfishConfig.h"
#include "BufferedNativeImageData.h"
#include "core/modules/threading/Thread.h"

typedef int(GC_get_sub_pointer_proc)(void* ptr, struct GC_mark_pair* sub_ptrs);
template <GC_get_sub_pointer_proc proc, const int number_of_sub_pointer>
GC_ms_entry* markAndPushCustom(GC_word* addr,
                               struct GC_ms_entry* mark_stack_ptr,
                               struct GC_ms_entry* mark_stack_limit,
                               GC_word env)
{
    GC_mark_pair subPtrs[number_of_sub_pointer];
#if defined(GC_DEBUG)
    const char* start = (const char*)GC_USR_PTR_FROM_BASE(addr);
#else
    const char* start = (const char*)addr;
#endif
    int i = proc((/* no const */ void*)start, subPtrs);
    return GC_mark_and_push_ptrs(mark_stack_ptr, mark_stack_limit, subPtrs + i,
                                 number_of_sub_pointer - i);
}

int getValidValueNativeImageData(void* ptr, GC_mark_pair* arr)
{
    arr[0].from = (GC_word*)&ptr;
    arr[0].to = (GC_word*)ptr;
    return 0;
}

namespace Starfish {

// A disclaim proc is handed *every* unmarked slot of the block being swept --
// free-list fragments, slots never constructed, and (debug collector) slots
// poisoned by an explicit free -- and GC_disclaim_and_reclaim() (GCutil
// reclaim.c) re-visits the same slot on every later cycle. The liveness
// sentinel therefore cannot be word 0 (the vtable pointer): the instant this
// proc returns 0, GC_reclaim_generic() claims word 0 as the free-list link
// and only then zeroes the rest of the object via GC_clear_block(), which
// explicitly skips word 0. So on the next sweep over a slot still sitting
// unreclaimed on the free list, word 0 holds that link (an ordinary, usually
// non-zero pointer), not our sentinel -- this would misclassify the slot as
// live and dispatch disposeNativeImageData() (virtual) through a bogus
// vtable. m_gcDisclaimAlive lives elsewhere in the object, which
// GC_clear_block *does* zero on every pass (first disposal or a later
// re-visit alike), so it reads back 0 reliably from then on.
int GC_CALLBACK BufferedNativeImageData::disclaimProc(void* obj)
{
#ifdef GC_DEBUG
    obj = GC_USR_PTR_FROM_BASE(obj);
#endif
    BufferedNativeImageData* aliveObj = (BufferedNativeImageData*)obj;
    if (aliveObj->m_gcDisclaimAlive == 0) {
        // already freed
        return 0;
    }
#if !defined(NDEBUG)
    // An explicitly GC_FREE'd object is poisoned by the debug collector with
    // GC_FREED_MEM_MARKER (see GCutil include/private/dbg_mlc.h), and later
    // sweeps still run this disclaim proc over the freed slot. Whoever freed
    // it owned its disposal; treating the poison as a vtable crashes, so skip
    // it like the m_gcDisclaimAlive == 0 case above.
    const size_t kGcFreedMemMarker = sizeof(size_t) == 8
                                         ? (size_t)0xEFBEADDEdeadbeefULL
                                         : (size_t)0xdeadbeef;
    if (aliveObj->m_gcDisclaimAlive == kGcFreedMemMarker) {
        return 0;
    }
#endif
    aliveObj->disposeNativeImageData();
    // mark cleared
    aliveObj->m_gcDisclaimAlive = 0;
    return 0;
}

int BufferedNativeImageData::nativeImageDataGCKind()
{
    static bool isInited = false;
    static int gcKind;
    if (!isInited) {
        isInited = true;
        gcKind = GC_new_kind_enumerable(
            GC_new_free_list(),
            GC_MAKE_PROC(
                GC_new_proc(markAndPushCustom<getValidValueNativeImageData, 1>),
                0),
            FALSE, TRUE);
        GC_register_disclaim_proc(gcKind, disclaimProc, 1);
    }
    return gcKind;
}

std::vector<BufferedNativeImageData*>&
BufferedNativeImageData::everyNativeImageInstances()
{
    STARFISH_ASSERT(isMainThread());
    static std::vector<BufferedNativeImageData*> v;
    return v;
}

BufferedNativeImageData* BufferedNativeImageData::create(float devicePixelRatio,
                                                         size_t width,
                                                         size_t height)
{
    size_t deviceImageWidth = ceil(width * devicePixelRatio);
    size_t deviceImageHeight = ceil(height * devicePixelRatio);
    return BufferedNativeImageData::create(deviceImageWidth, deviceImageHeight);
}

} // namespace Starfish
