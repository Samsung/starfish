/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishStackingContext__
#define __StarfishStackingContext__

#include "core/page/RenderResult.h"
#include "core/modules/canvas/TextDecorationData.h"

namespace Starfish {

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
    NeedsGraphicsLayerReasonSiblingLayerNeedsAnimation,
    NeedsGraphicsLayerReasonNeedsScroll,
};

class GraphicsBufferHolder : public gc {
    friend class StackingContext;
    friend class WebView;

public:
    GraphicsBufferHolder(size_t bufferWidth, size_t bufferHeight,
                         size_t screenWidth, size_t screenHeight,
                         StackingContext* sc);

    size_t bufferWidth() const
    {
        return m_bufferWidth;
    }

    size_t bufferHeight() const
    {
        return m_bufferHeight;
    }

    size_t horizontalTileCount() const
    {
        return m_horizontalTileCount;
    }

    size_t verticalTileCount() const
    {
        return m_verticalTileCount;
    }

    float additionalPixelRatio() const
    {
        return m_additionalPixelRatio;
    }

    void flushSurfaces();
    void detachNativeBuffers();

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    GCVector<CanvasSurface*> m_surfaces;
    size_t m_bufferWidth;
    size_t m_bufferHeight;
    size_t m_tileDataWidth;
    size_t m_tileDataHeight;
    size_t m_horizontalTileCount;
    size_t m_verticalTileCount;
    float m_additionalPixelRatio;

    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(GraphicsBufferHolder, m_surfaces));
    }
};

class StackingContextChild : public GCVector<StackingContext*> {
};

struct StackingContextRareData : public gc {
    LayoutRect m_visibleRect;
    float m_additionalPixelRatio;
    GraphicsBufferHolder* m_graphicsBufferHolder;
    SkMatrix m_matrix;
    TextDecorationData m_textDecorationData;

    StackingContextRareData();

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(StackingContextRareData,
                                        m_graphicsBufferHolder));
    }
};

class StackingContext : public gc {
public:
    enum RecomputeStackContextReason { PositionFixed, Unknown };
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

    LayoutRect visibleRect();
    float additionalPixelRatio();

    LayoutLocation transformOrigin();
    void computeTransformMatrix();
    SkMatrix transformMatrix()
    {
        return m_rareData ? m_rareData->m_matrix : SkMatrix::I();
    }

    TextDecorationData textDecorationData()
    {
        return m_rareData ? m_rareData->m_textDecorationData
                          : TextDecorationData();
    }

    void computeStackingContextProperties();

    struct PaintingStackingContextContext {
        bool willCompositing;
        PrevDrawnStackingContextInfoMap& prevDrawnStackingContextInfoMap;
        LayoutRect screenClipRect;
        RepaintRegion& repaintRegion;
        LayoutRect layerClipRect;
        LayoutUnit scrollX, scrollY;
        LayoutUnit layerBaseX, layerBaseY;
        LayoutUnit layerScrollX, layerScrollY;
        PaintingStackingContextContext(
            bool willCompositing,
            PrevDrawnStackingContextInfoMap& prevDrawnStackingContextInfoMap,
            const LayoutRect& screenClipRect, RepaintRegion& repaintRegion,
            LayoutUnit scrollX, LayoutUnit scrollY)
            : willCompositing(willCompositing)
            , prevDrawnStackingContextInfoMap(prevDrawnStackingContextInfoMap)
            , screenClipRect(screenClipRect)
            , repaintRegion(repaintRegion)
            , scrollX(scrollX)
            , scrollY(scrollY)
        {
        }
    };
    void paintStackingContext(Canvas* canvas,
                              PaintingStackingContextContext& ctx);
    void paintScrollbar(Canvas* canvas);
    bool fillGraphicsBufferContents(PaintingStackingContextContext& globalCtx);
    bool fillGraphicsBufferContentsWithoutClipRect();
    void compositeStackingContext(Compositor* compositor);
    void compositeScrollbar(Compositor* compositor);
    Frame* hitTestStackingContext(LayoutUnit x, LayoutUnit y,
                                  BrowsingContext* from);
    LayoutLocation relativeLocation(StackingContext* child);

    int32_t zIndex();

    NeedsGraphicsLayerReason needsGraphicsBufferReason()
    {
        return m_needsGraphicsBufferReason;
    }

    bool needsComposite()
    {
        return needsGraphicsBufferReason() || needsGraphicsBuffer();
    }

    bool hasFilterEffect()
    {
        return m_hasFilterEffect;
    }

    const LayoutRect& screenExtent()
    {
        return m_screenExtent;
    }

    bool isIFrameStackingContext();
    bool isIFrameStackingContextOwner();

    bool isAncestorOf(StackingContext* f)
    {
        while (f) {
            if (f == this) {
                return true;
            }
            f = f->parent();
        }
        return false;
    }

    GraphicsBufferHolder* graphicsBufferHolder()
    {
        if (m_rareData) {
            return m_rareData->m_graphicsBufferHolder;
        }
        return nullptr;
    }
    void clearGraphicsBuffer();

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
    struct ApplyPropertiesPostProcessingContext {
        ApplyPropertiesPostProcessingContext()
            : baseAdditionalPixelRatio(1)
        {
        }
        float baseAdditionalPixelRatio;
        GCVector<StackingContext*> stackingContextsNeedsGraphicsBuffer;
    };
    void applyStackingContextPropertiesPostProcessing(
        ApplyPropertiesPostProcessingContext& ctx);
    void fillGraphicsBufferContents(Canvas* canvas,
                                    PaintingStackingContextContext& ctx);
    void applyMask(Canvas* canvas, PaintingStackingContextContext& ctx);

    bool m_needsGraphicsBuffer : 1;
    bool m_hasNon2DRectTransform : 1;
    bool m_isVisibleRectComputedForNonGraphicsLayer : 1;
    bool m_hasFilterEffect : 1;
    NeedsGraphicsLayerReason m_needsGraphicsBufferReason : 3;
    FrameBox* m_owner;
    StackingContext* m_parent;
    GCVector<StackingContextChild*> m_childContexts;
    GCVector<StackingContext*> m_ancestorsThatHasFilters;
    StackingContextRareData* m_rareData;
    LayoutRect m_screenExtent;
};
} // namespace Starfish

#endif
