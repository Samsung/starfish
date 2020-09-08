/*
 * Copyright (c) 2020-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "binding/ScriptWrappable.h"
#include "core/dom/ExecutionContext.h"
#include "core/extra/Performance.h"

namespace Starfish {

Performance::Performance(ExecutionContext* executionContext)
    : m_executionContext(executionContext)
{
}

void* Performance::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(Performance));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(Performance)] = { 0 };
        Performance::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(Performance));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

double Performance::now()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return 0;
}
}
