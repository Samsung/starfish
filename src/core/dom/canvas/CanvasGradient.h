/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishCanvasGradient__
#define __StarfishCanvasGradient__

#include "binding/ScriptWrappable.h"

namespace Starfish {
class NativeGradient;
class ExecutionContext;
struct GradientDrawingInfo;

class CanvasGradient : public ScriptWrappable {
public:
    CanvasGradient(ExecutionContext* executionContext, double x0, double y0,
                   double x1, double y1);
    CanvasGradient(ExecutionContext* executionContext, double x0, double y0,
                   double r0, double x1, double y1, double r1);
    CanvasGradient(ExecutionContext* executionContext,
                   GradientDrawingInfo* info);
    ~CanvasGradient()
    {
    }

    void* operator new(size_t size);
    void clearNativeResources();
    // Objects allocated via GC_finalized_malloc must not be freed with
    // GC_FREE or delete. The no-op operator delete below prevents this.
    void operator delete(void*)
    {
    }
    void operator delete[](void*) = delete;

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(CanvasGradient)

    void addColorStop(double offset, NULLABLE String* color);
    void addColorStop(double offset, const Unit::Color& color);

    std::shared_ptr<NativeGradient> nativeGradient()
    {
        return m_nativeGardient;
    }

    bool isZeroSize();

private:
    CanvasGradient(ExecutionContext* executionContext);

    ExecutionContext* m_executionContext;
    std::shared_ptr<NativeGradient> m_nativeGardient;
};
} // namespace Starfish

#endif
