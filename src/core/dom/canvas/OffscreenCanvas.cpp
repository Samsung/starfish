/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifdef STARFISH_ENABLE_CANVAS

#include "StarfishConfig.h"
#include "core/dom/DOMException.h"
#include "core/dom/canvas/OffscreenCanvas.h"
#include "core/dom/canvas/OffscreenCanvasRenderingContext2D.h"

namespace Starfish {

OffscreenCanvas::OffscreenCanvas(ExecutionContext* executionContext,
                                 uint64_t width, uint64_t height)
    : m_executionContext(executionContext)
    , m_width(width)
    , m_height(height)
{
}

void OffscreenCanvas::setWidth(uint64_t width)
{
    m_width = width;
    resetBitmap();
}

void OffscreenCanvas::setHeight(uint64_t height)
{
    m_height = height;
    resetBitmap();
}

void OffscreenCanvas::resetBitmap()
{
    if (m_context) {
        m_context.value()->onResize();
        m_context.value()->setOriginCleanFlag(true);
    }
}

Optional<OffscreenCanvasRenderingContext2D*> OffscreenCanvas::getContext(
    String* contextId, ScriptValue options)
{
    if (contextId->isEmpty()) {
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_TYPE_ERR,
                               "Invalid OffscreenCanvas context type");
    }
    if (!contextId->equals("2d")) {
        return NullOption;
    }

    if (!m_context) {
        m_context = new OffscreenCanvasRenderingContext2D(this);
    }
    return m_context;
}

OffscreenCanvasRenderingContext2D::OffscreenCanvasRenderingContext2D(
    OffscreenCanvas* owner)
    : CanvasRenderingContext2DMixIn(owner->executionContext(), NullOption)
    , m_owner(owner)
{
    initialize();
    setOriginCleanFlag(true);
}

void OffscreenCanvasRenderingContext2D::initialize()
{
    initializeBitmap(std::min<uint64_t>(m_owner->width(), UINT32_MAX),
                     std::min<uint64_t>(m_owner->height(), UINT32_MAX));
}

} // namespace Starfish

#endif
