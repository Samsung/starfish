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

template <GC_get_sub_pointer_proc proc, const int number_of_sub_pointer>
GC_ms_entry* markAndPushCustom(GC_word* addr,
                               struct GC_ms_entry* mark_stack_ptr,
                               struct GC_ms_entry* mark_stack_limit,
                               GC_word env)
{
    GC_mark_custom_result subPtrs[number_of_sub_pointer];
    return GC_mark_and_push_custom(addr, mark_stack_ptr, mark_stack_limit, proc,
                                   subPtrs, number_of_sub_pointer);
}

int getValidValueNativeImageData(void* ptr, GC_mark_custom_result* arr)
{
    arr[0].from = (GC_word*)&ptr;
    arr[0].to = (GC_word*)ptr;
    return 0;
}

namespace Starfish {

static int bufferedNativeImageDataClear(void* obj)
{
#if !defined(NDEBUG)
    obj = GC_USR_PTR_FROM_BASE(obj);
#endif
    size_t* ptr = (size_t*)obj;
    if (*ptr == 0) {
        // already freed
        return 0;
    }
#if !defined(NDEBUG)
    // An explicitly GC_FREE'd object is poisoned by the debug collector with
    // GC_FREED_MEM_MARKER (see GCutil include/private/dbg_mlc.h), and later
    // sweeps still run this disclaim proc over the freed slot. Whoever freed
    // it owned its disposal; treating the poison as a vtable crashes, so skip
    // it like the *ptr == 0 case above.
    const size_t kGcFreedMemMarker = sizeof(size_t) == 8
                                         ? (size_t)0xEFBEADDEdeadbeefULL
                                         : (size_t)0xdeadbeef;
    if (*ptr == kGcFreedMemMarker) {
        return 0;
    }
#endif
    BufferedNativeImageData* aliveObj = (BufferedNativeImageData*)obj;
    aliveObj->disposeNativeImageData();
    // mark cleared
    *ptr = 0;
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
        GC_register_disclaim_proc(gcKind, bufferedNativeImageDataClear, 1);
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
