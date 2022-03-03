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

#include "StarfishConfig.h"

#include "core/layout/StackingContext.h"

#include "Starfish.h"
#include "core/dom/Node.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/Scrolling.h"
#include "core/animation/AnimationTask.h"
#include "core/style/FilterFunctions.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/FrameReplaced.h"
#include "core/layout/OverflowStatus.h"
#include "core/page/BrowsingContext.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "platform/window/PlatformWindow.h"
#include "core/modules/canvas/ShadowBlur.h"
#include "core/modules/canvas/image/BufferedNativeImageData.h"
#include "core/style/CSSGradientValue.h"
#include "core/style/GradientData.h"
#include "core/modules/canvas/NativeGradient.h"

namespace Starfish {

inline void computeBufferSizeFromVisibleRect(LayoutUnit minX, LayoutUnit minY,
                                             LayoutUnit maxX, LayoutUnit maxY,
                                             size_t& bufferWidth,
                                             size_t& bufferHeight)
{
#if defined(STARFISH_ENABLE_TEST)
    bufferWidth = (int)(maxX - minX);
    bufferHeight = (int)(maxY - minY);
#else
    bufferWidth = (maxX - minX).round();
    bufferHeight = (maxY - minY).round();
#endif
}

struct StackingContext::ComputeStackingContextContext {
    bool needsToAllocateGraphicsBufferForFixedElement;
    bool seenPositionFixed;
    std::unordered_map<StackingContext*, LayoutRect> extentPerLayer;
    std::unordered_map<StackingContext*, LayoutRect> clippedExtentPerLayer;
    std::unordered_map<StackingContext*, bool> compositeFlagInfo;
    std::unordered_map<StackingContext*, bool> compositeFlagInfoBecauseSelf;
    std::vector<StackingContext*> compositedLayers;
    std::set<Document*> compositedDocuments;
    std::vector<StackingContext*> documentOwners;
    StackingContext* rootLayer;

    ComputeStackingContextContext(StackingContext* rootLayer)
        : rootLayer(rootLayer)
    {
        seenPositionFixed = false;
        needsToAllocateGraphicsBufferForFixedElement = false;
    }

    LayoutRect screenExtentPerLayer(StackingContext* c)
    {
        {
            auto iter = extentPerLayer.find(c);
            if (iter != extentPerLayer.end()) {
                return iter->second;
            }
        }

        LayoutRect rt = c->owner()->computeScreenExtent();
        extentPerLayer.insert(std::make_pair(c, rt));

        return rt;
    }

    // screenExtent after overflow applies
    LayoutRect clippedScreenExtentPerLayer(StackingContext* c)
    {
        STARFISH_ASSERT(c != nullptr);

        {
            auto iter = clippedExtentPerLayer.find(c);
            if (iter != clippedExtentPerLayer.end()) {
                return iter->second;
            }
        }

        LayoutRect rt = screenExtentPerLayer(c);

        if (c->owner()->style()->position() == FixedPositionValue ||
            c->isIFrameStackingContext()) {
            return rt;
        }

        Frame* f = c->owner()->layoutParent();

        while (f != nullptr) {
            if (f->isFrameBox() && f->asFrameBox()->stackingContext() &&
                f->asFrameBox()->stackingContext()->isIFrameStackingContext()) {
                break;
            }

            if (f->shouldApplyOverflow()) {
                LayoutRect parentExtent =
                    f->asFrameBox()->computeScreenExtent();
                rt = LayoutRect::overlappedRect(parentExtent, rt);

                if (rt.isEmpty() ||
                    f->style()->position() == FixedPositionValue) {
                    break;
                }
            }
            f = f->layoutParent();
        }

        clippedExtentPerLayer.insert(std::make_pair(c, rt));
        return rt;
    }

    void pushCompsitedLayer(StackingContext* c)
    {
        STARFISH_ASSERT(c != nullptr);
        STARFISH_ASSERT(!isCompsitedLayer(c));

        if (seenPositionFixed) {
            throw RecomputeStackContextReason::PositionFixed;
        }
        compositedLayers.push_back(c);
        compositedDocuments.insert(c->m_owner->node()->document());
    }

    bool isCompsitedLayer(StackingContext* c, size_t* idx = nullptr)
    {
        STARFISH_ASSERT(c != nullptr);
        for (size_t i = 0; i < compositedLayers.size(); i++) {
            if (compositedLayers.at(i) == c) {
                if (idx) {
                    *idx = i;
                }
                return true;
            }
        }
        return false;
    }

    bool seenCompsitedLayer()
    {
        return compositedLayers.size();
    }
};

GraphicsBufferHolder::GraphicsBufferHolder(CanvasSurface* s)
    : m_bufferWidth(s->bufferWidth())
    , m_bufferHeight(s->bufferHeight())
    , m_tileDataWidth(m_bufferWidth)
    , m_tileDataHeight(m_bufferHeight)
    , m_horizontalTileCount(1)
    , m_verticalTileCount(1)
    , m_additionalPixelRatio(1)
{
    m_surfaces.push_back(s);
}

GraphicsBufferHolder::GraphicsBufferHolder(size_t bufferWidth,
                                           size_t bufferHeight,
                                           size_t screenWidth,
                                           size_t screenHeight,
                                           StackingContext* sc)
    : m_bufferWidth(bufferWidth)
    , m_bufferHeight(bufferHeight)
    , m_tileDataWidth(bufferWidth)
    , m_tileDataHeight(bufferHeight)
    , m_horizontalTileCount(1)
    , m_verticalTileCount(1)
    , m_additionalPixelRatio(sc->additionalPixelRatio())
{
    STARFISH_ASSERT(m_bufferWidth);
    STARFISH_ASSERT(m_bufferHeight);

    bool dontSplitGraphicsBufferCond = false;

    if (sc->owner()->style()->hasFilter()) {
        dontSplitGraphicsBufferCond = true;
    }

    LayoutRect screenRect(0, 0, screenWidth, screenHeight);
    // if buffer is smaller than screen && whole content will be shown on
    // screen
    // we don't need to divide buffer
    if (screenRect.containsInVisual(sc->screenExtent()) &&
        bufferWidth <= screenWidth && bufferHeight <= screenHeight) {
        dontSplitGraphicsBufferCond = true;
    }

    if (dontSplitGraphicsBufferCond) {
        m_tileDataWidth = m_bufferWidth;
        m_horizontalTileCount = 1;
        m_tileDataHeight = m_bufferHeight;
        m_verticalTileCount = 1;

        m_surfaces.resize(1);
        m_surfaces[0] = nullptr;
    } else {
        size_t tileSize =
            ceil(CanvasSurface::g_canvasSurfaceTileSize /
                 sc->owner()->node()->webView()->screenInfo().devicePixelRatio);

        float effectiveWidth = m_bufferWidth * sc->additionalPixelRatio();
        float effectiveHeight = m_bufferHeight * sc->additionalPixelRatio();

        size_t wTextureCount = 1;
        while (effectiveWidth / wTextureCount > tileSize) {
            wTextureCount++;
        }
        m_tileDataWidth = ceil(effectiveWidth / (float)wTextureCount);
        m_horizontalTileCount = wTextureCount;

        size_t hTextureCount = 1;
        while (effectiveHeight / hTextureCount > tileSize) {
            hTextureCount++;
        }
        m_tileDataHeight = ceil(effectiveHeight / (float)hTextureCount);
        m_verticalTileCount = hTextureCount;

        m_surfaces.resize(wTextureCount * hTextureCount);
        for (size_t i = 0; i < m_surfaces.size(); i++) {
            m_surfaces[i] = nullptr;
        }
    }
}

void GraphicsBufferHolder::flushSurfaces()
{
    for (size_t i = 0; i < m_surfaces.size(); i++) {
        if (m_surfaces[i]) {
            m_surfaces[i]->detachNativeBuffer();
            m_surfaces[i] = nullptr;
        }
    }
}

void GraphicsBufferHolder::detachNativeBuffers()
{
    for (size_t i = 0; i < m_surfaces.size(); i++) {
        if (m_surfaces[i]) {
            m_surfaces[i]->detachNativeBuffer();
        }
        m_surfaces[i] = nullptr;
    }
    m_surfaces.clear();

    m_tileDataWidth = 0;
    m_tileDataHeight = 0;
    m_horizontalTileCount = 0;
    m_verticalTileCount = 0;
    m_bufferWidth = 0;
    m_bufferHeight = 0;
}

void* GraphicsBufferHolder::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(GraphicsBufferHolder));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(GraphicsBufferHolder)] = { 0 };
        GraphicsBufferHolder::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(GraphicsBufferHolder));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

StackingContextRareData::StackingContextRareData()
    : m_visibleRect(0, 0, 0, 0)
    , m_additionalPixelRatio(1)
    , m_graphicsBufferHolder(nullptr)
    , m_matrix(SkMatrix::I())
{
}

void* StackingContextRareData::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(StackingContextRareData));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(StackingContextRareData)] = { 0 };
        StackingContextRareData::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(StackingContextRareData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

StackingContext::StackingContext(FrameBox* owner, StackingContext* parent)
    : m_needsGraphicsBuffer(false)
    , m_hasNon2DRectTransform(false)
    , m_isVisibleRectComputedForNonGraphicsLayer(false)
    , m_hasFilterEffect(false)
    , m_needsGraphicsBufferReason(
          NeedsGraphicsLayerReason::NeedsGraphicsLayerReasonNone)
    , m_owner(owner)
    , m_parent(parent)
    , m_rareData(nullptr)
{
    if (m_parent) {
        int32_t num = zIndex();
        auto iter = m_parent->m_childContexts.rbegin();
        size_t idx = m_parent->m_childContexts.size();
        StackingContextChild* target = nullptr;
        while (iter != m_parent->m_childContexts.rend()) {
            StackingContextChild* child = *iter;

            if (child->at(0)->zIndex() == num) {
                target = child;
                break;
            } else if (child->at(0)->zIndex() < num) {
                target = new StackingContextChild();
                m_parent->m_childContexts.insert(idx, target);
                break;
            }

            idx--;
            iter++;
        }
        if (!target) {
            target = new StackingContextChild();
            m_parent->m_childContexts.insert(m_parent->m_childContexts.begin(),
                                             target);
        }
        target->push_back(this);

        StackingContext* ancestor = m_parent;
        while (ancestor) {
            if (ancestor->owner()->style()->hasAvailableFilter()) {
                ancestorsThatHasFilters().push_back(ancestor);
            }
            ancestor = ancestor->parent();
        }
    }
}

void* StackingContext::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(StackingContext));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(StackingContext)] = { 0 };
        StackingContext::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(StackingContext));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

StackingContextRareData* StackingContext::ensureRareData()
{
    if (!m_rareData) {
        m_rareData = new StackingContextRareData();
    }
    return m_rareData;
}

int32_t StackingContext::zIndex()
{
    if (m_owner->isPositioned() ||
        (m_owner->isFlexItem() && m_owner->isSpecifiedZIndex())) {
        return m_owner->style()->zIndex();
    } else {
        return 0;
    }
}

void StackingContext::clearGraphicsBuffer()
{
    if (m_rareData && m_rareData->m_graphicsBufferHolder) {
        m_rareData->m_graphicsBufferHolder->detachNativeBuffers();
        m_rareData->m_graphicsBufferHolder = nullptr;
    }
}

bool StackingContext::isIFrameStackingContext()
{
    if (m_owner->layoutParent() && m_owner->layoutParent()->isFrameDocument()) {
        if (!m_owner->node()
                 ->document()
                 ->browsingContext()
                 ->isTopLevelBrowsingContext()) {
            return true;
        }
    }
    return false;
}

bool StackingContext::isIFrameStackingContextOwner()
{
    if (m_owner->node()->isHTMLIFrameElement()) {
        return true;
    }
    return false;
}

void StackingContext::flushGraphicsBuffer()
{
    STARFISH_ASSERT(needsGraphicsBuffer());
    STARFISH_ASSERT(!m_owner->hasOwnGraphicsBufferMethod());

    if (m_rareData && m_rareData->m_graphicsBufferHolder) {
        m_rareData->m_graphicsBufferHolder->flushSurfaces();
    }
}

class CanvasStateRestorer {
private:
    std::vector<std::pair<Frame*, std::pair<bool, bool>>>
        m_canApplyOverflowOrScrolls;

    void insertIntoCanApplyOverflowOrScrolls(Frame* f, std::pair<bool, bool> v)
    {
        auto len = m_canApplyOverflowOrScrolls.size();
        for (size_t i = 0; i < len; i++) {
            if (m_canApplyOverflowOrScrolls[i].first == f) {
                m_canApplyOverflowOrScrolls[i].second = v;
                return;
            }
        }
        m_canApplyOverflowOrScrolls.push_back(std::make_pair(f, v));
    }

    std::pair<bool, bool> readFromCanApplyOverflowOrScrolls(Frame* f)
    {
        auto len = m_canApplyOverflowOrScrolls.size();
        for (size_t i = 0; i < len; i++) {
            if (m_canApplyOverflowOrScrolls[i].first == f) {
                return m_canApplyOverflowOrScrolls[i].second;
            }
        }
        return std::make_pair(false, false);
    }

public:
    CanvasStateRestorer(
        Canvas* canvas, StackingContext* sCtx, FrameBox* owner,
        const StackingContext::PaintingStackingContextContext& ctx)
        : m_canvas(canvas)
        , m_ownerContext(sCtx)
    {
        canvas->save();
        FrameBox* self = sCtx->owner();

        // TODO CanvasStateRestorer, CompositorStateRestorer,
        // Frame::ComputeVisibleRectContext::uniteRect have same source

        if (self->style()->position() != FixedPositionValue) {
            bool needsRestore = false;
            Frame* s = self->layoutParent();
            while (s) {
                if ((s->shouldApplyOverflow() &&
                     (!s->needToEstablishStackingContext() ||
                      (s->isFrameBox() &&
                       !s->asFrameBox()->canOwnsStackingContext()))) ||
                    (s->style() &&
                     s->style()->position() == FixedPositionValue)) {
                    needsRestore = true;
                    break;
                }
                s = s->layoutParent();
            }

            if (!needsRestore) {
                auto o = self->absolutePointIncludingScroll(owner, false);
                canvas->translate(o.x(), o.y());
                return;
            }
        }

        VectorWithInlineStorage<32, FrameBox*, std::allocator<FrameBox*>>
            frameList;

        m_canApplyOverflowOrScrolls.reserve(32);

        Frame* nearstBufferedFrame = nullptr;
        bool shareWithStackingBuffer = true;
        {
            Frame* f = self;
            OverflowStatus status(f);
            bool canScroll = OverflowStatus::isScrollableFrame(f);

            while (f) {
                frameList.push_back(f->asFrameBox());
                f = f->layoutParent();

                if (shareWithStackingBuffer && f &&
                    f->asFrameBox()->stackingContext() &&
                    f->asFrameBox()->stackingContext()->needsGraphicsBuffer() &&
                    f->asFrameBox()->stackingContext()->isAncestorOf(sCtx)) {
                    nearstBufferedFrame = f;
                    shareWithStackingBuffer = false;
                }

                if (shareWithStackingBuffer) {
                    bool applyOverflow = status.canApplyOverflow(f);
                    if (applyOverflow) {
                        status.reset(f);
                        canScroll = status.m_child->style()->position() !=
                                    FixedPositionValue;
                        insertIntoCanApplyOverflowOrScrolls(
                            f, std::make_pair(true, canScroll && f &&
                                                        f->isFrameBlockBox()));
                    } else {
                        if (status.m_seenAbsBlock &&
                            !status.m_seenContainingBlockForAbsBlock) {
                            canScroll = false;
                        }

                        insertIntoCanApplyOverflowOrScrolls(
                            f, std::make_pair(false, canScroll && f &&
                                                         f->isFrameBlockBox()));
                    }
                }

                if (canScroll) {
                    if (f && f->style() &&
                        f->style()->position() == FixedPositionValue) {
                        canScroll = false;
                    }
                }
            }
        }

        canvas->resetMatrixAndClip();
        canvas->resetTextDecorationData();

        if (!ctx.willCompositing) {
            canvas->pixelSnappedClip(ctx.screenClipRect);
        }

        StackingContext* sc = sCtx->parent();
        while (true) {
            if (sc == nullptr) {
                break;
            }
            if (sc->needsGraphicsBuffer()) {
                LayoutRect visibleRect = sc->visibleRect();
                LayoutUnit minX = visibleRect.x();
                LayoutUnit minY = visibleRect.y();

                if (ctx.willCompositing) {
                    canvas->pixelSnappedClip(ctx.layerClipRect);
                    canvas->translate(-ctx.layerBaseX - ctx.layerScrollX,
                                      -ctx.layerBaseY - ctx.layerScrollY);
                }
                canvas->translate(-minX, -minY);
                break;
            }
            sc = sc->parent();
        }

        auto iter = frameList.rbegin();
        shareWithStackingBuffer = nearstBufferedFrame ? false : true;
        LayoutUnit dx, dy;
        while (iter != frameList.rend()) {
            FrameBox* b = *iter;
            StackingContext* ctx = b->stackingContext();

            if (b->style()) {
                if (b != self) {
                    if (b->shouldResetTextDecoration()) {
                        canvas->resetTextDecorationData();
                    } else {
                        canvas->mergeTextDecorationData(b->style());
                    }
                }
            }

            if (nearstBufferedFrame && nearstBufferedFrame == b) {
                shareWithStackingBuffer = true;
            }

            if (!shareWithStackingBuffer) {
                iter++;
                continue;
            }

            if (!(nearstBufferedFrame && nearstBufferedFrame == b)) {
                dx += b->x();
                dy += b->y();
            }

            if (b->style()) {
                auto overflowOrScroll = readFromCanApplyOverflowOrScrolls(b);

                if (b != self) {
                    if (b != nearstBufferedFrame) {
                        StackingContext* ctx = b->stackingContext();
                        if (ctx) {
                            SkMatrix m =
                                b->stackingContext()->transformMatrix();
                            if (!m.isIdentity()) {
                                canvas->translate(dx, dy);
                                dx = dy = 0;

                                auto o =
                                    b->stackingContext()->transformOrigin();
                                canvas->translate(o.x(), o.y());
                                canvas->postMatrix(m);
                                canvas->translate(-o.x(), -o.y());
                            }
                        }
                    }

                    if (overflowOrScroll.first) {
                        Unit::Rect rt(b->borderLeft() + dx, b->borderTop() + dy,
                                      b->width() - b->borderWidth(),
                                      b->height() - b->borderHeight());
                        canvas->clip(rt);
                        if (b->hasFrameBorderRadius()) {
                            canvas->translate(dx, dy);
                            const LayoutRect rect(0, 0, b->width(),
                                                  b->height());
                            b->applyBorderRadiusClippingIfNeeds(canvas, rect);
                            canvas->translate(-dx, -dy);
                        }
                    }

                    if (b->isAbsolutePositioned()) {
                        RectData* rect = b->style()->clip();
                        if (rect) {
                            canvas->translate(dx, dy);
                            canvas->clip(
                                Unit::Rect(rect->left().numberData(),
                                           rect->top().numberData(),
                                           rect->right().numberData(),
                                           rect->bottom().numberData()));
                            canvas->translate(-dx, -dy);
                        }
                    }

                    if (overflowOrScroll.second) {
                        dx += -b->asFrameBlockBox()->scrollLeft();
                        dy += -b->asFrameBlockBox()->scrollTop();
                    }
                }
            }

            if (ctx && ctx->isIFrameStackingContext() &&
                nearstBufferedFrame != b) {
                FrameBlockBox* document =
                    ctx->owner()->layoutParent()->asFrameBlockBox();
                FrameBox* iframeBox = ctx->owner()
                                          ->node()
                                          ->document()
                                          ->browsingContext()
                                          ->sourceElement()
                                          ->frame()
                                          ->asFrameBox();
                canvas->translate(
                    iframeBox->borderLeft() + iframeBox->paddingLeft(),
                    iframeBox->borderTop() + iframeBox->paddingTop());
            }

            iter++;
        }
        canvas->translate(dx, dy);
    }
    ~CanvasStateRestorer()
    {
        m_canvas->restore();
    }

    Canvas* m_canvas;
    StackingContext* m_ownerContext;
};

class CompositorStateRestorer {
public:
    std::vector<std::pair<Frame*, std::pair<bool, bool>>>
        m_canApplyOverflowOrScrolls;

    void insertIntoCanApplyOverflowOrScrolls(Frame* f, std::pair<bool, bool> v)
    {
        auto len = m_canApplyOverflowOrScrolls.size();
        for (size_t i = 0; i < len; i++) {
            if (m_canApplyOverflowOrScrolls[i].first == f) {
                m_canApplyOverflowOrScrolls[i].second = v;
                return;
            }
        }
        m_canApplyOverflowOrScrolls.push_back(std::make_pair(f, v));
    }

    std::pair<bool, bool> readFromCanApplyOverflowOrScrolls(Frame* f)
    {
        auto len = m_canApplyOverflowOrScrolls.size();
        for (size_t i = 0; i < len; i++) {
            if (m_canApplyOverflowOrScrolls[i].first == f) {
                return m_canApplyOverflowOrScrolls[i].second;
            }
        }
        return std::make_pair(false, false);
    }

public:
    CompositorStateRestorer(Compositor* compositor, StackingContext* sCtx,
                            FrameBox* owner)
        : m_compositor(compositor)
        , m_opacity(1)
    {
        compositor->save();

        if (!owner) {
            return;
        }

        FrameBox* self = sCtx->owner();
        // TODO CanvasStateRestorer, CompositorStateRestorer,
        // Frame::ComputeVisibleRectContext::uniteRect have same source

        VectorWithInlineStorage<32, FrameBox*, std::allocator<FrameBox*>>
            frameList;

        m_canApplyOverflowOrScrolls.reserve(32);

        {
            Frame* f = self;

            OverflowStatus status(f);
            bool canScroll = OverflowStatus::isScrollableFrame(f);

            while (f) {
                frameList.push_back(f->asFrameBox());
                f = f->layoutParent();

                bool applyOverflow = status.canApplyOverflow(f);
                if (applyOverflow) {
                    status.reset(f);
                    canScroll = status.m_child->style()->position() !=
                                FixedPositionValue;
                    insertIntoCanApplyOverflowOrScrolls(
                        f, std::make_pair(true, canScroll && f &&
                                                    f->isFrameBlockBox()));
                } else {
                    if (status.m_seenAbsBlock &&
                        !status.m_seenContainingBlockForAbsBlock) {
                        canScroll = false;
                    }

                    insertIntoCanApplyOverflowOrScrolls(
                        f, std::make_pair(false, canScroll && f &&
                                                     f->isFrameBlockBox()));
                }

                if (canScroll) {
                    if (f && f->style() &&
                        f->style()->position() == FixedPositionValue) {
                        canScroll = false;
                    }
                }

            }
        }

        compositor->resetMatrixAndClip();
        float opacity = 1;

        auto iter = frameList.rbegin();
        while (iter != frameList.rend()) {
            FrameBox* b = *iter;
            StackingContext* ctx = b->stackingContext();
            compositor->translate(b->x(), b->y());
            ComputedStyle* style = b->style();
            if (style) {
                auto overflowOrScroll = readFromCanApplyOverflowOrScrolls(b);

                if (ctx) {
                    SkMatrix m = ctx->transformMatrix();
                    if (!m.isIdentity()) {
                        auto o = ctx->transformOrigin();
                        compositor->translate(o.x(), o.y());
                        compositor->postMatrix(m);
                        compositor->translate(-o.x(), -o.y());
                    }

                    float n = style->opacity();
                    if (n != 1) {
                        opacity = opacity * n;
                    }

                    SkMatrix test;
                    if (!compositor->currentTransformMatrix().invert(&test)) {
                        compositor->postMatrix(SkMatrix::InvalidMatrix());
                        return;
                    }
                }

                if (overflowOrScroll.first && self != b) {
                    Unit::Rect rt(b->borderLeft(), b->borderTop(),
                                  b->width() - b->borderWidth(),
                                  b->height() - b->borderHeight());
                    compositor->clip(rt);
                    if (b->hasFrameBorderRadius()) {
                        const LayoutRect rect(0, 0, b->width(), b->height());
                        b->applyBorderRadiusClippingIfNeeds(compositor, rect);
                    }
                }

                if (style->isAbsolutePositioned()) {
                    RectData* rect = style->clip();
                    if (rect) {
                        compositor->clip(Unit::Rect(
                            rect->left().numberData(), rect->top().numberData(),
                            rect->right().numberData(),
                            rect->bottom().numberData()));
                    }
                }

                if (overflowOrScroll.second && sCtx->owner() != b) {
                    compositor->translate(-b->asFrameBlockBox()->scrollLeft(),
                                          -b->asFrameBlockBox()->scrollTop());
                }

                if (ctx && ctx->isIFrameStackingContext()) {
                    FrameBlockBox* document =
                        ctx->owner()->layoutParent()->asFrameBlockBox();
                    FrameBox* iframeBox = ctx->owner()
                                              ->node()
                                              ->document()
                                              ->browsingContext()
                                              ->sourceElement()
                                              ->frame()
                                              ->asFrameBox();
                    compositor->translate(
                        iframeBox->borderLeft() + iframeBox->paddingLeft(),
                        iframeBox->borderTop() + iframeBox->paddingTop());
                }
            }
            iter++;
        }

        m_opacity = opacity;
        if (m_opacity != 1) {
            compositor->beginOpacityLayer(m_opacity);
        }
    }

    ~CompositorStateRestorer()
    {
        if (m_opacity != 1) {
            m_compositor->endOpacityLayer();
        }
        m_compositor->restore();
    }

    Compositor* m_compositor;
    float m_opacity;
};

LayoutLocation StackingContext::transformOrigin()
{
    LayoutUnit ox = m_owner->width() / 2;
    LayoutUnit oy = m_owner->height() / 2;
    ComputedStyle* cs = m_owner->style();
    if (cs->hasTransformOrigin()) {
        auto od = cs->transformOrigin()->originValue();
        ox = od->getXAxis().specifiedValue(m_owner->width(), m_owner);
        oy = od->getYAxis().specifiedValue(m_owner->height(), m_owner);
    }
    return LayoutLocation(ox, oy);
}

void StackingContext::computeTransformMatrix()
{
    ComputedStyle* cs = m_owner->style();
    if (cs->hasTransforms(m_owner)) {
        ensureRareData();
        m_rareData->m_matrix = cs->transformsToMatrix(
            m_owner->width(), m_owner->height(), m_owner, true);

        m_hasNon2DRectTransform = m_owner->style()->has3DTransforms(m_owner) ||
                                  !m_rareData->m_matrix.rectStaysRect();

        if (!m_rareData->m_matrix.isIdentity()) {
            /*
            STARFISH_LOG_INFO("matrix [%f %f %f][%f %f %f][%f %f %f]",
                               m_rareData->m_matrix.getScaleX(),
                               m_rareData->m_matrix.getSkewX(),
                               m_rareData->m_matrix.getTranslateX(),
                               m_rareData->m_matrix.getSkewY(),
                               m_rareData->m_matrix.getScaleY(),
                               m_rareData->m_matrix.getTranslateY(),
                               m_rareData->m_matrix.getPerspX(),
                               m_rareData->m_matrix.getPerspY(),
                               m_rareData->m_matrix.get(8));*/
            SkMatrix test;
            bool testResult = m_rareData->m_matrix.invert(&test);
            if (testResult) {
                for (size_t i = 0; i < 9; i++) {
                    // prevent applying too big matrix
                    // because cairo can't deal well with huge matrix
                    if (m_rareData->m_matrix.get(i) >
                        STARFISH_CANVAS_LENGTH_MAX) {
                        testResult = false;
                        break;
                    }
                }
            }
            if (!testResult) {
                m_rareData->m_matrix = SkMatrix::InvalidMatrix();
            }
        }
    } else {
        if (m_rareData) {
            m_rareData->m_matrix = SkMatrix::I();
        }
    }
}

static void extractMaxScaleFactorFromAnimation(AnimatedValue* v,
                                               float& transformScaleMaxValue)
{
    if (v->isTransformData()) {
        auto transformData = v->getTransformData();
        for (size_t i = 0; i < transformData->size(); i++) {
            if (transformData->at(i).type() == StyleTransformData::Scale) {
                transformScaleMaxValue =
                    std::max(transformScaleMaxValue,
                             (float)transformData->at(i).scale()->x());
                transformScaleMaxValue =
                    std::max(transformScaleMaxValue,
                             (float)transformData->at(i).scale()->y());
            }
        }
    } else {
        auto m = v->getMatrix();
        transformScaleMaxValue =
            std::max(transformScaleMaxValue, m.getScaleX());
        transformScaleMaxValue =
            std::max(transformScaleMaxValue, m.getScaleY());
    }
}

static void findAnimationTaskRelatedWithTransformScale(
    ActiveAnimationTask* task, float& transformScaleMaxValue)
{
    if (task->property() == CSSStyleValuePair::Transform) {
        const auto& v = task->values();
        for (size_t i = 0; i < v.size(); i++) {
            extractMaxScaleFactorFromAnimation(v.at(i), transformScaleMaxValue);
        }
    }
}

void StackingContext::computeStackingContextProperties()
{
    STARFISH_ASSERT(isRootContext());

    ComputeStackingContextContext ctx(this);
    try {
        computeStackingContextProperties(ctx);
    } catch (RecomputeStackContextReason e) {
        if (e == RecomputeStackContextReason::PositionFixed) {
            ctx = ComputeStackingContextContext(this);
            ctx.needsToAllocateGraphicsBufferForFixedElement = true;
            computeStackingContextProperties(ctx);
        }
    }
    applyStackingContextProperties(ctx);

    ApplyPropertiesPostProcessingContext postCtx;
    applyStackingContextPropertiesPostProcessing(postCtx);

    m_owner->node()
        ->window()
        ->webView()
        ->m_stackingContextsNeedsGraphicsBuffer =
        std::move(postCtx.stackingContextsNeedsGraphicsBuffer);
}

void StackingContext::computeStackingContextProperties(
    ComputeStackingContextContext& compositingState)
{
    computeTransformMatrix();

    m_hasFilterEffect = false;
    if ((owner()->style()->hasAvailableFilter() ||
         m_ancestorsThatHasFilters.size() != 0)) {
        m_hasFilterEffect = true;
    }

    if (m_owner->isRootElement()) {
        compositingState.documentOwners.push_back(this);
    }

    NeedsGraphicsLayerReason reason =
        NeedsGraphicsLayerReason::NeedsGraphicsLayerReasonNone;

    bool selfNeedsGraphicsBuffer = m_owner->needsGraphicsBuffer();

    if (m_owner->style()->position() == PositionValue::FixedPositionValue) {
        if (!compositingState.needsToAllocateGraphicsBufferForFixedElement &&
            compositingState.seenCompsitedLayer()) {
            throw RecomputeStackContextReason::PositionFixed;
        } else if (compositingState
                       .needsToAllocateGraphicsBufferForFixedElement) {
            selfNeedsGraphicsBuffer = true;
        } else {
            compositingState.seenPositionFixed = true;
        }
    }

    // check self visibility
    if (selfNeedsGraphicsBuffer && m_owner->isBoxesInvisibleFromHere()) {
        selfNeedsGraphicsBuffer = false;
    }
    if (selfNeedsGraphicsBuffer &&
        m_owner->style()->visibility() == HiddenVisibilityValue) {
        bool everyDesendentBoxIsHidden = true;
        m_owner->iterateChildFrameBox(
            [&everyDesendentBoxIsHidden](FrameBox* fb) {
                if (!fb->isAnonymous() &&
                    fb->style()->visibility() != HiddenVisibilityValue) {
                    everyDesendentBoxIsHidden = false;
                }
            });
        if (everyDesendentBoxIsHidden) {
            selfNeedsGraphicsBuffer = false;
        }
    }

    auto selfExtent = compositingState.screenExtentPerLayer(this);
    m_screenExtent = selfExtent;

    bool compositedBySelf =
        selfNeedsGraphicsBuffer ||
        m_owner->node()->window()->webView()->hasActiveAnimationExecutor();

#if !defined(STARFISH_ENABLE_TEST)
    if (m_owner->isRootElement()) {
        FrameBlockBox* fb = m_owner->asFrameBlockBox();
        if (isRootContext()) {
            fb = m_owner->node()
                     ->document()
                     ->frame()
                     ->asFrameBox()
                     ->asFrameBlockBox();
        }
        if ((fb->hasBiggerContentThanFrameWidth() ||
             fb->hasBiggerContentThanFrameHeight())) {
            compositedBySelf = true;
        }
    }
#endif

    if (compositingState.seenCompsitedLayer() && !isRootContext() &&
        m_owner->isAbsolutePositioned()) {
        SkMatrix windowMatrix = m_owner->computeMatrixOnWindow();
        auto windowRect = computeBoxExtent(
            LayoutRect(0, 0, m_owner->width(), m_owner->height()),
            windowMatrix);
        if (windowRect.maxX() < 0 || windowRect.maxY() < 0) {
            compositedBySelf = true;
        }
    }

    if (Compositor::supportsFilterEffect(1, 1) ==
            true /* test whatever compostior supports filter */
        && m_hasFilterEffect) {
        compositedBySelf = true;
    }

    if (compositedBySelf) {
        reason = NeedsGraphicsLayerReason::NeedsGraphicsLayerReasonBySelf;
        compositingState.compositeFlagInfoBecauseSelf[this] = true;
    } else {
        compositingState.compositeFlagInfoBecauseSelf[this] = false;
    }
    bool willBeComposited = compositedBySelf;

    if (!willBeComposited && compositingState.seenCompsitedLayer() &&
        compositingState.compositedDocuments.find(
            m_owner->node()->document()) !=
            compositingState.compositedDocuments.end()) {
        // find most nearest Composited ancestor index
        size_t ancestorIndex = 0;

        StackingContext* p = parent();
        StackingContext* compositedAncestor = nullptr;
        while (p) {
            if (compositingState.isCompsitedLayer(p, &ancestorIndex)) {
                compositedAncestor = p;
                break;
            }
            p = p->parent();
        }
        STARFISH_ASSERT(compositedAncestor);
        bool canConveredByParentCompositedLayer = false;
        bool isCollapsedWithSilbingLayer = false;
        bool parentIsRootElementLayer = false; // parent layer is html element

        auto parentExtent =
            compositingState.screenExtentPerLayer(compositedAncestor);

        if (compositedAncestor->owner()->isRootElement()) {
            parentIsRootElementLayer = true;
        }

        bool thereIsOverflowHiddenBetweenSelfAndCompositedAncestor = false;
        {
            Frame* f = m_owner;
            while (f != nullptr) {
                if (f->shouldApplyOverflow()) {
                    thereIsOverflowHiddenBetweenSelfAndCompositedAncestor =
                        true;
                }
                if (f == compositedAncestor->owner()) {
                    break;
                }
                f = f->layoutParent();
            }
        }

        if (parentIsRootElementLayer ||
            (parentExtent.containsInVisual(selfExtent.x(), selfExtent.y()) ==
                 true &&
             parentExtent.containsInVisual(selfExtent.maxX(), selfExtent.y()) ==
                 true &&
             parentExtent.containsInVisual(selfExtent.x(), selfExtent.maxY()) ==
                 true &&
             parentExtent.containsInVisual(selfExtent.maxX(),
                                           selfExtent.maxY())) ||
            thereIsOverflowHiddenBetweenSelfAndCompositedAncestor) {
            canConveredByParentCompositedLayer = true;
        } else {
            reason = NeedsGraphicsLayerReason::
                NeedsGraphicsLayerReasonNotCoveredByParent;
        }

        if (canConveredByParentCompositedLayer) {
            auto& cv = compositingState.compositedLayers;
            for (size_t i = ancestorIndex + 1; i < cv.size(); i++) {
                auto extent =
                    compositingState.clippedScreenExtentPerLayer(cv[i]);
                if (extent.intersects(selfExtent)) {
                    isCollapsedWithSilbingLayer = true;
                    reason = NeedsGraphicsLayerReason::
                        NeedsGraphicsLayerReasonCollapsedWithSiblingLayer;
                    break;
                }

                if (cv[i]->owner()->isRunningTransformAnimation()) {
                    // find never collapsed case by overflow: hidden;
                    Frame* f = cv[i]->owner();
                    bool foundOverflow = false;
                    LayoutRect clippedExtentRect;
                    while (f != nullptr) {
                        if (f->isAncestorOf(owner())) {
                            break;
                        }
                        if (f->shouldApplyOverflow()) {
                            foundOverflow = true;
                            clippedExtentRect =
                                compositingState.clippedScreenExtentPerLayer(
                                    cv[i]);
                            break;
                        }
                        f = f->layoutParent();
                    }

                    if (foundOverflow) {
                        if (clippedExtentRect.intersects(selfExtent)) {
                            isCollapsedWithSilbingLayer = true;
                            reason = NeedsGraphicsLayerReason::
                                NeedsGraphicsLayerReasonSiblingLayerNeedsAnimation;
                        }
                    } else {
                        isCollapsedWithSilbingLayer = true;
                        reason = NeedsGraphicsLayerReason::
                            NeedsGraphicsLayerReasonSiblingLayerNeedsAnimation;
                    }

                    if (isCollapsedWithSilbingLayer) {
                        break;
                    }
                }
            }
        }

        if (canConveredByParentCompositedLayer &&
            !isCollapsedWithSilbingLayer) {
        } else {
            willBeComposited = true;
        }
    }

    if (willBeComposited) {
        for (size_t i = 0; i < compositingState.documentOwners.size(); i++) {
            if ((compositingState.documentOwners[i]->isRootContext()) ||
                (compositingState.documentOwners[i]
                     ->owner()
                     ->asFrameBlockBox()
                     ->hasBiggerContentThanFrameWidth()) ||
                (compositingState.documentOwners[i]
                     ->owner()
                     ->asFrameBlockBox()
                     ->hasBiggerContentThanFrameHeight())) {
                if (compositingState.compositedLayers.size() == 0) {
                    STARFISH_ASSERT(
                        compositingState.documentOwners[i]->isRootContext());
                }
                if (!compositingState.isCompsitedLayer(
                        compositingState.documentOwners[i])) {
                    compositingState.pushCompsitedLayer(
                        compositingState.documentOwners[i]);
                }
            }
        }
        if (!compositingState.isCompsitedLayer(this)) {
            compositingState.pushCompsitedLayer(this);
        }
    }

    m_needsGraphicsBufferReason = reason;

    auto iter = m_childContexts.begin();
    while (iter != m_childContexts.end()) {
        StackingContextChild* child = *iter;
        int32_t num = child->at(0)->zIndex();
        if (num >= 0) {
            break;
        }
        auto iter2 = child->begin();
        while (iter2 != child->end()) {
            (*iter2)->computeStackingContextProperties(compositingState);
            iter2++;
        }
        iter++;
    }

    iter = m_childContexts.begin();
    while (iter != m_childContexts.end()) {
        StackingContextChild* child = *iter;
        int32_t num = child->at(0)->zIndex();
        if (num >= 0) {
            auto iter2 = child->begin();
            while (iter2 != child->end()) {
                (*iter2)->computeStackingContextProperties(compositingState);
                iter2++;
            }
        }
        iter++;
    }

    if (m_owner->isRootElement()) {
        if (compositingState.isCompsitedLayer(this)) {
            willBeComposited = true;
            m_needsGraphicsBufferReason = NeedsGraphicsLayerReason::
                NeedsGraphicsLayerReasonCollapsedWithSiblingLayer;
        }
    }

    compositingState.compositeFlagInfo.insert(
        std::make_pair(this, willBeComposited));
}

static void computeVisibleRectPedigreeWorker(
    StackingContext* c, std::vector<FrameBox*>& pedigree,
    std::vector<FrameBox*>::reverse_iterator iter,
    Frame::ComputeVisibleRectContext& ctx)
{
    if (pedigree.rend() == iter) {
        c->owner()->computeVisibleRect(ctx);
    } else {
        Frame::ComputeVisibleRectContextFragment f(ctx, *iter);
        computeVisibleRectPedigreeWorker(c, pedigree, iter + 1, ctx);
    }
}

static void computeVisibleRect(StackingContext* source, StackingContext* c,
                               Frame::ComputeVisibleRectContext& ctx)
{
    if (c != source && c->needsGraphicsBuffer()) {
        return;
    }

    if (c != source) {
        std::vector<FrameBox*> pedigree;
        FrameBox* box = c->owner()->layoutParent()->asFrameBox();
        while (source->owner() != box) {
            pedigree.push_back(box);
            box = box->layoutParent()->asFrameBox();
        }

        computeVisibleRectPedigreeWorker(c, pedigree, pedigree.rbegin(), ctx);
    } else {
        c->owner()->computeVisibleRect(ctx);
    }

    auto iter = c->childContexts().begin();
    while (iter != c->childContexts().end()) {
        StackingContextChild* child = *iter;
        auto iter2 = child->begin();
        while (iter2 != child->end()) {
            computeVisibleRect(source, (*iter2), ctx);
            iter2++;
        }
        iter++;
    }
}

void StackingContext::applyStackingContextProperties(
    ComputeStackingContextContext& ctx)
{
    auto iter = m_childContexts.begin();
    while (iter != m_childContexts.end()) {
        StackingContextChild* child = *iter;
        auto iter2 = child->rbegin();
        while (iter2 != child->rend()) {
            (*iter2)->applyStackingContextProperties(ctx);
            iter2++;
        }
        iter++;
    }

    bool inAnimation =
        m_owner->node()->window()->webView()->hasActiveAnimationExecutor();
    auto& prevDrawnMap =
        m_owner->node()->webView()->prevDrawnStackingContextInfo();
    bool compositedBefore = false;
    auto prevDrawnMapIter = prevDrawnMap.find(m_owner->node());
    if (prevDrawnMap.end() != prevDrawnMapIter) {
        compositedBefore = prevDrawnMapIter->second.needsGraphicsBuffer;
        if (compositedBefore) {
            ensureRareData()->m_graphicsBufferHolder =
                prevDrawnMapIter->second.graphicsBufferHolder;
        }
    }
    bool willBeComposited = ctx.compositeFlagInfo[this];
    bool willBeCompositedDueToSelf = ctx.compositeFlagInfoBecauseSelf[this];

    if (inAnimation && compositedBefore && !willBeComposited) {
        willBeComposited = true;
    }

    if (m_rareData) {
        m_rareData->m_visibleRect = LayoutRect(0, 0, 0, 0);
    }

    if (willBeComposited) {
        ensureRareData();
        bool shouldPaintWindowBackgroundImage = false;
        if (m_owner->isRootElement()) {
            BrowsingContext* bc =
                m_owner->node()->document()->browsingContext();
            HTMLElement* e = nullptr;
            if (bc->hasRootElementBackground()) {
                HTMLHtmlElement* root = bc->document()->rootElement();
                e = root;
            } else if (bc->hasBodyElementBackground()) {
                HTMLBodyElement* body = bc->document()->rootElement()->body();
                e = body;
            }

            if (e) {
                if (e->style()->backgroundLayerSize()) {
                    shouldPaintWindowBackgroundImage = true;
                }
            }
        }

        SkMatrix l = SkMatrix::I();
        Frame::ComputeVisibleRectContext ctx(
            willBeCompositedDueToSelf
                ? Frame::ComputeVisibleRectContext::GraphicsBufferBySelf
                : Frame::ComputeVisibleRectContext::GraphicsBufferByOtherLayer,
            this, l, m_rareData->m_visibleRect);

        if (shouldPaintWindowBackgroundImage) {
            ctx.isVisibleRectCollapsible = false;
        }

        computeVisibleRect(this, this, ctx);

        if (shouldPaintWindowBackgroundImage) {
            LayoutRect scrollRect(
                0, 0, m_owner->document()->window()->scrollWidth(false),
                m_owner->document()->window()->scrollHeight(false));
            m_rareData->m_visibleRect.unite(scrollRect);
            m_rareData->m_visibleRect.unite(
                LayoutRect(0, 0, m_owner->node()->window()->innerWidth(),
                           m_owner->node()->window()->innerHeight()));
        }

        if (m_owner->isRootElement()) {
            if (m_rareData->m_visibleRect.x() < 0) {
                if (m_rareData->m_visibleRect.width() +
                        m_rareData->m_visibleRect.x() >
                    0) {
                    m_rareData->m_visibleRect.setWidth(
                        m_rareData->m_visibleRect.width() +
                        m_rareData->m_visibleRect.x());
                    m_rareData->m_visibleRect.setX(0);
                } else {
                    m_rareData->m_visibleRect.setWidth(0);
                }
            }
            if (m_rareData->m_visibleRect.y() < 0) {
                if (m_rareData->m_visibleRect.height() +
                        m_rareData->m_visibleRect.y() >
                    0) {
                    m_rareData->m_visibleRect.setHeight(
                        m_rareData->m_visibleRect.height() +
                        m_rareData->m_visibleRect.y());
                    m_rareData->m_visibleRect.setY(0);
                } else {
                    m_rareData->m_visibleRect.setHeight(0);
                }
            }
        }

        if (m_rareData->m_visibleRect.width() == 0 &&
            m_rareData->m_visibleRect.height() == 0 &&
            !m_owner->isRootElement() && !inAnimation) {
            willBeComposited = false;
        }

        m_isVisibleRectComputedForNonGraphicsLayer = true;
    } else {
        m_isVisibleRectComputedForNonGraphicsLayer = false;
    }

    if (compositedBefore != willBeComposited) {
        m_owner->node()->webView()->markNeedsPaintingConsiderInRendering();
    } else if (compositedBefore && compositedBefore == willBeComposited) {
        if (prevDrawnMapIter != prevDrawnMap.end()) {
            if (prevDrawnMapIter->second.graphicsBufferVisibleRect !=
                visibleRect()) {
                // visible rect changed
                m_owner->node()
                    ->webView()
                    ->markNeedsPaintingConsiderInRendering();
            }
        }
    } else if (!compositedBefore && !willBeComposited) {
        if (prevDrawnMapIter != prevDrawnMap.end()) {
            if ((prevDrawnMapIter->second.opacity !=
                 m_owner->style()->opacity()) ||
                (prevDrawnMapIter->second.transformMatrix !=
                 transformMatrix())) {
                m_owner->node()
                    ->webView()
                    ->markNeedsPaintingConsiderInRendering();
            }
        } else {
            if (transformMatrix() != SkMatrix::I()) {
                m_owner->node()
                    ->webView()
                    ->markNeedsPaintingConsiderInRendering();
            }
        }
    }

    if (willBeComposited) {
        m_needsGraphicsBuffer = true;
    } else {
        m_needsGraphicsBuffer = false;
    }

    if (isRootContext() && willBeComposited) {
        m_owner->node()->webView()->markNeedsCompositeConsiderInRendering();
    }
}

void StackingContext::applyStackingContextPropertiesPostProcessing(
    ApplyPropertiesPostProcessingContext& ctx)
{
    float orgBaseAdditionalPixelRatio = ctx.baseAdditionalPixelRatio;

    if (needsComposite()) {
        ctx.stackingContextsNeedsGraphicsBuffer.push_back(this);
        if (!m_owner->hasOwnGraphicsBufferMethod()) {
            STARFISH_ASSERT(m_rareData);
            float oldAdditionalPixelRatio = m_rareData->m_additionalPixelRatio;
            m_rareData->m_additionalPixelRatio = ctx.baseAdditionalPixelRatio;

            const float minScale = 1;
            const float maxScale = 4;

            if (m_owner->node() &&
                m_owner->node()->isRunningTransformAnimation()) {
                float transformScaleMaxValue = 1;

                auto& transitions = m_owner->node()
                                        ->document()
                                        ->animationExecutor()
                                        ->activeTransitions();
                auto iter = transitions.begin();
                while (iter != transitions.end()) {
                    findAnimationTaskRelatedWithTransformScale(
                        *iter, transformScaleMaxValue);
                    iter++;
                }

                auto& animations = m_owner->node()
                                       ->document()
                                       ->animationExecutor()
                                       ->activeAnimations();
                auto iter2 = animations.begin();
                while (iter2 != animations.end()) {
                    if (iter2->first->m_element == m_owner->node()) {
                        auto& v = iter2->second;
                        auto iter3 = v.begin();
                        while (iter3 != v.end()) {
                            findAnimationTaskRelatedWithTransformScale(
                                *iter3, transformScaleMaxValue);
                            iter3++;
                        }
                    }
                    iter2++;
                }

                if (transformScaleMaxValue < minScale) {
                    transformScaleMaxValue = minScale;
                } else if (transformScaleMaxValue > maxScale) {
                    transformScaleMaxValue = maxScale;
                }
                m_rareData->m_additionalPixelRatio = std::max(
                    ctx.baseAdditionalPixelRatio, transformScaleMaxValue);
                ctx.baseAdditionalPixelRatio =
                    std::max(ctx.baseAdditionalPixelRatio,
                             m_rareData->m_additionalPixelRatio);
            } else {
                auto matrix = m_owner->style()->transformsToMatrix(
                    m_owner->width(), m_owner->height(), m_owner,
                    m_owner->isTransformable());

                int32_t windowWidth = m_owner->node()->window()->innerWidth();
                int32_t windowHeight = m_owner->node()->window()->innerHeight();
                LayoutUnit visibleWidth = m_rareData->m_visibleRect.width();
                LayoutUnit visibleHeight = m_rareData->m_visibleRect.height();
                // additional
#ifndef STARFISH_GRAPHICS_BUFFER_ADDITIONAL_FACTOR_MAX_SCALE
#define STARFISH_GRAPHICS_BUFFER_ADDITIONAL_FACTOR_MAX_SCALE 6
#endif
                const int32_t minimumScale =
                    STARFISH_GRAPHICS_BUFFER_ADDITIONAL_FACTOR_MAX_SCALE;

                if ((!m_rareData->m_visibleRect.isEmpty() &&
                     !m_owner->node()->window()->isInnerSizeEmpty()) &&
                    ((visibleWidth > windowWidth * minimumScale) ||
                     (visibleHeight > windowHeight * minimumScale))) {
                    int m = std::max(visibleWidth / windowWidth,
                                     visibleHeight / windowHeight);
                    float scale =
                        std::min(matrix.getScaleX(), matrix.getScaleY());
                    scale = std::min(1.f / m, scale);
                    // respect org scale
                    m_rareData->m_additionalPixelRatio =
                        std::min(ctx.baseAdditionalPixelRatio, scale);
                    ctx.baseAdditionalPixelRatio =
                        std::min(ctx.baseAdditionalPixelRatio,
                                 m_rareData->m_additionalPixelRatio);
                } else {
                    float scale =
                        std::max(matrix.getScaleX(), matrix.getScaleY());
                    if (scale < minScale) {
                        scale = minScale;
                    } else if (scale > maxScale) {
                        scale = maxScale;
                    }

                    m_rareData->m_additionalPixelRatio =
                        std::max(ctx.baseAdditionalPixelRatio, scale);
                    ctx.baseAdditionalPixelRatio =
                        std::max(ctx.baseAdditionalPixelRatio,
                                 m_rareData->m_additionalPixelRatio);
                }
            }

            if (oldAdditionalPixelRatio != m_rareData->m_additionalPixelRatio) {
                m_owner->node()
                    ->webView()
                    ->markNeedsPaintingConsiderInRendering();
            }
        }
    } else {
        if (m_rareData) {
            m_rareData->m_additionalPixelRatio = 1;
        }
    }

    auto iter = childContexts().begin();
    while (iter != childContexts().end()) {
        StackingContextChild* child = *iter;
        auto iter2 = child->begin();
        while (iter2 != child->end()) {
            (*iter2)->applyStackingContextPropertiesPostProcessing(ctx);
            iter2++;
        }
        iter++;
    }

    ctx.baseAdditionalPixelRatio = orgBaseAdditionalPixelRatio;
}

class FilterContext : public gc {
public:
    FilterContext(Canvas** origin, StackingContext* owner,
                  StackingContext::PaintingStackingContextContext& ctx)
        : m_origin(origin)
        , m_originCanvas(*origin)
        , m_ownerStackingContext(owner)
        , m_maxRadiusOffset(0.0f)
        , m_nativeImageToApplyFilter(nullptr)
        , m_canvasToApplyFilter(nullptr)
    {
        auto style = m_ownerStackingContext->owner()->style();

        Length standardDeviation;
        if (style->hasAvailableFilter() &&
            style->filter()->getStandardDeviationOfBlurFilter(
                standardDeviation)) {
            m_maxRadiusOffset = std::max(
                m_maxRadiusOffset, (standardDeviation.numberData() * 2 * 1.8f));
        }

        for (auto ancestor :
             m_ownerStackingContext->ancestorsThatHasFilters()) {
            auto s = ancestor->owner()->style();
            if (s->filter()->getStandardDeviationOfBlurFilter(
                    standardDeviation)) {
                m_maxRadiusOffset =
                    std::max(m_maxRadiusOffset,
                             (standardDeviation.numberData() * 2 * 1.8f));
            }
        }
        m_maxRadiusOffset =
            std::min(ShadowBlur::RADIUS_LIMIT, m_maxRadiusOffset);

        if (!m_ownerStackingContext->needsGraphicsBuffer()) {
            LayoutRect rect = m_ownerStackingContext->screenExtent();
            LayoutUnit minX = rect.x();
            LayoutUnit maxX = rect.maxX();
            LayoutUnit minY = rect.y();
            LayoutUnit maxY = rect.maxY();

            minX = minX.floor();
            maxX = maxX.ceil();
            minY = minY.floor();
            maxY = maxY.ceil();

            size_t bufferWidth;
            size_t bufferHeight;
            computeBufferSizeFromVisibleRect(minX, minY, maxX, maxY,
                                             bufferWidth, bufferHeight);

            if (owner->owner()->node()->webView()->needsComposite()) {
                auto& renderTarget = m_originCanvas->renderTargetInfo();
                bufferWidth = renderTarget.m_width;
                bufferHeight = renderTarget.m_height;
            }

            m_nativeImageToApplyFilter = BufferedNativeImageData::create(
                bufferWidth + ceil(m_maxRadiusOffset),
                bufferHeight + ceil(m_maxRadiusOffset));

            m_canvasToApplyFilter = Canvas::create(
                m_ownerStackingContext->owner()->node()->webView(),
                m_nativeImageToApplyFilter);

            m_canvasToApplyFilter->clearColor(Unit::Color(0, 0, 0, 0));

            m_canvasToApplyFilter->setFont(style->font());
            m_canvasToApplyFilter->setFillColor(style->color());
            m_canvasToApplyFilter->setTextDecorationData(
                m_originCanvas->textDecorationData());

            if (owner->owner()->node()->webView()->needsComposite()) {
                m_canvasToApplyFilter->postMatrix(
                    m_originCanvas->currentTransformMatrix());
            }

            m_canvasToApplyFilter->translate(ceil(m_maxRadiusOffset / 2),
                                             ceil(m_maxRadiusOffset / 2));

            (*m_origin) = m_canvasToApplyFilter;
        }
    }

    ~FilterContext()
    {
    }

    void changeCurrentCanvasToOriginal()
    {
        (*m_origin) = m_originCanvas;
    }

    void changeCurrentCanvasToApplyFilter()
    {
        if (m_canvasToApplyFilter) {
            (*m_origin) = m_canvasToApplyFilter;
        }
    }

    void applyAllFilter(StackingContext::PaintingStackingContextContext& ctx)
    {
        auto webView = m_ownerStackingContext->owner()->node()->webView();

        uint8_t* buffer;
        size_t width, height, stride;

        if (m_ownerStackingContext->needsGraphicsBuffer()) {
            const auto& info = m_originCanvas->renderTargetInfo();
            buffer = info.m_buffer;
            width = info.m_width;
            height = info.m_height;
            stride = info.m_stride;
        } else {
            buffer = m_nativeImageToApplyFilter->data();
            width = m_nativeImageToApplyFilter->width();
            height = m_nativeImageToApplyFilter->height();
            stride = m_nativeImageToApplyFilter->stride();
        }

        size_t imageStartYPosition = height;
        if (!m_ownerStackingContext->needsGraphicsBuffer()) {
            delete m_canvasToApplyFilter;

            LongTaskFinder t("testing painting result in FilterContext", 1);
            uint32_t* ptr = (uint32_t*)m_nativeImageToApplyFilter->data();
            size_t len = stride * height / 4;
            for (size_t i = 0; i < len; i++) {
                if (ptr[i]) {
                    imageStartYPosition = i * 4 / stride;
                    break;
                }
            }

            if (imageStartYPosition != height) {
                // give a room for blurred image
                auto offset = ceil(m_maxRadiusOffset / 2);
                if ((int)imageStartYPosition - offset > 0) {
                    imageStartYPosition -= offset;
                } else {
                    imageStartYPosition = 0;
                }
            }
        } else {
            // TODO
            imageStartYPosition = 0;
        }

        if (imageStartYPosition != height) {
            auto style = m_ownerStackingContext->owner()->style();
            if (style->hasAvailableFilter()) {
                for (auto filter : *style->filter()) {
                    filter->apply(webView,
                                  buffer + imageStartYPosition * stride, width,
                                  height - imageStartYPosition, stride);
                }
            }

            for (auto ancestor :
                 m_ownerStackingContext->ancestorsThatHasFilters()) {
                style = ancestor->owner()->style();
                for (auto filter : *style->filter()) {
                    filter->apply(webView,
                                  buffer + imageStartYPosition * stride, width,
                                  height - imageStartYPosition, stride);
                }
            }
        }

        if (!m_ownerStackingContext->needsGraphicsBuffer()) {
            if (imageStartYPosition != height) {
                Unit::Rect rect(0, 0, m_nativeImageToApplyFilter->width(),
                                m_nativeImageToApplyFilter->height());
                float offset = ceil(m_maxRadiusOffset / 2);
                m_originCanvas->save();
                if (m_ownerStackingContext->owner()
                        ->node()
                        ->webView()
                        ->needsComposite()) {
                    m_originCanvas->setMatrix(SkMatrix::I());
                }
                m_originCanvas->translate(-offset, -offset);
                m_originCanvas->drawImage(m_nativeImageToApplyFilter, rect);
                m_originCanvas->restore();
            }
            delete m_nativeImageToApplyFilter;
            (*m_origin) = m_originCanvas;
        }
    }

private:
    Canvas** m_origin;
    Canvas* m_originCanvas;
    StackingContext* m_ownerStackingContext;
    float m_maxRadiusOffset;

    NativeImageData* m_nativeImageToApplyFilter;
    Canvas* m_canvasToApplyFilter;
};

void StackingContext::fillGraphicsBufferContents(
    Canvas* canvas, PaintingStackingContextContext& ctx)
{
    canvas->save();

    if (isRootContext()) {
        m_owner->node()->document()->browsingContext()->paintWindowBackground(
            canvas);
    }

    if (owner()->shouldResetTextDecoration()) {
        canvas->resetTextDecorationData();
    } else {
        canvas->mergeTextDecorationData(owner()->style());
    }

    if (owner()->isAbsolutePositioned()) {
        RectData* rect = owner()->style()->clip();
        if (rect) {
            canvas->clip(Unit::Rect(
                rect->left().numberData(), rect->top().numberData(),
                rect->right().numberData(), rect->bottom().numberData()));
        }
    }

    bool canRejectPainting =
        canvas->canRejectPainting(StackingContext::visibleRect());

    if (!canRejectPainting) {
        applyMask(canvas, ctx);
        m_owner->paintBackgroundAndBorders(canvas);
    }

    bool needsComputeScroll = !isRootContext() && !isIFrameStackingContext() &&
                              m_owner->isFrameBlockBox();
    if (needsComputeScroll) {
        canvas->save();
        canvas->translate(-m_owner->asFrameBlockBox()->scrollLeft(),
                          -m_owner->asFrameBlockBox()->scrollTop());
        ctx.layerScrollX = m_owner->asFrameBlockBox()->scrollLeft();
        ctx.layerScrollY = m_owner->asFrameBlockBox()->scrollTop();
    }

    // Within each stacking context, the following layers are painted in
    // back-to-front order:
    // the background and borders of the element forming the stacking context.
    if (isIFrameStackingContext()) {
        FrameBlockBox* document = m_owner->layoutParent()->asFrameBlockBox();
        FrameBox* iframeBox = m_owner->node()
                                  ->document()
                                  ->browsingContext()
                                  ->sourceElement()
                                  ->frame()
                                  ->asFrameBox();
        auto clipRect = iframeBox->makeRect(BoxValue::PaddingBoxBoxValue);
        HTMLIFrameElement* iframe =
            m_owner->node()->document()->browsingContext()->sourceElement();
        clipRect.setX(clipRect.x() + m_owner->node()
                                         ->document()
                                         ->browsingContext()
                                         ->window()
                                         ->scrollX());
        clipRect.setY(clipRect.y() + m_owner->node()
                                         ->document()
                                         ->browsingContext()
                                         ->window()
                                         ->scrollY());

        if (!needsGraphicsBuffer()) {
            canvas->translate(iframeBox->borderLeft() +
                                  iframeBox->paddingLeft(),
                              iframeBox->borderTop() + iframeBox->paddingTop());
        }

        if (needsGraphicsBuffer()) {
            canvas->save();
            canvas->resetMatrixAndClip();
            m_owner->node()
                ->document()
                ->browsingContext()
                ->paintWindowBackground(canvas);
            canvas->restore();
        } else {
            m_owner->node()
                ->document()
                ->browsingContext()
                ->paintWindowBackground(canvas);
        }
    }

    // the child stacking contexts with negative stack levels (most negative
    // first).
    {
        auto iter = childContexts().begin();
        while (iter != childContexts().end()) {
            StackingContextChild* child = *iter;
            int32_t num = child->at(0)->zIndex();
            if (num >= 0) {
                break;
            }
            auto iter2 = child->begin();
            while (iter2 != child->end()) {
                StackingContext* sCtx = *iter2;
                CanvasStateRestorer r(canvas, sCtx, m_owner, ctx);
                sCtx->paintStackingContext(canvas, ctx);
                iter2++;
            }
            iter++;
        }
    }

    if (!canRejectPainting) {
        if (m_hasFilterEffect && !Compositor::supportsFilterEffect(
                                     canvas->renderTargetInfo().m_width,
                                     canvas->renderTargetInfo().m_height)) {
            FilterContext filterContext(&canvas, this, ctx);
            m_owner->paintStackingContextContent(canvas);
            m_owner->paintOutline(canvas);
            filterContext.applyAllFilter(ctx);
        } else {
            m_owner->paintStackingContextContent(canvas);
            m_owner->paintOutline(canvas);
        }
    }

    // the child stacking contexts with positive stack levels (least positive
    // first).
    {
        auto iter = childContexts().begin();
        while (iter != childContexts().end()) {
            StackingContextChild* child = *iter;
            int32_t num = child->at(0)->zIndex();
            if (num >= 0) {
                auto iter2 = child->begin();
                while (iter2 != child->end()) {
                    StackingContext* sCtx = *iter2;

                    CanvasStateRestorer r(canvas, sCtx, m_owner, ctx);
                    sCtx->paintStackingContext(canvas, ctx);
                    iter2++;
                }
            }
            iter++;
        }
    }

    if (needsComputeScroll) {
        canvas->restore();
    }

    paintScrollbar(canvas);

    canvas->restore();
}

LayoutRect StackingContext::visibleRect()
{
    if (!m_isVisibleRectComputedForNonGraphicsLayer) {
        ensureRareData()->m_visibleRect = m_owner->frameVisibleRect();
        SkMatrix l = SkMatrix::I();
        Frame::ComputeVisibleRectContext ctx(
            Frame::ComputeVisibleRectContext::GraphicsBufferBySelf, this, l,
            m_rareData->m_visibleRect);
        computeVisibleRect(this, this, ctx);
        m_isVisibleRectComputedForNonGraphicsLayer = true;
    }
    return m_rareData ? m_rareData->m_visibleRect : LayoutRect(0, 0, 0, 0);
}

float StackingContext::additionalPixelRatio()
{
    return m_rareData ? m_rareData->m_additionalPixelRatio : 1;
}

static LayoutRect computeScreenRect(StackingContext* ctx)
{
    LayoutRect screenRect(0, 0,
                          ctx->owner()
                              ->node()
                              ->webView()
                              ->mainBrowsingContext()
                              ->window()
                              ->innerWidth(),
                          ctx->owner()
                              ->node()
                              ->webView()
                              ->mainBrowsingContext()
                              ->window()
                              ->innerHeight());
    LayoutRect windowRect = screenRect;

    if (!ctx->owner()
             ->node()
             ->document()
             ->browsingContext()
             ->isTopLevelBrowsingContext()) {
        FrameBox* f = ctx->owner();

        while (f) {
            if (f->isFrameReplaced() &&
                f->asFrameReplaced()->isFrameReplacedIFrame()) {
                break;
            }
            f = f->layoutParent()->asFrameBox();
        }

        if (f) {
            windowRect = f->computeScreenExtent();
        }
    }

    return windowRect;
}

static LayoutRect computeWindowRectOnScreen(StackingContext* ctx)
{
    LayoutRect windowRect = computeScreenRect(ctx);

    if (!ctx->owner()
             ->node()
             ->document()
             ->browsingContext()
             ->isTopLevelBrowsingContext()) {
        FrameBox* f = ctx->owner();

        while (f) {
            if (f->isFrameReplaced() &&
                f->asFrameReplaced()->isFrameReplacedIFrame()) {
                break;
            }
            f = f->layoutParent()->asFrameBox();
        }

        if (f) {
            windowRect = f->computeScreenExtent();
        }
    }
    return windowRect;
}

bool canSkipFillGraphicsBufferDueToOpacityIsZero(
    StackingContext* stackingContext)
{
    STARFISH_ASSERT(stackingContext != nullptr);

    if (stackingContext->needsGraphicsBuffer() &&
        stackingContext->owner()->style()->opacity() == 0 &&
        !stackingContext->owner()->isRunningOpacityAnimation()) {
        return true;
    }

    return false;
}

bool StackingContext::fillGraphicsBufferContentsWithoutClipRect()
{
    if (needsGraphicsBuffer()) {
        if (canSkipFillGraphicsBufferDueToOpacityIsZero(this)) {
            return false;
        }

        LayoutRect visibleRect = StackingContext::visibleRect();
        LayoutUnit minX = visibleRect.x();
        LayoutUnit maxX = visibleRect.maxX();
        LayoutUnit minY = visibleRect.y();
        LayoutUnit maxY = visibleRect.maxY();
        size_t bufferWidth;
        size_t bufferHeight;
        computeBufferSizeFromVisibleRect(minX, minY, maxX, maxY, bufferWidth,
                                         bufferHeight);

        if (owner()->hasOwnGraphicsBufferMethod()) {
            CanvasSurface* s = nullptr;
            owner()->createGraphicsBuffer(&s, bufferWidth, bufferHeight);
            if (s) {
                ensureRareData()->m_graphicsBufferHolder =
                    new GraphicsBufferHolder(s);
            }
        } else {
            if (bufferWidth && bufferHeight) {
                ensureRareData();
                if (!m_rareData->m_graphicsBufferHolder) {
                    m_rareData->m_graphicsBufferHolder =
                        new GraphicsBufferHolder(
                            bufferWidth, bufferHeight,
                            m_owner->node()->window()->innerWidth(),
                            m_owner->node()->window()->innerHeight(), this);
                }

                size_t wTileSize =
                    m_rareData->m_graphicsBufferHolder->m_tileDataWidth;
                size_t hTileSize =
                    m_rareData->m_graphicsBufferHolder->m_tileDataHeight;
                size_t wTextureCount =
                    m_rareData->m_graphicsBufferHolder->m_horizontalTileCount;
                size_t hTextureCount =
                    m_rareData->m_graphicsBufferHolder->m_verticalTileCount;

                size_t tileIndex = 0;
                size_t coveredRowsCount = 0;

                LayoutRect screenRect = computeScreenRect(this);
                LayoutRect windowRect = computeWindowRectOnScreen(this);
                auto screenMatrix = m_owner->computeScreenMatrix();

                size_t hVisibleTextureStart = hTextureCount;
                size_t hVisibleTextureEnd = 0;
                size_t wVisibleTextureStart = wTextureCount;
                size_t wVisibleTextureEnd = 0;

                for (size_t y = 0; y < hTextureCount; y++) {
                    size_t coveredColsCount = 0;
                    for (size_t x = 0; x < wTextureCount; x++) {
                        size_t tileDataX = coveredColsCount;
                        size_t tileDataY = coveredRowsCount;
                        size_t tileDataWidth = std::min(
                            wTileSize,
                            m_rareData->m_graphicsBufferHolder->bufferWidth() -
                                coveredColsCount);
                        size_t tileDataHeight = std::min(
                            hTileSize,
                            m_rareData->m_graphicsBufferHolder->bufferHeight() -
                                coveredRowsCount);

                        LayoutRect tileExtent = computeBoxExtent(
                            LayoutRect(minX + (LayoutUnit)tileDataX,
                                       minY + (LayoutUnit)tileDataY,
                                       tileDataWidth, tileDataHeight),
                            screenMatrix);

                        bool willPaintOnScreen =
                            screenRect.intersects(tileExtent) &&
                            windowRect.intersects(tileExtent);

                        if (willPaintOnScreen) {
                            wVisibleTextureStart =
                                std::min(wVisibleTextureStart, x);
                            wVisibleTextureEnd =
                                std::max(wVisibleTextureEnd, x + 1);
                            hVisibleTextureStart =
                                std::min(hVisibleTextureStart, y);
                            hVisibleTextureEnd =
                                std::max(hVisibleTextureEnd, y + 1);
                        }

                        coveredColsCount += wTileSize;
                    }
                    coveredRowsCount += hTileSize;
                }

                Scrolling* scrolling = nullptr;
                if (m_owner->isRootElement()) {
                    scrolling = m_owner->node()->window()->scrolling();
                } else {
                    if (m_owner->node()->isElement() &&
                        m_owner->node()->asElement()->rareMembers()) {
                        scrolling = m_owner->node()
                                        ->asElement()
                                        ->rareMembers()
                                        ->asRareElementMembers()
                                        ->m_scrolling;
                    }
                }

                size_t hEarlyPaintingTextureStart = hVisibleTextureStart;
                size_t hEarlyPaintingTextureEnd = hVisibleTextureEnd;
                size_t wEarlyPaintingTextureStart = wVisibleTextureStart;
                size_t wEarlyPaintingTextureEnd = wVisibleTextureEnd;

                if (scrolling) {
                    if (scrolling->inVerticalScrollingDown()) {
                        hEarlyPaintingTextureEnd = hVisibleTextureEnd + 1;
                    }
                    if (hVisibleTextureStart != 0 &&
                        scrolling->inVerticalScrollingUp()) {
                        hEarlyPaintingTextureStart = hVisibleTextureStart - 1;
                    }

                    if (scrolling->inHorizontalScrollingRight()) {
                        wEarlyPaintingTextureEnd = wVisibleTextureEnd + 1;
                    }
                    if (wVisibleTextureStart != 0 &&
                        scrolling->inHorizontalScrollingLeft()) {
                        wEarlyPaintingTextureStart = wVisibleTextureStart - 1;
                    }
                }

                if (m_owner->node()->isElement() &&
                    m_owner->node()->isRunningTransformAnimation()) {
                    struct Data {
                        Element* element;
                        size_t* hEarlyPaintingTextureStart;
                        size_t* hEarlyPaintingTextureEnd;
                        size_t* wEarlyPaintingTextureStart;
                        size_t* wEarlyPaintingTextureEnd;
                    } d;
                    d.element = m_owner->node()->asElement();
                    d.hEarlyPaintingTextureStart = &hEarlyPaintingTextureStart;
                    d.hEarlyPaintingTextureEnd = &hEarlyPaintingTextureEnd;
                    d.wEarlyPaintingTextureStart = &wEarlyPaintingTextureStart;
                    d.wEarlyPaintingTextureEnd = &wEarlyPaintingTextureEnd;

                    m_owner->node()
                        ->document()
                        ->animationExecutor()
                        ->iterateAnimationTasks(
                            [](ActiveAnimationTask* task, void* data) {
                                Data* d = (Data*)data;
                                if (task->targetElement() == d->element &&
                                    task->property() ==
                                        CSSStyleValuePair::KeyKind::Transform &&
                                    task->fraction(
                                        d->element->document()
                                            ->browsingContext()
                                            ->styleResolveStartTick())) {
                                    auto fromValue =
                                        ((ActiveTransformAnimationTask*)task)
                                            ->decomposedFrom();
                                    auto toValue =
                                        ((ActiveTransformAnimationTask*)task)
                                            ->decomposedTo();

                                    if (fromValue.translateX <
                                        toValue.translateX) {
                                        if (*d->wEarlyPaintingTextureStart !=
                                            0) {
                                            *d->wEarlyPaintingTextureStart =
                                                *d->wEarlyPaintingTextureStart -
                                                1;
                                        }
                                    }
                                    if (fromValue.translateX >
                                        toValue.translateX) {
                                        *d->wEarlyPaintingTextureEnd =
                                            *d->wEarlyPaintingTextureEnd + 1;
                                    }

                                    if (fromValue.translateY <
                                        toValue.translateY) {
                                        if (*d->hEarlyPaintingTextureStart !=
                                            0) {
                                            *d->hEarlyPaintingTextureStart =
                                                *d->hEarlyPaintingTextureStart -
                                                1;
                                        }
                                    }
                                    if (fromValue.translateY >
                                        toValue.translateY) {
                                        *d->hEarlyPaintingTextureEnd =
                                            *d->hEarlyPaintingTextureEnd + 1;
                                    }
                                }
                            },
                            &d);
                }

                tileIndex = 0;
                coveredRowsCount = 0;

                for (size_t y = 0; y < hTextureCount; y++) {
                    size_t coveredColsCount = 0;
                    for (size_t x = 0; x < wTextureCount; x++) {
                        size_t tileDataX = coveredColsCount;
                        size_t tileDataY = coveredRowsCount;
                        size_t tileDataWidth = std::min(
                            wTileSize,
                            m_rareData->m_graphicsBufferHolder->bufferWidth() -
                                coveredColsCount);
                        size_t tileDataHeight = std::min(
                            hTileSize,
                            m_rareData->m_graphicsBufferHolder->bufferHeight() -
                                coveredRowsCount);

                        LayoutRect tileExtent = computeBoxExtent(
                            LayoutRect(minX + (LayoutUnit)tileDataX,
                                       minY + (LayoutUnit)tileDataY,
                                       tileDataWidth, tileDataHeight),
                            screenMatrix);

                        bool isVisible = hVisibleTextureStart <= y &&
                                         y < hVisibleTextureEnd &&
                                         wVisibleTextureStart <= x &&
                                         x < wVisibleTextureEnd;
                        bool isEarlyPainting =
                            hEarlyPaintingTextureStart <= y &&
                            y < hEarlyPaintingTextureEnd &&
                            wEarlyPaintingTextureStart <= x &&
                            x < wEarlyPaintingTextureEnd;

                        if (isVisible || isEarlyPainting) {
                            if (m_rareData->m_graphicsBufferHolder
                                    ->m_surfaces[tileIndex] == nullptr) {
                                if (m_owner->document()
                                        ->webView()
                                        ->didFirstRenderingAfterWakeup() &&
                                    !isVisible && isEarlyPainting) {
                                    auto tick = longTickCount();
                                    if (tick - m_owner->node()
                                                   ->webView()
                                                   ->lastRenderingTick() >
                                        (uint64_t)WebView::
                                                g_fillingGraphicsBufferTileFrameTimeLimitInMS *
                                            1000) {
                                        tileIndex++;
                                        continue;
                                    }
                                }

                                CanvasSurface* canvasSurface =
                                    CanvasSurface::create(
                                        m_owner->document()
                                            ->webView()
                                            ->platformWindow(),
                                        tileDataWidth, tileDataHeight,
                                        additionalPixelRatio(),
                                        m_hasFilterEffect
                                            ? CanvasSurface::
                                                  ElementHasFilterEffect
                                            : CanvasSurface::PlainElement);
                                Canvas* canvas = Canvas::create(
                                    m_owner->node()->webView(), canvasSurface);

                                canvas->setTextDecorationData(
                                    m_rareData->m_textDecorationData);

                                // give empty repaint region
                                // this stage. we will just filling empty tiles
                                // if
                                // needed
                                RepaintRegion rr;
                                StackingContext::PaintingStackingContextContext
                                    ctx(true,
                                        m_owner->node()
                                            ->webView()
                                            ->m_prevDrawnStackingContextInfo,
                                        LayoutRect(0, 0, 0, 0), rr, 0, 0);

                                ctx.layerBaseX = tileDataX;
                                ctx.layerBaseY = tileDataY;

                                ctx.layerClipRect =
                                    LayoutRect(0, 0, canvasSurface->width(),
                                               canvasSurface->height());

                                canvas->translate(-minX, -minY);
                                canvas->translate(-ctx.layerBaseX,
                                                  -ctx.layerBaseY);

                                fillGraphicsBufferContents(canvas, ctx);

                                delete canvas;

                                canvasSurface
                                    ->unmapBufferAndNotifyUpdatedRegion(
                                        0, 0, canvasSurface->bufferWidth(),
                                        canvasSurface->bufferHeight());

                                m_rareData->m_graphicsBufferHolder
                                    ->m_surfaces[tileIndex] = canvasSurface;
                            }
                        } else {
                            if (m_rareData->m_graphicsBufferHolder
                                    ->m_surfaces[tileIndex]) {
                                m_rareData->m_graphicsBufferHolder
                                    ->m_surfaces[tileIndex]
                                    ->detachNativeBuffer();
                                m_rareData->m_graphicsBufferHolder
                                    ->m_surfaces[tileIndex] = nullptr;
                            }
                        }

                        tileIndex++;
                        coveredColsCount += wTileSize;
                    }

                    coveredRowsCount += hTileSize;
                }

                STARFISH_ASSERT(tileIndex == wTextureCount * hTextureCount);
            }
        }
    }

    return false;
}

bool StackingContext::fillGraphicsBufferContents(
    PaintingStackingContextContext& globalCtx)
{
    if (!needsGraphicsBuffer()) {
        return false;
    }

    bool drawnSomething = false;
    LayoutRect visibleRect = StackingContext::visibleRect();
    LayoutUnit minX = visibleRect.x();
    LayoutUnit maxX = visibleRect.maxX();
    LayoutUnit minY = visibleRect.y();
    LayoutUnit maxY = visibleRect.maxY();

    size_t bufferWidth;
    size_t bufferHeight;
    computeBufferSizeFromVisibleRect(minX, minY, maxX, maxY, bufferWidth,
                                     bufferHeight);

    if (canSkipFillGraphicsBufferDueToOpacityIsZero(this)) {
        return false;
    }

    if (bufferWidth == 0 || bufferHeight == 0) {
        return false;
    }

    LayoutRect deviceLayerClipRect;

    if (m_rareData->m_graphicsBufferHolder == nullptr ||
        m_rareData->m_graphicsBufferHolder->bufferWidth() != bufferWidth ||
        m_rareData->m_graphicsBufferHolder->bufferHeight() != bufferHeight ||
        m_rareData->m_graphicsBufferHolder->additionalPixelRatio() !=
            additionalPixelRatio()) {
        if (m_owner->hasOwnGraphicsBufferMethod()) {
            CanvasSurface* s = nullptr;
            m_owner->createGraphicsBuffer(&s, bufferWidth, bufferHeight);
            if (s) {
                m_rareData->m_graphicsBufferHolder =
                    new GraphicsBufferHolder(s);
            }
        } else {
            bool reuse = false;
            auto iter =
                globalCtx.prevDrawnStackingContextInfoMap.find(m_owner->node());
            if (iter != globalCtx.prevDrawnStackingContextInfoMap.end()) {
                if (iter->second.graphicsBufferHolder &&
                    iter->second.graphicsBufferHolder->bufferWidth() ==
                        bufferWidth &&
                    iter->second.graphicsBufferHolder->bufferHeight() ==
                        bufferHeight &&
                    iter->second.graphicsBufferHolder->additionalPixelRatio() ==
                        additionalPixelRatio()) {
                    reuse = true;
                    m_rareData->m_graphicsBufferHolder =
                        iter->second.graphicsBufferHolder;
                    iter->second.graphicsBufferHolder = nullptr;
                } else {
                    if (iter->second.graphicsBufferHolder) {
                        iter->second.graphicsBufferHolder->flushSurfaces();
                        iter->second.graphicsBufferHolder = nullptr;
                    }
                }
            }

            if (!reuse) {
                m_rareData->m_graphicsBufferHolder = new GraphicsBufferHolder(
                    bufferWidth, bufferHeight,
                    m_owner->node()->window()->innerWidth(),
                    m_owner->node()->window()->innerHeight(), this);
            }
        }
    } else {
        auto iter =
            globalCtx.prevDrawnStackingContextInfoMap.find(m_owner->node());
        if (iter != globalCtx.prevDrawnStackingContextInfoMap.end()) {
            iter->second.graphicsBufferHolder = nullptr;
        }
    }

    if (m_owner->hasOwnGraphicsBufferMethod()) {
        auto iter =
            globalCtx.prevDrawnStackingContextInfoMap.find(m_owner->node());
        if (iter != globalCtx.prevDrawnStackingContextInfoMap.end()) {
            iter->second.graphicsBufferHolder = nullptr;
        }
        return false;
    }

    size_t wTileSize = m_rareData->m_graphicsBufferHolder->m_tileDataWidth;
    size_t hTileSize = m_rareData->m_graphicsBufferHolder->m_tileDataHeight;
    size_t wTextureCount =
        m_rareData->m_graphicsBufferHolder->m_horizontalTileCount;
    size_t hTextureCount =
        m_rareData->m_graphicsBufferHolder->m_verticalTileCount;

    auto screenMatrix = m_owner->computeScreenMatrix();

    LayoutRect screenRect = computeScreenRect(this);
    LayoutRect windowRect = computeWindowRectOnScreen(this);

    size_t tileIndex = 0;
    size_t coveredRowsCount = 0;
    for (size_t y = 0; y < hTextureCount; y++) {
        size_t coveredColsCount = 0;
        for (size_t x = 0; x < wTextureCount; x++) {
            size_t tileDataX = coveredColsCount;
            size_t tileDataY = coveredRowsCount;
            size_t tileDataWidth = std::min(
                wTileSize, m_rareData->m_graphicsBufferHolder->bufferWidth() -
                               coveredColsCount);
            size_t tileDataHeight = std::min(
                hTileSize, m_rareData->m_graphicsBufferHolder->bufferHeight() -
                               coveredRowsCount);

            LayoutRect tileExtent =
                computeBoxExtent(LayoutRect(minX + (LayoutUnit)tileDataX,
                                            minY + (LayoutUnit)tileDataY,
                                            tileDataWidth, tileDataHeight),
                                 screenMatrix);

            bool willPaintOnScreen = screenRect.intersects(tileExtent) &&
                                     windowRect.intersects(tileExtent);

            LayoutRect layerClipRect = globalCtx.repaintRegion[owner()->node()];
            bool isOverlappedWithScreenClipRect =
                layerClipRect.intersects(LayoutRect(
                    (LayoutUnit)tileDataX + minX, (LayoutUnit)tileDataY + minY,
                    tileDataWidth, tileDataHeight));
            layerClipRect.setX(layerClipRect.x() - (LayoutUnit)tileDataX -
                               minX);
            layerClipRect.setY(layerClipRect.y() - (LayoutUnit)tileDataY -
                               minY);

            if (isOverlappedWithScreenClipRect && !willPaintOnScreen) {
                if (m_rareData->m_graphicsBufferHolder->m_surfaces[tileIndex]) {
                    m_rareData->m_graphicsBufferHolder->m_surfaces[tileIndex]
                        ->detachNativeBuffer();
                    m_rareData->m_graphicsBufferHolder->m_surfaces[tileIndex] =
                        nullptr;
                }
            } else if (willPaintOnScreen) {
                bool gotNewBuffer = false;
                if (!m_rareData->m_graphicsBufferHolder
                         ->m_surfaces[tileIndex]) {
                    m_rareData->m_graphicsBufferHolder->m_surfaces[tileIndex] =
                        CanvasSurface::create(
                            m_owner->document()->webView()->platformWindow(),
                            tileDataWidth, tileDataHeight,
                            additionalPixelRatio(),
                            m_hasFilterEffect
                                ? CanvasSurface::ElementHasFilterEffect
                                : CanvasSurface::PlainElement);
                    gotNewBuffer = true;
                }

                if (isOverlappedWithScreenClipRect || gotNewBuffer) {
                    CanvasSurface* canvasSurface =
                        m_rareData->m_graphicsBufferHolder
                            ->m_surfaces[tileIndex];
                    Canvas* canvas = Canvas::create(m_owner->node()->webView(),
                                                    canvasSurface);

                    canvas->setTextDecorationData(
                        m_rareData->m_textDecorationData);

                    StackingContext::PaintingStackingContextContext ctx(
                        true, globalCtx.prevDrawnStackingContextInfoMap,
                        globalCtx.screenClipRect, globalCtx.repaintRegion,
                        globalCtx.scrollX, globalCtx.scrollY);

                    ctx.layerBaseX = tileDataX;
                    ctx.layerBaseY = tileDataY;

                    bool needsInitialClip = false;
                    ctx.layerClipRect = LayoutRect(0, 0, canvasSurface->width(),
                                                   canvasSurface->height());

                    if (isOverlappedWithScreenClipRect && !gotNewBuffer) {
                        ctx.layerClipRect = layerClipRect;
                        needsInitialClip = true;

                        if (ctx.layerClipRect.x() <= 0 &&
                            ctx.layerClipRect.y() <= 0 &&
                            ctx.layerClipRect.maxX() >=
                                (int)canvasSurface->width() &&
                            ctx.layerClipRect.maxY() >=
                                (int)canvasSurface->height()) {
                            needsInitialClip = false;
                            ctx.layerClipRect =
                                LayoutRect(0, 0, canvasSurface->width(),
                                           canvasSurface->height());
                        }
                    } else if (isOverlappedWithScreenClipRect || gotNewBuffer) {
                    } else {
                        ctx.layerClipRect = LayoutRect(0, 0, 0, 0);
                        needsInitialClip = true;
                    }

                    if (needsInitialClip) {
                        deviceLayerClipRect =
                            canvas->pixelSnappedClip(ctx.layerClipRect);
                    } else {
                        deviceLayerClipRect =
                            LayoutRect(0, 0, canvasSurface->bufferWidth(),
                                       canvasSurface->bufferHeight());
                    }
                    canvas->translate(-minX, -minY);
                    canvas->translate(-ctx.layerBaseX, -ctx.layerBaseY);

                    ComputedStyle* cs = owner()->node()->style();
                    bool canSkipClear =
                        gotNewBuffer ||
                        (!isRootContext() && !isIFrameStackingContext() &&
                         !cs->backgroundColor()
                              .hasAlpha() && // if bg-color is solid
                         cs->outlineColor().isTransparent() && // no outline
                         !cs->boxShadow());                    // no shadow

                    if (!canSkipClear) {
                        if (needsInitialClip) {
                            canvas->clearColor(Unit::Color(0, 0, 0, 0));
                        } else if (!gotNewBuffer) {
                            memset(canvas->renderTargetInfo().m_buffer, 0,
                                   canvas->renderTargetInfo().m_stride *
                                       canvas->renderTargetInfo().m_height);
                        }
                    }

                    fillGraphicsBufferContents(canvas, ctx);

                    delete canvas;

                    deviceLayerClipRect = LayoutRect::overlappedRect(
                        deviceLayerClipRect,
                        LayoutRect(0, 0,
                                   (LayoutUnit)canvasSurface->bufferWidth(),
                                   (LayoutUnit)canvasSurface->bufferHeight()));

                    if ((bool)deviceLayerClipRect.width() ||
                        (bool)deviceLayerClipRect.height()) {
                        canvasSurface->unmapBufferAndNotifyUpdatedRegion(
                            (int)deviceLayerClipRect.x(),
                            (int)deviceLayerClipRect.y(),
                            (int)deviceLayerClipRect.width(),
                            (int)deviceLayerClipRect.height());
                    } else {
                        canvasSurface->unmapBufferAndNotifyUpdatedRegion(0, 0,
                                                                         0, 0);
                    }

                    drawnSomething = true;
                }
            }

            tileIndex++;
            coveredColsCount += wTileSize;
        }

        coveredRowsCount += hTileSize;
    }

    return drawnSomething;
}

void StackingContext::paintStackingContext(Canvas* canvas,
                                           PaintingStackingContextContext& ctx)
{
    if (needsGraphicsBuffer()) {
        ensureRareData()->m_textDecorationData = canvas->textDecorationData();
        return;
    }

    {
        // draw debug rect
        // canvas->save();
        // canvas->setFillColor(Color(0, 0, 255, 64));
        // canvas->drawRect(visibleRect);
        // canvas->restore();
    }

    float opacity = owner()->style()->opacity();

    if (opacity == 0) {
        // invisible from here
        return;
    }

    canvas->save();

    if (opacity != 1) {
        canvas->beginOpacityLayer(owner()->style()->opacity());
    }

    {
        SkMatrix m = transformMatrix();

        if (!m.isIdentity()) {
            SkMatrix test;
            bool testResult = m_rareData->m_matrix.invert(&test);
            if (!testResult) {
                // ignorePaintingDueToInvalidMatrix
                if (owner()->style()->opacity() != 1) {
                    canvas->endOpacityLayer();
                }
                canvas->restore();
                return;
            }
            LayoutLocation to = transformOrigin();
            canvas->translate(to.x(), to.y());
            canvas->postMatrix(m);
            canvas->translate(-to.x(), -to.y());
        }
    }

    if (owner()->shouldResetTextDecoration()) {
        canvas->resetTextDecorationData();
    } else {
        canvas->mergeTextDecorationData(owner()->style());
    }

    if (owner()->style()->visibility() ==
        VisibilityValue::HiddenVisibilityValue) {
        canvas->setVisible(false);
    } else {
        canvas->setVisible(true);
    }

    if (owner()->isAbsolutePositioned()) {
        RectData* rect = owner()->style()->clip();
        if (rect) {
            canvas->clip(Unit::Rect(
                rect->left().numberData(), rect->top().numberData(),
                rect->right().numberData(), rect->bottom().numberData()));
        }
    }

    bool canRejectPainting;
    if (owner()->shouldApplyOverflow()) {
        canRejectPainting =
            canvas->canRejectPainting(m_owner->frameVisibleRect());
    } else {
        canRejectPainting =
            canvas->canRejectPainting(StackingContext::visibleRect());
    }

    applyMask(canvas, ctx);

    if (!canRejectPainting) {
        if (m_hasFilterEffect) {
            FilterContext filterContext(&canvas, this, ctx);
            m_owner->paintBackgroundAndBorders(canvas);
            filterContext.applyAllFilter(ctx);
        } else {
            m_owner->paintBackgroundAndBorders(canvas);
        }
    }

    // Within each stacking context, the following layers are painted in
    // back-to-front order:
    // the background and borders of the element forming the stacking context.
    if (isIFrameStackingContext()) {
        FrameBlockBox* document = m_owner->layoutParent()->asFrameBlockBox();
        FrameBox* iframeBox = m_owner->node()
                                  ->document()
                                  ->browsingContext()
                                  ->sourceElement()
                                  ->frame()
                                  ->asFrameBox();
        auto clipRect = iframeBox->makeRect(BoxValue::PaddingBoxBoxValue);
        HTMLIFrameElement* iframe =
            m_owner->node()->document()->browsingContext()->sourceElement();
        clipRect.setX(clipRect.x() + m_owner->node()
                                         ->document()
                                         ->browsingContext()
                                         ->window()
                                         ->scrollX());
        clipRect.setY(clipRect.y() + m_owner->node()
                                         ->document()
                                         ->browsingContext()
                                         ->window()
                                         ->scrollY());
        canvas->clip(clipRect);
        canvas->translate(iframeBox->borderLeft() + iframeBox->paddingLeft(),
                          iframeBox->borderTop() + iframeBox->paddingTop());

        if (needsGraphicsBuffer()) {
            canvas->save();
            canvas->resetMatrixAndClip();

            m_owner->node()
                ->document()
                ->browsingContext()
                ->paintWindowBackground(canvas);
            canvas->restore();
        } else {
            m_owner->node()
                ->document()
                ->browsingContext()
                ->paintWindowBackground(canvas);
        }
    }

    if (owner()->shouldApplyOverflow()) {
        canvas->clip(owner()->makeRect(BoxValue::PaddingBoxBoxValue));
        const LayoutRect rect(0, 0, m_owner->width(), m_owner->height());
        m_owner->applyBorderRadiusClippingIfNeeds(canvas, rect);
        if (m_owner->isFrameBlockBox()) {
            canvas->translate(-m_owner->asFrameBlockBox()->scrollLeft(),
                              -m_owner->asFrameBlockBox()->scrollTop());
        }
    }

    // the child stacking contexts with negative stack levels (most negative
    // first).
    {
        auto iter = childContexts().begin();
        while (iter != childContexts().end()) {
            StackingContextChild* child = *iter;
            int32_t num = child->at(0)->zIndex();
            if (num >= 0) {
                break;
            }
            auto iter2 = child->begin();
            while (iter2 != child->end()) {
                StackingContext* sCtx = *iter2;
                CanvasStateRestorer r(canvas, sCtx, m_owner, ctx);
                sCtx->paintStackingContext(canvas, ctx);
                iter2++;
            }
            iter++;
        }
    }

    if (!canRejectPainting) {
        if (m_hasFilterEffect) {
            FilterContext filterContext(&canvas, this, ctx);
            m_owner->paintStackingContextContent(canvas);
            m_owner->paintOutline(canvas);
            filterContext.applyAllFilter(ctx);
        } else {
            m_owner->paintStackingContextContent(canvas);
            m_owner->paintOutline(canvas);
        }
    }

    // the child stacking contexts with positive stack levels (least positive
    // first).
    {
        auto iter = childContexts().begin();
        while (iter != childContexts().end()) {
            StackingContextChild* child = *iter;
            int32_t num = child->at(0)->zIndex();
            if (num >= 0) {
                auto iter2 = child->begin();
                while (iter2 != child->end()) {
                    StackingContext* sCtx = *iter2;
                    CanvasStateRestorer r(canvas, sCtx, m_owner, ctx);
                    sCtx->paintStackingContext(canvas, ctx);
                    iter2++;
                }
            }
            iter++;
        }
    }

    if (opacity != 1) {
        canvas->endOpacityLayer();
    }

    paintScrollbar(canvas);

    canvas->restore();
}

void StackingContext::paintScrollbar(Canvas* canvas)
{
    if (isIFrameStackingContext()) {
        HTMLIFrameElement* iframe =
            m_owner->node()->document()->browsingContext()->sourceElement();
        if (!iframe->scrolling()->toASCIILower()->equals("no")) {
            canvas->save();
            canvas->translate(m_owner->node()
                                  ->document()
                                  ->browsingContext()
                                  ->window()
                                  ->scrollX(),
                              m_owner->node()
                                  ->document()
                                  ->browsingContext()
                                  ->window()
                                  ->scrollY());
            FrameBlockBox* document =
                m_owner->layoutParent()->asFrameBlockBox();
            if (!needsGraphicsBuffer()) {
                Scrolling::paintScrollbars<Canvas*>(
                    m_owner->node()
                        ->document()
                        ->browsingContext()
                        ->window()
                        ->scrolling(),
                    canvas, document, document->appliedOverflowX(),
                    document->appliedOverflowY());
            }
            canvas->restore();
        }
    } else if (!isRootContext()) {
        if (m_owner->shouldApplyOverflow() && m_owner->node() &&
            m_owner->node()->isElement() && m_owner->isFrameBlockBox() &&
            !needsComposite()) {
            Scrolling::paintScrollbars<Canvas*>(
                m_owner->node()->asElement()->rareMembers()
                    ? m_owner->node()->asElement()->rareMembers()->m_scrolling
                    : nullptr,
                canvas, m_owner->asFrameBlockBox(), m_owner->appliedOverflowX(),
                m_owner->appliedOverflowY());
        }
    }
}

void StackingContext::compositeScrollbar(Compositor* compositor)
{
    if (isIFrameStackingContextOwner()) {
        if (m_childContexts.size()) {
            StackingContext* childCtx = m_childContexts[0]->at(0);
            if (childCtx->needsGraphicsBuffer()) {
                auto bc = m_owner->node()
                              ->asHTMLIFrameElement()
                              ->contentDocument()
                              ->browsingContext();
                {
                    CompositorStateRestorer r(compositor, this,
                                              parent()->owner());
                    compositor->translate(
                        m_owner->borderLeft() + m_owner->paddingLeft(),
                        m_owner->borderTop() + m_owner->paddingTop());
                    FrameBlockBox* mainFrame =
                        bc->document()->frame()->asFrameBlockBox();
                    Scrolling::paintScrollbars<Compositor*>(
                        m_owner->node()
                            ->document()
                            ->browsingContext()
                            ->window()
                            ->scrolling(),
                        compositor, mainFrame, mainFrame->appliedOverflowX(),
                        mainFrame->appliedOverflowY());
                }
            }
        }
    } else if (!isRootContext()) {
        if (m_owner->shouldApplyOverflow() && m_owner->node() &&
            m_owner->node()->isElement() && m_owner->isFrameBlockBox()) {
            Scrolling::paintScrollbars<Compositor*>(
                m_owner->node()->asElement()->rareMembers()
                    ? m_owner->node()->asElement()->rareMembers()->m_scrolling
                    : nullptr,
                compositor, m_owner->asFrameBlockBox(),
                m_owner->appliedOverflowX(), m_owner->appliedOverflowY());
        }
    }
}

void StackingContext::compositeStackingContext(Compositor* compositor)
{
    STARFISH_ASSERT(compositor != nullptr);
    STARFISH_ASSERT(needsComposite());

    LayoutRect visibleRect = StackingContext::visibleRect();
    LayoutUnit minX = visibleRect.x();
    LayoutUnit maxX = visibleRect.maxX();
    LayoutUnit minY = visibleRect.y();
    LayoutUnit maxY = visibleRect.maxY();

    size_t bufferWidth;
    size_t bufferHeight;
    computeBufferSizeFromVisibleRect(minX, minY, maxX, maxY, bufferWidth,
                                     bufferHeight);

    bool thereIsNoBufferBecauseThereIsNoVisibleContent =
        !bufferWidth || !bufferHeight;

    ComputedStyle* ownerStyle = m_owner->style();
    STARFISH_ASSERT(ownerStyle != nullptr);

    FrameBox* parentBox = parent() ? parent()->owner() : nullptr;

    CompositorStateRestorer r(compositor, this, parentBox);

    // If current matrix is invalid, we could not composite StackckingContext
    SkMatrix test;
    if (!r.m_compositor->currentTransformMatrix().invert(&test)) {
        return;
    }

    if (isIFrameStackingContextOwner()) {
        if (m_childContexts.size()) {
            StackingContext* childCtx = m_childContexts[0]->at(0);
            STARFISH_ASSERT(childCtx != nullptr);

            if (childCtx->needsGraphicsBuffer()) {
                auto bc = m_owner->node()
                              ->asHTMLIFrameElement()
                              ->contentDocument()
                              ->browsingContext();
                auto bgColor = bc->hasWindowBackgroundColor();
                if (bgColor.first) {
                    CompositorStateRestorer r(compositor, this,
                                              parent()->owner());

                    compositor->save();
                    compositor->setFillColor(bgColor.second);
                    compositor->drawRect(LayoutRect(
                        m_owner->borderLeft() + m_owner->paddingLeft(),
                        m_owner->borderTop() + m_owner->paddingTop(),
                        m_owner->contentWidth(), m_owner->contentHeight()));
                    compositor->restore();
                }
            }
        }
    }

    if (!thereIsNoBufferBecauseThereIsNoVisibleContent) {
        owner()->willCompsiteStackingContext(compositor);

        bool hasFilterEffect = false;
        if (Compositor::supportsFilterEffect(bufferWidth, bufferHeight) &&
            m_hasFilterEffect) {
            hasFilterEffect = true;
            Length standardDeviation;
            float maxRadiusOffset = 0;
            if (ownerStyle->hasAvailableFilter() &&
                ownerStyle->filter()->getStandardDeviationOfBlurFilter(
                    standardDeviation)) {
                maxRadiusOffset =
                    std::max(maxRadiusOffset, (standardDeviation.numberData()));
            }

            for (auto ancestor : ancestorsThatHasFilters()) {
                auto s = ancestor->owner()->style();
                if (s->filter()->getStandardDeviationOfBlurFilter(
                        standardDeviation)) {
                    maxRadiusOffset = std::max(
                        maxRadiusOffset, (standardDeviation.numberData()));
                }
            }

            if (maxRadiusOffset) {
                compositor->save();
                compositor->enableBlurEffect(maxRadiusOffset);
            }
        }

        if (owner()->hasOwnGraphicsBufferMethod()) {
            if (m_rareData->m_graphicsBufferHolder) {
                compositor->save();
                compositor->translate(minX, minY);

                if (owner()->needsToPaintBackgroundOrBorderOrBoxShadow()) {
                    CanvasSurface* backgroundSurface = CanvasSurface::create(
                        m_owner->document()->webView()->platformWindow(),
                        bufferWidth, bufferHeight, 1,
                        CanvasSurface::CanvasElement);
                    Canvas* canvas = Canvas::create(m_owner->node()->webView(),
                                                    backgroundSurface);
                    canvas->clearColor(Unit::Color(0, 0, 0, 0));
                    canvas->setTextDecorationData(
                        m_rareData->m_textDecorationData);
                    owner()->asFrameBox()->paintBackgroundAndBorders(canvas);
                    delete canvas;
                    backgroundSurface->unmapBufferAndNotifyUpdatedRegion(
                        0, 0, backgroundSurface->bufferWidth(),
                        backgroundSurface->bufferHeight());
                    compositor->drawSurface(
                        backgroundSurface,
                        Unit::Rect(0, 0, backgroundSurface->bufferWidth(),
                                   backgroundSurface->bufferHeight()));
                    backgroundSurface->detachNativeBuffer();
                }

                auto dx = owner()->borderLeft() + owner()->paddingLeft();
                auto dy = owner()->borderTop() + owner()->paddingTop();
                compositor->translate(dx, dy);

                auto surface =
                    m_rareData->m_graphicsBufferHolder->m_surfaces[0];
                compositor->drawSurface(
                    surface, Unit::Rect(0, 0, owner()->contentWidth(),
                                        owner()->contentHeight()));
                compositor->restore();
            }
        } else if (m_rareData->m_graphicsBufferHolder) {
            size_t wTileSize =
                m_rareData->m_graphicsBufferHolder->m_tileDataWidth;
            size_t hTileSize =
                m_rareData->m_graphicsBufferHolder->m_tileDataHeight;
            size_t wTextureCount =
                m_rareData->m_graphicsBufferHolder->m_horizontalTileCount;
            size_t hTextureCount =
                m_rareData->m_graphicsBufferHolder->m_verticalTileCount;

            size_t tileIndex = 0;
            size_t coveredRowsCount = 0;

            compositor->translate(minX, minY);

            for (size_t y = 0; y < hTextureCount; y++) {
                size_t coveredColsCount = 0;
                for (size_t x = 0; x < wTextureCount; x++) {
                    size_t tileDataX = coveredColsCount;
                    size_t tileDataY = coveredRowsCount;
                    size_t tileDataWidth = std::min(
                        wTileSize,
                        m_rareData->m_graphicsBufferHolder->bufferWidth() -
                            coveredColsCount);
                    size_t tileDataHeight = std::min(
                        hTileSize,
                        m_rareData->m_graphicsBufferHolder->bufferHeight() -
                            coveredRowsCount);

                    if (tileIndex < m_rareData->m_graphicsBufferHolder
                                        ->m_surfaces.size() &&
                        m_rareData->m_graphicsBufferHolder
                                ->m_surfaces[tileIndex] != nullptr) {
                        compositor->drawSurface(
                            m_rareData->m_graphicsBufferHolder
                                ->m_surfaces[tileIndex],
                            Unit::Rect(tileDataX, tileDataY, tileDataWidth,
                                       tileDataHeight));
                    }
                    tileIndex++;
                    coveredColsCount += wTileSize;
                }

                coveredRowsCount += hTileSize;
            }

            compositor->translate(-minX, -minY);
        }

#ifdef STARFISH_ENABLE_TEST
        if (UNLIKELY(owner()->node()->webView()->startUpFlag() &
                     StarfishStartUpFlag::enableDebugGraphicsLayer)) {
            // debug compositing method
            switch (m_needsGraphicsBufferReason) {
            case NeedsGraphicsLayerReasonNone:
                compositor->setFillColor(Unit::Color(255, 64, 0, 64));
                break;
            case NeedsGraphicsLayerReasonBySelf:
                compositor->setFillColor(Unit::Color(255, 0, 0, 64));
                break;
            case NeedsGraphicsLayerReasonNotCoveredByParent:
                compositor->setFillColor(Unit::Color(0, 255, 0, 64));
                break;
            case NeedsGraphicsLayerReasonCollapsedWithSiblingLayer:
                compositor->setFillColor(Unit::Color(0, 0, 255, 64));
                break;
            case NeedsGraphicsLayerReasonSiblingLayerNeedsAnimation:
                compositor->setFillColor(Unit::Color(0, 255, 255, 64));
                break;
            default:
                STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
            }
            compositor->beginOpacityLayer(0.5);
            compositor->drawRect(
                Unit::Rect(minX, minY, bufferWidth, bufferHeight));
            compositor->endOpacityLayer();
        }

        if (UNLIKELY(owner()->node()->webView()->startUpFlag() &
                     StarfishStartUpFlag::enableDebugRepaintRegion)) {
            auto iter =
                owner()->node()->webView()->repaintRegionInRendering().find(
                    owner()->node());

            if (iter !=
                owner()->node()->webView()->repaintRegionInRendering().end()) {
                compositor->beginOpacityLayer(0.5);
                compositor->setFillColor(Unit::Color(0, 255, 0, 64));
                compositor->drawRect(iter->second);
                compositor->endOpacityLayer();
            }
        }
#endif

        if (hasFilterEffect) {
            compositor->restore();
        }

        owner()->didCompsiteStackingContext(compositor);
    }

    compositeScrollbar(compositor);
}

LayoutLocation StackingContext::relativeLocation(StackingContext* sCtx)
{
    LayoutLocation l = sCtx->owner()->absolutePoint(m_owner);
    bool isFixed = sCtx->owner()->style()->position() == FixedPositionValue;

    if (isFixed) {
        Frame* parent = sCtx->owner()->layoutParent();
        while ((!parent->style() || !parent->style()->hasTransforms(parent)) &&
               !parent->isFrameDocument() &&
               (isFixed || !parent->isPositioned())) {
            parent = parent->layoutParent();
        }

        if (isFixed && parent->isFrameBlockBox()) {
            l.setX(l.x() + parent->asFrameBlockBox()->scrollLeft());
            l.setY(l.y() + parent->asFrameBlockBox()->scrollTop());
        }
    } else {
        FrameBox* box = sCtx->owner()->layoutParent()->asFrameBox();
        while (!box->stackingContext()) {
            if (box->isFrameBlockBox()) {
                l.setX(l.x() - box->asFrameBlockBox()->scrollLeft());
                l.setY(l.y() - box->asFrameBlockBox()->scrollTop());
            }

            box = box->layoutParent()->asFrameBox();
        }
    }

    return l;
}

Frame* StackingContext::hitTestStackingContext(LayoutUnit x, LayoutUnit y,
                                               BrowsingContext* from)
{
    SkMatrix m = transformMatrix();
    if (!m.isIdentity()) {
        SkMatrix invert;
        if (!m.invert(&invert)) {
            return nullptr;
        }

        auto to = transformOrigin();
        LayoutUnit ox = to.x();
        LayoutUnit oy = to.y();
        x -= ox;
        y -= oy;
        SkPoint pt = SkPoint::Make((float)x, (float)y);
        invert.mapPoints(&pt, 1);
        x = pt.x() + ox;
        y = pt.y() + oy;
    }

    if (!m_owner->isAnonymous() &&
        m_owner->node()->document()->browsingContext() != from) {
        if (m_owner->FrameBox::hitTest(x, y, HitTestStageEnd)) {
            return m_owner->node()
                ->document()
                ->browsingContext()
                ->sourceElement()
                ->frame();
        } else {
            return nullptr;
        }
    }

    if (owner()->style()->visibility() ==
        VisibilityValue::HiddenVisibilityValue) {
        return nullptr;
    }

    if (owner()->shouldApplyOverflow()) {
        if (owner()->isFrameReplaced()) {
            return owner()->hitTest(x, y, HitTestStageEnd);
        } else if (owner()->FrameBox::hitTest(x, y, HitTestStageEnd) ==
                   nullptr) {
            return nullptr;
        }
    }

    if (m_owner->isFrameBlockBox()) {
        x += m_owner->asFrameBlockBox()->scrollLeft();
        y += m_owner->asFrameBlockBox()->scrollTop();
    }

    Frame* result = nullptr;
    // the child stacking contexts with positive stack levels (least positive
    // first).
    {
        auto iter = childContexts().rbegin();
        while (iter != childContexts().rend()) {
            StackingContextChild* child = *iter;
            int32_t num = child->at(0)->zIndex();
            if (num >= 0) {
                auto iter2 = child->rbegin();
                LayoutUnit oldX = x;
                LayoutUnit oldY = y;
                while (iter2 != child->rend()) {
                    StackingContext* sCtx = *iter2;
                    LayoutLocation l = relativeLocation(sCtx);
                    x -= l.x();
                    y -= l.y();
                    result = sCtx->hitTestStackingContext(x, y, from);
                    x = oldX;
                    y = oldY;
                    if (result) {
                        // TODO test overflow between sCtx and this
                        return result;
                    }

                    iter2++;
                }
            }
            iter++;
        }
    }

    // the child stacking contexts with stack level 0 and the positioned
    // descendants with stack level 0.
    result = m_owner->hitTestChildrenWith(x, y, HitTestPositionedElements);
    if (result) {
        return result;
    }

    // the in-flow, inline-level, non-positioned descendants, including inline
    // tables and inline blocks.
    result = m_owner->hitTestChildrenWith(x, y, HitTestNormalFlowInline);
    if (result) {
        return result;
    }

    // the non-positioned float.
    result = m_owner->hitTestChildrenWith(x, y, HitTestNonPositionedFloats);
    if (result) {
        return result;
    }

    // the in-flow, non-inline-level, non-positioned descendants.
    result = m_owner->hitTestChildrenWith(x, y, HitTestNormalFlowBlock);
    if (result) {
        return result;
    }

    // the child stacking contexts with negative stack levels (most negative
    // first).
    {
        auto iter = childContexts().rbegin();
        while (iter != childContexts().rend()) {
            StackingContextChild* child = *iter;
            int32_t num = child->at(0)->zIndex();
            if (num > 0) {
                break;
            }
            auto iter2 = child->rbegin();
            LayoutUnit oldX = x;
            LayoutUnit oldY = y;
            while (iter2 != child->rend()) {
                StackingContext* sCtx = *iter2;
                LayoutLocation l = relativeLocation(sCtx);
                x -= l.x();
                y -= l.y();
                result = sCtx->hitTestStackingContext(x, y, from);
                if (result) {
                    return result;
                }

                x = oldX;
                y = oldY;
                iter2++;
            }
            iter++;
        }
    }

    if (m_owner->isFrameBlockBox()) {
        x -= m_owner->asFrameBlockBox()->scrollLeft();
        y -= m_owner->asFrameBlockBox()->scrollTop();
    }

    // the background and borders of the element forming the stacking context.
    result = m_owner->FrameBox::hitTest(x, y, HitTestNormalFlowBlock);
    if (result) {
        return result;
    }

    return nullptr;
}

void StackingContext::applyMask(Canvas* canvas,
                                PaintingStackingContextContext& ctx)
{
    if (m_owner->isFrameSVGBox()) {
        return;
    }

    if (m_owner->style() == nullptr || m_owner->style()->maskLayerSize() == 0) {
        return;
    }

    auto style = m_owner->style();
    auto document = m_owner->document();
    for (uint32_t i = 0; i < m_owner->style()->maskLayerSize(); i++) {
        if (style->maskImage(i) == nullptr) {
            continue;
        }

        auto type = style->maskImage(i)->type();
        if (!type.isGradient()) {
            STARFISH_UNIMPLEMENTED();
            continue;
        }

        ImageValue* imageValue = style->maskImage(i);
        auto gradientValue = imageValue->gradientValue();
        Unit::Rect rect =
            m_owner->makeRect(BoxValue::BorderBoxBoxValue).snapSizeToPixel();
        bool cacheable =
            gradientValue->isCacheable(rect.width(), rect.height(), false);
        auto info =
            imageValue->gradientValue()->makeGradientDrawingInfo(rect, m_owner);
        NativeImageData* gradientNativeImageData = nullptr;
        if (cacheable) {
            std::shared_ptr<NativeGradient> gradient =
                document->findInNativeGradientCache(info);

            if (gradient.get() == nullptr) {
                gradient = NativeGradient::create(info);
            }

            if (gradient->gradientImageDataCached() == nullptr) {
                auto imageData = BufferedNativeImageData::create(rect.width(),
                                                                 rect.height());
                Canvas* gradientCanvas =
                    Canvas::create(m_owner->node()->webView(), imageData);
                gradientCanvas->clearColor(Unit::Color(0, 0, 0, 0));
                if (imageValue->gradientValue()->type() ==
                    GradientType::LinearGradient) {
                    gradientCanvas->drawLinearGradient(rect, info,
                                                       gradient.get());
                } else if (imageValue->gradientValue()->type() ==
                           GradientType::RadialGradient) {
                    gradientCanvas->drawRadialGradient(rect, info,
                                                       gradient.get());
                }
                gradientCanvas->fill();
                delete gradientCanvas;
                gradient->setGradientImageDataCached(imageData);
                document->cacheNativeGradient(info, gradient);
            }
            gradientNativeImageData = gradient->gradientImageDataCached();
            canvas->maskNativeImage(gradientNativeImageData, rect, false);
        } else {
            auto gradient = NativeGradient::create(info);
            auto imageData =
                BufferedNativeImageData::create(rect.width(), rect.height());
            Canvas* gradientCanvas =
                Canvas::create(m_owner->node()->webView(), imageData);
            gradientCanvas->clearColor(Unit::Color(0, 0, 0, 0));
            if (imageValue->gradientValue()->type() ==
                GradientType::LinearGradient) {
                gradientCanvas->drawLinearGradient(rect, info, gradient.get());
            } else if (imageValue->gradientValue()->type() ==
                       GradientType::RadialGradient) {
                gradientCanvas->drawRadialGradient(rect, info, gradient.get());
            }
            gradientCanvas->fill();
            delete gradientCanvas;
            gradientNativeImageData = imageData;
            canvas->maskNativeImage(gradientNativeImageData, rect);
        }
    }
}

} // namespace Starfish
