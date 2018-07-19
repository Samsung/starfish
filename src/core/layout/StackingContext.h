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

#include "core/page/RenderResult.h"

namespace StarFish {

class Canvas;
class CanvasSurface;
class Compositor;
class Frame;
class FrameBox;
class Node;
class StackingContext;
class BrowsingContext;

enum NeedsGraphicsLayerReason ENSURE_ENUM_UNSIGNED {
    NeedsGraphicsLayerReasonNone,
    NeedsGraphicsLayerReasonBySelf,
    NeedsGraphicsLayerReasonNotCoveredByParent,
    NeedsGraphicsLayerReasonCollapsedWithSiblingLayer,
};

class StackingContextChild : public GCVector<StackingContext*> {
};

struct StackingContextRareData : public gc {
    LayoutRect m_visibleRect;
    CanvasSurface* m_buffer;
    SkMatrix m_matrix;
    SkMatrix m_screenMatrix;

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

    GCVector<StackingContext*>& ancestorsThatHasFilters()
    {
        return m_ancestorsThatHasFilters;
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
        return m_needsGraphicsBuffer;
    }

    void clearGraphicsBuffer();

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

    struct PaintingStackingContextContext {
        bool willCompositing;
        PrevDrawnStackingContextInfoMap& prevDrawnStackingContextInfoMap;
        LayoutRect screenClipRect;
        LayoutRect layerClipRect;
        LayoutUnit scrollX, scrollY;
        PaintingStackingContextContext(
            bool willCompositing,
            PrevDrawnStackingContextInfoMap& prevDrawnStackingContextInfoMap,
            const LayoutRect& screenClipRect, LayoutUnit scrollX,
            LayoutUnit scrollY)
            : willCompositing(willCompositing)
            , prevDrawnStackingContextInfoMap(prevDrawnStackingContextInfoMap)
            , screenClipRect(screenClipRect)
            , scrollX(scrollX)
            , scrollY(scrollY)
        {
        }
    };
    void paintStackingContext(Canvas* canvas,
                              PaintingStackingContextContext& ctx);
    void compositeStackingContext(Compositor* compositor);
    Frame* hitTestStackingContext(LayoutUnit x, LayoutUnit y,
                                  BrowsingContext* from);
    LayoutLocation relativeLocation(StackingContext* child);

    int32_t zIndex();

    NeedsGraphicsLayerReason needsGraphicsBufferReason()
    {
        return m_needsGraphicsBufferReason;
    }

    const LayoutRect& screenExtent()
    {
        return m_screenExtent;
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
        GC_set_bit(desc,
                   GC_WORD_OFFSET(StackingContext, m_ancestorsThatHasFilters));
    }

    StackingContextRareData* ensureRareData();

    struct ComputeStackingContextContext;
    void computeStackingContextProperties(ComputeStackingContextContext& ctx);
    void applyStackingContextProperties(ComputeStackingContextContext& ctx);

    bool m_catchedMatrixChangedWhileComputeStackingContextProperties : 1;
    bool m_needsGraphicsBuffer : 1;
    bool m_hasNon2DRectTransform : 1;
    bool m_isVisibleRectComputedForNonGraphicsLayer : 1;
    NeedsGraphicsLayerReason m_needsGraphicsBufferReason : 2;
    FrameBox* m_owner;
    StackingContext* m_parent;
    GCVector<StackingContextChild*> m_childContexts;
    GCVector<StackingContext*> m_ancestorsThatHasFilters;
    StackingContextRareData* m_rareData;
    LayoutRect m_screenExtent;
};
}

#endif
