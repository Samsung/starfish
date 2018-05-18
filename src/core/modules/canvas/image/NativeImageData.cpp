/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef ESCARGOT
#define ESCARGOT // for GCutil
#endif
#include "StarFishConfig.h"
#include "NativeImageData.h"
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
    arr[0].from = (GC_word*)ptr;
    arr[0].to = (GC_word*)ptr;
    return 0;
}

namespace StarFish {

int NativeImageData::nativeImageDataGCKind()
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
    }
    return gcKind;
}

std::vector<NativeImageData*>& NativeImageData::everyNativeImageInstances()
{
    STARFISH_ASSERT(isMainThread());
    static std::vector<NativeImageData*> v;
    return v;
}
} // namespace StarFish
