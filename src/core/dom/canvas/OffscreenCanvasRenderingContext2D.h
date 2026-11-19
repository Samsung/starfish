/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef __StarfishOffscreenCanvasRenderingContext2D__
#define __StarfishOffscreenCanvasRenderingContext2D__

#ifdef STARFISH_ENABLE_CANVAS

#include "core/dom/canvas/CanvasRenderingContext2DMixIn.h"

namespace Starfish {

class OffscreenCanvas;

class OffscreenCanvasRenderingContext2D : public CanvasRenderingContext2DMixIn {
public:
    explicit OffscreenCanvasRenderingContext2D(OffscreenCanvas* owner);
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isOffscreenCanvasRenderingContext2D() const override;
    void initialize() override;

    OffscreenCanvas* canvas() const
    {
        return m_owner;
    }

    void* operator new(size_t size)
    {
        using Self = OffscreenCanvasRenderingContext2D;
        bool& initialized = GCDescriptorCache<Self>::inited;
        GC_descr& descriptor = GCDescriptorCache<Self>::descr;
        if (!initialized) {
            GC_word bitmap[GC_BITMAP_SIZE(Self)] = { 0 };
            CanvasRenderingContext2DMixIn::fillGCDescriptor(bitmap);
            GC_set_bit(bitmap, GC_WORD_OFFSET(Self, m_owner));
            descriptor = GC_make_descriptor(bitmap, GC_WORD_LEN(Self));
            initialized = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descriptor);
    }

private:
    OffscreenCanvas* m_owner;
};

} // namespace Starfish

#endif
#endif
