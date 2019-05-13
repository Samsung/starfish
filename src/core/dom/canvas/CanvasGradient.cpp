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
#include "core/dom/canvas/CanvasGradient.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/style/Style.h"
#include "core/modules/canvas/NativeGradient.h"

namespace Starfish {

CanvasGradient::CanvasGradient(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
{
    STARFISH_ASSERT(executionContext != nullptr);
    GC_REGISTER_FINALIZER_NO_ORDER(this,
                                   [](void* obj, void* cd) {
                                       STARFISH_ASSERT(obj != nullptr);
                                       CanvasGradient* c = (CanvasGradient*)obj;
                                       c->~CanvasGradient();
                                   },
                                   NULL, NULL, NULL);
}

CanvasGradient::CanvasGradient(ExecutionContext* executionContext, double x0,
                               double y0, double x1, double y1)
    : CanvasGradient(executionContext)
{
    m_nativeGardient = NativeGradient::create(x0, y0, x1, y1);
}

CanvasGradient::CanvasGradient(ExecutionContext* executionContext, double x0,
                               double y0, double r0, double x1, double y1,
                               double r1)
    : CanvasGradient(executionContext)
{
    m_nativeGardient = NativeGradient::create(x0, y0, r0, x1, y1, r1);
}

ScriptBindingInstance* CanvasGradient::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

void CanvasGradient::addColorStop(double offset, NULLABLE String* color)
{
    if (offset < 0 || 1 < offset) {
        throw new DOMException(m_executionContext,
                               DOMException::Code::INDEX_SIZE_ERR,
                               "The offset is less than 0 or greater than 1.");
    }

    Unit::Color clr;
    bool failed = true;
    if (color && !color->isEmpty()) {
        CSSTokenValue token(color->toUTF8NonGCString());
        CSSStyleValuePair pair;
        if (pair.updateValueUnitColor(token)) {
            if (pair.valueKind() ==
                CSSStyleValuePair::ValueKind::ColorValueKind) {
                clr = pair.colorValue();
                failed = false;
            } else if (pair.valueKind() ==
                       CSSStyleValuePair::ValueKind::NamedColorValueKind) {
                clr = NamedColor::namedColorToColor(pair.namedColorValue());
                failed = false;
            }
        }
    }

    if (failed) {
        throw new DOMException(m_executionContext,
                               DOMException::Code::SYNTAX_ERR,
                               "The color is invalid.");
    }
    m_nativeGardient->addColorStop(offset, clr);
}

bool CanvasGradient::isZeroSize()
{
    return m_nativeGardient->isZeroSize();
}
}
