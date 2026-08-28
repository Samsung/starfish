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

// Why scrolling this layer cannot be a pure composite (tile translate) and
// must repaint instead. Kept as an enum so callers can report the exact
// blocking condition (e.g. scroll-performance diagnostics).
enum RepaintingWhenScrollingReason ENSURE_ENUM_UNSIGNED {
    RepaintingWhenScrollingReasonNone,
    RepaintingWhenScrollingReasonNoGraphicsBuffer,
    RepaintingWhenScrollingReasonBorder,
    RepaintingWhenScrollingReasonBoxShadow,
    RepaintingWhenScrollingReasonOutline,
    RepaintingWhenScrollingReasonBackgroundSize,
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

    size_t tileBufferWidth() const
    {
        return m_tileDataWidth * m_horizontalTileCount;
    }

    size_t tileBufferHeight() const
    {
        return m_tileDataHeight * m_verticalTileCount;
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

class StackingContextChild : public GCVector<StackingContext*> {};

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
    // visibleRect() minus its own-frame seed: only what the traversal
    // actually united (things that paint). This is what the composed
    // recursion contributes to ancestors - the seed is a lower bound for
    // this layer's own buffer, not painted content, and leaking it upward
    // gave paints-nothing wrappers a phantom rect (WPT
    // translation-animation-subpixel-offset and the color-scheme
    // iframe-background-mismatch-dynamic pair caught this).
    LayoutRect visibleRectContentOnly();
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
        Optional<StackingContext*> paintingForCompositingStartingFrom;
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

    // True when this context or any descendant context composites into its
    // own graphics buffer. Such subtrees must always be visited during a
    // paint walk (paintStackingContext captures per-layer state for them),
    // so clip-based culling skips only subtrees where this is false.
    // Cached per rendered frame (FrameBox::g_paintExtentEpoch).
    bool subtreeContainsGraphicsBufferLayer();

    // Saves this context's live visibleRect into the carry-over map keyed
    // by its owner node, so the context recreated by a full re-establish
    // can adopt it (restorePrevVisibleRectIfPossible). Contexts that are
    // dirty, never computed, or anonymous-owned are not saved - their
    // successors recompute cold.
    void collectPrevVisibleRect(PrevStackingContextVisibleRectMap& map);

    // Marks this context's visibleRect (and every ancestor's, since a
    // subtree rect feeds each enclosing context's union) as needing a
    // recompute on the next stacking-context properties pass. Called from
    // the same mutation points that request that pass; contexts not marked
    // keep their cached rect across the pass.
    void markVisibleRectDirtyUpward()
    {
        StackingContext* c = this;
        while (c && !c->m_visibleRectDirty) {
            c->m_visibleRectDirty = true;
            c = c->parent();
        }
    }

    // For a needsGraphicsBuffer() context, a paint-walk visit's only
    // observable effect is capturing the text-decoration state merged along
    // the ancestor path (paintStackingContext returns right after). The full
    // ComputeOverflow path resets that state first, so when every merge on
    // the path is a known no-op the captured value is simply the default -
    // store it directly and report the visit as handled, skipping the
    // ComputeOverflow ancestor replay. Cached per rendered frame.
    bool tryFastBufferedLayerVisit();

    // Recursively fast-captures the state of every graphics-buffer layer in
    // this (non-buffered) context's subtree via tryFastBufferedLayerVisit().
    // When it returns true the subtree visit has no side effects left beyond
    // pixels, so the visit may be culled by the clip test like any plain
    // subtree. Cached per rendered frame.
    bool tryFastCaptureBufferedDescendants();

    // visibleRect() placed at this context's position inside its parent
    // context's coordinate space (absolutePointIncludingScroll from the
    // parent's owner) - the rect the child-SC clip culling tests. The
    // ancestor walk behind it repeats identically for every tile filled in
    // a frame, so cache the result per rendered frame.
    LayoutRect cullRectInParentSpace();

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

    RepaintingWhenScrollingReason repaintingWhenScrollingReason();
    bool needsRepaintingWhenScrolling();
    bool needsToDrawScrollbar();
    bool inScrollActive();
    bool inScrollWithGraphicsBufferActive()
    {
        return needsGraphicsBuffer() && inScrollActive();
    }

    // True when this layer's own background-color may be drawn by the
    // compositor (as a full border-box fill, rounded when border-radius is
    // present) instead of being baked into the graphics buffer, so the buffer
    // can be sized to content only. Must be consistent across visibleRect
    // sizing, background painting and composite.
    bool isOwnerBackgroundDrawnByCompositor();
    // Draws the owner's background-color directly with the compositor, filling
    // a rounded path when the owner has border-radius. Used at the composite
    // sites that elide the graphics buffer per
    // isOwnerBackgroundDrawnByCompositor.
    void drawOwnerBackgroundByCompositor(Compositor* compositor);

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

    // subtreeContainsGraphicsBufferLayer() cache; valid while the epoch
    // matches FrameBox::g_paintExtentEpoch. Plain data, no GC pointers.
    bool m_subtreeContainsGraphicsBufferLayer { false };
    uint32_t m_subtreeGBLayerEpoch { 0 };

    void restorePrevVisibleRectIfPossible();

    // Set by markVisibleRectDirtyUpward(); consumed by
    // applyStackingContextProperties, which only then invalidates or
    // recomputes this context's visibleRect. A fresh context starts clean:
    // its never-computed state (flag/rect) already forces the first
    // computation, and staying clean is what lets a carried-over rect
    // survive the pass after a full re-establish.
    bool m_visibleRectDirty { false };

    // tryFastBufferedLayerVisit() cache, same epoch scheme.
    bool m_fastBufferedVisitOk { false };
    uint32_t m_fastBufferedVisitEpoch { 0 };

    // tryFastCaptureBufferedDescendants() cache, same epoch scheme.
    bool m_fastCaptureDescendantsOk { false };
    uint32_t m_fastCaptureDescendantsEpoch { 0 };

    // cullRectInParentSpace() cache, same epoch scheme.
    LayoutRect m_cullRectInParentSpace;
    uint32_t m_cullRectEpoch { 0 };

    // Set alongside m_rareData->m_visibleRect (same validity flag).
    LayoutRect m_visibleRectContentOnly;
};

} // namespace Starfish

#endif
