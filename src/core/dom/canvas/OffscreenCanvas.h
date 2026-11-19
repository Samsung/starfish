/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef __StarfishOffscreenCanvas__
#define __StarfishOffscreenCanvas__

#ifdef STARFISH_ENABLE_CANVAS

#include "core/dom/EventTarget.h"

namespace Starfish {

class OffscreenCanvasRenderingContext2D;

class OffscreenCanvas : public EventTarget {
public:
    OffscreenCanvas(ExecutionContext* executionContext, uint64_t width,
                    uint64_t height);
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isOffscreenCanvas() const override;

    ExecutionContext* executionContext() const override
    {
        return m_executionContext;
    }

    uint64_t width() const
    {
        return m_width;
    }
    uint64_t height() const
    {
        return m_height;
    }
    void setWidth(uint64_t width);
    void setHeight(uint64_t height);

    Optional<OffscreenCanvasRenderingContext2D*> getContext(
        String* contextId, ScriptValue options);

    Optional<OffscreenCanvasRenderingContext2D*> context2D()
    {
        return m_context;
    }

private:
    void resetBitmap();

    ExecutionContext* m_executionContext;
    Optional<OffscreenCanvasRenderingContext2D*> m_context;
    uint64_t m_width;
    uint64_t m_height;
};

} // namespace Starfish

#endif
#endif
