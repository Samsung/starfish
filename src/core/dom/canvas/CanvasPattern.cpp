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

#include "StarfishConfig.h"
#include "core/dom/canvas/CanvasPattern.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/canvas/NativePattern.h"

namespace Starfish {
CanvasPattern::CanvasPattern(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_nativePattern()
    , m_originCleanFlag(true)
{
    STARFISH_ASSERT(executionContext != nullptr);
    GC_REGISTER_FINALIZER_NO_ORDER(this,
                                   [](void* obj, void* cd) {
                                       STARFISH_ASSERT(obj != nullptr);
                                       CanvasPattern* c = (CanvasPattern*)obj;
                                       c->~CanvasPattern();
                                   },
                                   NULL, NULL, NULL);
}

CanvasPattern::CanvasPattern(ExecutionContext* executionContext,
                             NULLABLE NativeImageData* image, bool repeatX,
                             bool repeatY)
    : CanvasPattern(executionContext)
{
    m_nativePattern = NativePattern::create(image, repeatX, repeatY);
}

ScriptBindingInstance* CanvasPattern::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

bool CanvasPattern::isEmptyPattern()
{
    return m_nativePattern->isEmpyPattern();
}
}
