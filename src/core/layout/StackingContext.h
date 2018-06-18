/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishStackingContext__
#define __StarFishStackingContext__

namespace StarFish {

class Canvas;
class CanvasSurface;
class Compositor;
class Frame;
class FrameBox;
class Node;
class StackingContext;
class BrowsingContext;

class StackingContextChild : public GCVector<StackingContext*> {
};

struct StackingContextRareData : public gc {
    bool m_needsGraphicsBuffer;
    bool m_hasNon2DRectTransform;
    bool m_isVisibleRectComputedForNonGraphicsLayer;
    LayoutRect m_visibleRect;
    CanvasSurface* m_buffer;
    SkMatrix m_matrix;

    StackingContextRareData();

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(StackingContextRareData, m_buffer));
    }
};

class StackingContext : public gc {
public:
    StackingContext(FrameBox* owner, StackingContext* parent);

    const GCVector<StackingContextChild*>& childContexts()
    {
        return m_childContexts;
    }

    FrameBox* owner()
    {
        return m_owner;
    }

    StackingContext* parent()
    {
        return m_parent;
    }

    bool isRootContext()
    {
        return parent() == nullptr;
    }

    bool needsGraphicsBuffer()
    {
        return m_rareData ? m_rareData->m_needsGraphicsBuffer : false;
    }

    void clearGraphicsBuffer(bool needsDetachNative = true);

    CanvasSurface* buffer()
    {
        return m_rareData ? m_rareData->m_buffer : nullptr;
    }

    LayoutRect visibleRect()
    {
        return m_rareData ? m_rareData->m_visibleRect : LayoutRect(0, 0, 0, 0);
    }

    LayoutLocation transformOrigin();
    void computeTransformMatrix();
    SkMatrix transformMatrix()
    {
        return m_rareData ? m_rareData->m_matrix : SkMatrix::I();
    }

    void computeStackingContextProperties();

    void paintStackingContext(Canvas* canvas, bool needsPainting,
                              bool parentGraphicsLayerNeedsPainting = false);
    void compositeStackingContext(Compositor* compositor);
    Frame* hitTestStackingContext(LayoutUnit x, LayoutUnit y,
                                  BrowsingContext* from);
    LayoutLocation relativeLocation(StackingContext* child);

    int32_t zIndex();

    bool needsRepainting()
    {
        return m_needsRepainting;
    }

    void setNeedsRepainting()
    {
        m_needsRepainting = true;
    }

    bool isIFrameStackingContext();

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(StackingContext, m_rareData));
        GC_set_bit(desc, GC_WORD_OFFSET(StackingContext, m_owner));
        GC_set_bit(desc, GC_WORD_OFFSET(StackingContext, m_parent));
        GC_set_bit(desc, GC_WORD_OFFSET(StackingContext, m_childContexts));
    }

    StackingContextRareData* ensureRareData();

    struct ComputeStackingContextContext;
    void computeStackingContextProperties(ComputeStackingContextContext& ctx);
    void applyStackingContextProperties(ComputeStackingContextContext& ctx);

    bool m_needsRepainting;
    bool m_catchedMatrixChangedWhileComputeStackingContextProperties;
    FrameBox* m_owner;
    StackingContext* m_parent;
    GCVector<StackingContextChild*> m_childContexts;
    StackingContextRareData* m_rareData;
};
}

#endif
