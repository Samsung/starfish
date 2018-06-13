/*
 * Copyright (C) 2009, 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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

#include "StarFishConfig.h"

#include "core/layout/StackingContext.h"

#include "StarFish.h"
#include "core/dom/Node.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/FrameReplaced.h"
#include "core/page/BrowsingContext.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "platform/window/PlatformWindow.h"

namespace StarFish {

struct StackingContext::ComputeStackingContextContext {
    StackingContext* rootLayer;
    StackingContext* compositingAncestor;
    std::shared_ptr<std::unordered_map<StackingContext*, LayoutRect>>
        extentPerLayer;
    std::shared_ptr<std::vector<StackingContext*>> compositedLayers;
    std::shared_ptr<bool> overlapMapFilled;
    std::shared_ptr<std::set<StackingContext*>>
        seenPossiblyNonCompositeLayers; // when found prev computing
    std::shared_ptr<std::vector<StackingContext*>>
        seenPossiblyNonCompositeLayersNow;
    std::shared_ptr<std::unordered_map<StackingContext*, bool>>
        compositeFlagInfo;
    bool subLayerHasGraphicsBuffer;
    bool testingOverlap;

    ComputeStackingContextContext(StackingContext* rootLayer,
                                  StackingContext* compositingAncestor,
                                  std::shared_ptr<std::set<StackingContext*>>
                                      seenPossiblyNonCompositeLayers,
                                  bool testingOverlap = true)
        : rootLayer(rootLayer)
        , compositingAncestor(compositingAncestor)
        , extentPerLayer(new std::unordered_map<StackingContext*, LayoutRect>())
        , compositedLayers(new std::vector<StackingContext*>())
        , overlapMapFilled(new bool(false))
        , seenPossiblyNonCompositeLayers(seenPossiblyNonCompositeLayers)
        , seenPossiblyNonCompositeLayersNow(new std::vector<StackingContext*>())
        , compositeFlagInfo(new std::unordered_map<StackingContext*, bool>())
        , subLayerHasGraphicsBuffer(false)
        , testingOverlap(testingOverlap)
    {
    }

    ComputeStackingContextContext(const ComputeStackingContextContext& other)
        : rootLayer(other.rootLayer)
        , compositingAncestor(other.compositingAncestor)
        , extentPerLayer(other.extentPerLayer)
        , compositedLayers(other.compositedLayers)
        , overlapMapFilled(other.overlapMapFilled)
        , seenPossiblyNonCompositeLayers(other.seenPossiblyNonCompositeLayers)
        , seenPossiblyNonCompositeLayersNow(
              other.seenPossiblyNonCompositeLayersNow)
        , compositeFlagInfo(other.compositeFlagInfo)
        , subLayerHasGraphicsBuffer(other.subLayerHasGraphicsBuffer)
        , testingOverlap(other.testingOverlap)
    {
    }

    LayoutRect computeLayerExtent(StackingContext* c, SkMatrix m)
    {
        LayoutRect rt(LayoutLocation(), c->owner()->frameRect().size());
        if (m.rectStaysRect()) {
            SkRect skRect =
                SkRect::MakeXYWH((float)rt.x(), (float)rt.y(),
                                 (float)rt.width(), (float)rt.height());
            m.mapRect(&skRect);
            skRect.sort();

            return LayoutRect(skRect.x(), skRect.y(), skRect.width(),
                              skRect.height());
        } else {
            SkPoint pt[4];

            pt[0].fX = rt.x();
            pt[0].fX = rt.y();

            pt[1].fX = rt.maxX();
            pt[1].fX = rt.y();

            pt[2].fX = rt.x();
            pt[2].fX = rt.maxY();

            pt[3].fX = rt.maxX();
            pt[3].fX = rt.maxY();

            m.mapPoints(pt, 4);

            LayoutUnit minX = pt[0].x();
            LayoutUnit minY = pt[0].y();
            LayoutUnit maxX = pt[0].x();
            LayoutUnit maxY = pt[0].y();

            for (size_t i = 1; i < 4; i++) {
                minX = std::min((float)pt[i].x(), (float)minX);
                minY = std::min((float)pt[i].y(), (float)minY);

                maxX = std::max((float)pt[i].x(), (float)maxX);
                maxY = std::max((float)pt[i].y(), (float)maxY);
            }

            return LayoutRect(minX, minY, (maxX - minX).abs(),
                              (maxY - minY).abs());
        }
    }

    LayoutRect screenExtentPerLayer(StackingContext* c)
    {
        auto iter = extentPerLayer->find(c);
        if (iter != extentPerLayer->end()) {
            return iter->second;
        }

        StackingContext* cur = c;

        std::vector<StackingContext*> path;
        while (cur != rootLayer) {
            path.push_back(cur);
            cur = cur->parent();
        }

        SkMatrix m = SkMatrix::I();
        FrameBox* before = rootLayer->owner();
        FrameBox* after;
        for (size_t i = 0; i < path.size(); i++) {
            after = path[i]->owner();

            SkMatrix m2 = after->stackingContext()->transformMatrix();
            if (!m2.isIdentity()) {
                LayoutLocation to = after->stackingContext()->transformOrigin();
                m.postTranslate((float)to.x(), (float)to.y());
                m.preConcat(m2);
                m.postTranslate(-(float)to.x(), -(float)to.y());
            }

            auto pos = after->absolutePointIncludingScroll(before);
            m.postTranslate((float)pos.x(), (float)pos.y());

            after = before;
        }

        LayoutRect rt = computeLayerExtent(c, m);

        extentPerLayer->insert(std::make_pair(c, rt));

        return rt;
    }

    bool isOverlap(StackingContext* a, StackingContext* b)
    {
        auto extentA = screenExtentPerLayer(a);
        auto extentB = screenExtentPerLayer(b);
        return extentA.intersects(extentB);
    }

    void pushCompsitedLayer(StackingContext* c)
    {
        if (!c->isRootContext()) {
            compositedLayers->push_back(c);
        }
    }

    bool isOverlapWithAlreadyCompositedLayer(StackingContext* a)
    {
        auto extentA = screenExtentPerLayer(a);
        for (size_t i = 0; i < compositedLayers->size(); i++) {
            auto extentB = screenExtentPerLayer(compositedLayers->at(i));
            if (extentA.intersects(extentB)) {
                return true;
            }
        }
        return false;
    }
};

StackingContextRareData::StackingContextRareData()
    : m_needsGraphicsBuffer(false)
    , m_hasNon2DRectTransform(false)
    , m_isVisibleRectComputedForNonGraphicsLayer(false)
    , m_visibleRect(0, 0, 0, 0)
    , m_buffer(nullptr)
    , m_matrix()
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
    : m_needsRepainting(true)
    , m_catchedMatrixChangedWhileComputeStackingContextProperties(false)
    , m_owner(owner)
    , m_parent(parent)
    , m_rareData(nullptr)
{
    if (m_parent) {
        int32_t num = owner->isPositioned() ? owner->style()->zIndex() : 0;
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
    if (m_owner->isPositioned()) {
        return m_owner->style()->zIndex();
    } else {
        return 0;
    }
}

void StackingContext::clearGraphicsBuffer(bool needsDetachNative)
{
    if (m_rareData && m_rareData->m_buffer) {
        if (needsDetachNative) {
            m_rareData->m_buffer->detachNativeBuffer();
        } else {
            m_owner->node()
                ->webView()
                ->m_backStackingContextBufferUpWhileReCompsite.push_back(
                    m_rareData->m_buffer);
        }
        m_rareData->m_buffer = nullptr;
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

class CanvasStateRestorer {
private:
    std::vector<std::pair<Frame*, std::pair<bool, bool>>>
        m_canApplyOverflowOrScrolls;
    struct OverflowStatus {
        Frame* m_child;
        bool m_seenContainingBlockForAbsBlock;
        bool m_seenAbsBlock;
        OverflowStatus(Frame* child)
        {
            reset(child);
        }

        bool canApplyOverflow(Frame* parent)
        {
            if (!parent) {
                return false;
            }

            if (!parent->style()) {
                STARFISH_ASSERT(parent->isLineBox());
                return false;
            }

            if (parent->isFrameReplaced() &&
                parent->asFrameReplaced()->isFrameReplacedIFrame()) {
                return true;
            }

            if (m_child->isAbsolutePositioned()) {
                if (m_seenContainingBlockForAbsBlock) {
                    return parent->shouldApplyOverflow();
                }
                bool b = parent->canBeContainingBlockOfAbsolutePositionedBox(
                    m_child);
                m_seenContainingBlockForAbsBlock =
                    m_seenContainingBlockForAbsBlock || b;
                return b && parent->shouldApplyOverflow();
            } else {
                if (m_seenAbsBlock) {
                    return false;
                }
                m_seenAbsBlock =
                    m_seenAbsBlock || parent->isAbsolutePositioned();
            }

            return parent->shouldApplyOverflow();
        }

        void reset(Frame* f)
        {
            m_child = f;
            m_seenContainingBlockForAbsBlock = false;
            m_seenAbsBlock = false;
        }
    };

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
    CanvasStateRestorer(Canvas* canvas, StackingContext* sCtx, FrameBox* owner)
        : m_canvas(canvas)
    {
        canvas->save();

        FrameBox* self = sCtx->owner();

        if (self->style()->position() != FixedPositionValue) {
            bool needsRestore = false;
            Frame* s = self->layoutParent();
            while (s) {
                if ((s->shouldApplyOverflow() &&
                     !s->isEstablishesStackingContext()) ||
                    (s->style() &&
                     s->style()->position() == FixedPositionValue)) {
                    needsRestore = true;
                    break;
                }
                s = s->layoutParent();
            }

            if (!needsRestore) {
                auto o = self->absolutePointIncludingScroll(owner);
                canvas->translate(o.x(), o.y());
                return;
            }
        }

        std::vector<FrameBox*> frameList;

        frameList.reserve(32);
        m_canApplyOverflowOrScrolls.reserve(32);

        Frame* nearstBufferedFrame = nullptr;
        bool shareWithStackingBuffer = true;
        {
            Frame* f = self;
            OverflowStatus status(f);
            bool canScroll =
                status.m_child->style()->position() != FixedPositionValue;

            while (f) {
                frameList.push_back(f->asFrameBox());
                f = f->layoutParent();

                if (shareWithStackingBuffer && f &&
                    f->asFrameBox()->stackingContext() &&
                    f->asFrameBox()->stackingContext()->needsGraphicsBuffer()) {
                    nearstBufferedFrame = f;
                    shareWithStackingBuffer = false;
                }

                if (shareWithStackingBuffer) {
                    if (status.canApplyOverflow(f)) {
                        insertIntoCanApplyOverflowOrScrolls(
                            f, std::make_pair(true, canScroll && f &&
                                                        f->isFrameBlockBox()));
                        status.reset(f);
                        canScroll = status.m_child->style()->position() !=
                                    FixedPositionValue;
                    } else {
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

        StackingContext* sc = sCtx->parent();
        while (true) {
            if (sc == nullptr) {
                break;
            }
            if (sc->needsGraphicsBuffer()) {
                if (sc->buffer()->pixelRatio() != 1) {
                    canvas->scale(1.0 / sc->buffer()->pixelRatio(),
                                  1.0 / sc->buffer()->pixelRatio());
                }
                canvas->translate(-sc->visibleRect().x(),
                                  -sc->visibleRect().y());
                break;
            }
            sc = sc->parent();
        }

        auto iter = frameList.rbegin();
        shareWithStackingBuffer = nearstBufferedFrame == nullptr;
        LayoutUnit dx, dy;
        while (iter != frameList.rend()) {
            FrameBox* b = *iter;

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

            iter++;
        }
        canvas->translate(dx, dy);
    }
    ~CanvasStateRestorer()
    {
        m_canvas->restore();
    }

    Canvas* m_canvas;
};

class CompositorStateRestorer {
public:
    CompositorStateRestorer(Compositor* canvas, StackingContext* sCtx,
                            FrameBox* owner)
        : m_compositor(canvas)
    {
        m_compositor->save();

        FrameBox* self = sCtx->owner();
        LayoutLocation l = self->absolutePoint(owner);
        m_compositor->translate(l.x(), l.y());

        if (self->style()->position() == FixedPositionValue) {
            Frame* parent = self->layoutParent();
            LayoutUnit offsetX = self->x(), offsetY = self->y();
            while (parent->isLineBox() ||
                   !parent->canBeContainingBlockOfAbsolutePositionedBox(self)) {
                offsetX += parent->asFrameBox()->x();
                offsetY += parent->asFrameBox()->y();
                parent = parent->layoutParent();
            }

            if (parent->isFrameBlockBox()) {
                m_compositor->translate(parent->asFrameBlockBox()->scrollLeft(),
                                        parent->asFrameBlockBox()->scrollTop());
            }
        }
    }
    ~CompositorStateRestorer()
    {
        m_compositor->restore();
    }

    Compositor* m_compositor;
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

        m_rareData->m_hasNon2DRectTransform =
            m_owner->style()->has3DTransforms(m_owner) ||
            !m_rareData->m_matrix.rectStaysRect();

#ifdef PORT_CANVAS_BACKEND_EFL
        // force use graphics buffer with complex-transform
        // because efl canvas can't deal well with complex-transform
        m_rareData->m_hasNon2DRectTransform =
            m_owner->style()->hasComplexTransforms(m_owner);
#endif

        if (!m_rareData->m_matrix.isIdentity()) {
            /*
            STARFISH_LOG_INFO("matrix [%f %f %f][%f %f %f][%f %f %f]\n",
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

enum IndirectCompositingReason {
    None,
    SubFrame,
    Stacking,
    Overlap,
    BackgroundLayer,
    GraphicalEffect, // opacity, mask, filter, transform etc.
    Perspective,
    Preserve3D
};

static bool requiresCompositingForIndirectReason(
    StackingContext* ctx, bool hasCompositedDescendants,
    bool has3DTransformedDescendants, IndirectCompositingReason& reason)
{
    // When a layer has composited descendants, some effects, like 2d
    // transforms, filters, masks etc must be implemented
    // via compositing so that they also apply to those composited descendants.
    if (hasCompositedDescendants &&
        ctx->owner()->style()->hasTransforms(ctx->owner())) {
        // && (layer.isolatesCompositedBlending() || layer.transform() ||
        // renderer.createsGroup() || renderer.hasReflection() ||
        // renderer.isRenderNamedFlowFragmentContainer())) {
        reason = IndirectCompositingReason::GraphicalEffect;
        return true;
    }

    if (hasCompositedDescendants && ctx->isIFrameStackingContext()) {
        reason = IndirectCompositingReason::SubFrame;
        return true;
    }

    // A layer with preserve-3d or perspective only needs to be composited if
    // there are descendant layers that
    // will be affected by the preserve-3d or perspective.
    if (has3DTransformedDescendants) {
        // TODO enable this after implement transform3d
        /*
        if (renderer.style().transformStyle3D() == TransformStyle3DPreserve3D) {
            reason = RenderLayer::IndirectCompositingReason::Preserve3D;
            return true;
        }

        if (renderer.style().hasPerspective()) {
            reason = RenderLayer::IndirectCompositingReason::Perspective;
            return true;
        }*/
    }

    reason = IndirectCompositingReason::None;
    return false;
}

void StackingContext::computeStackingContextProperties()
{
    STARFISH_ASSERT(parent() == nullptr);

    std::shared_ptr<std::set<StackingContext*>> seenPossiblyNonCompositeLayers(
        new std::set<StackingContext*>());
    ComputeStackingContextContext ctx(this, nullptr,
                                      seenPossiblyNonCompositeLayers);
    bool descendantHas3DTransform = false;

    size_t prevCnt = SIZE_MAX;
    do {
        prevCnt = seenPossiblyNonCompositeLayers->size();
        ctx.compositeFlagInfo->clear();
        computeStackingContextProperties(ctx, nullptr,
                                         descendantHas3DTransform);
        for (size_t i = 0; i < ctx.seenPossiblyNonCompositeLayersNow->size();
             i++) {
            seenPossiblyNonCompositeLayers->insert(
                ctx.seenPossiblyNonCompositeLayersNow->at(i));
        }
        if (prevCnt == seenPossiblyNonCompositeLayers->size()) {
            break;
        }
    } while (seenPossiblyNonCompositeLayers->size());

    applyStackingContextProperties(ctx);
}

bool StackingContext::canComposite(ComputeStackingContextContext& ctx)
{
    if (m_owner->needsGraphicsBuffer() ||
        m_owner->isRunningOpacityAnimation() ||
        m_owner->isRunningTransformAnimation()) {
        return true;
    }
    auto iter = ctx.seenPossiblyNonCompositeLayers->find(this);
    return ctx.seenPossiblyNonCompositeLayers->end() == iter;
}

void StackingContext::computeStackingContextProperties(
    ComputeStackingContextContext& compositingState,
    StackingContext* ancestorLayer, bool& descendantHas3DTransform)
{
    auto oldMatrix = transformMatrix();
    computeTransformMatrix();
    if (oldMatrix != transformMatrix()) {
        m_catchedMatrixChangedWhileComputeStackingContextProperties = true;
    }

    // OverlapExtent layerExtent;
    // Use the fact that we're composited as a hint to check for an animating
    // transform.
    // FIXME: Maybe needsToBeComposited() should return a bitmask of reasons, to
    // avoid the need to recompute things.
    // if (willBeComposited && !layer.isRootLayer())
    //      layerExtent.hasTransformAnimation =
    //      isRunningTransformAnimation(layer.renderer());

    // bool respectTransforms = !layerExtent.hasTransformAnimation;
    // overlapMap.geometryMap().pushMappingsToAncestor(&layer, ancestorLayer,
    // respectTransforms);

    bool willBeComposited = m_owner->needsGraphicsBuffer() ||
                            m_owner->isRunningOpacityAnimation() ||
                            m_owner->isRunningTransformAnimation();
    IndirectCompositingReason compositingReason =
        compositingState.subLayerHasGraphicsBuffer
            ? IndirectCompositingReason::Stacking
            : IndirectCompositingReason::None;

#ifdef STARFISH_ENABLE_MULTIMEDIA
    if (m_owner->isFrameReplaced() &&
        m_owner->asFrameReplaced()->isFrameReplacedVideo()) {
        compositingReason = IndirectCompositingReason::Overlap;
    }
#endif

    if (!willBeComposited &&
        compositingReason == IndirectCompositingReason::Stacking &&
        compositingState.testingOverlap) {
        compositingReason =
            compositingState.isOverlapWithAlreadyCompositedLayer(this)
                ? IndirectCompositingReason::Overlap
                : IndirectCompositingReason::None;
    }

    // layer.setIndirectCompositingReason(compositingReason);

    // Check if the computed indirect reason will force the layer to become
    // composited.
    if (!willBeComposited && compositingReason &&
        canComposite(compositingState)) {
        willBeComposited = true;
    }

    // The children of this layer don't need to composite, unless there is
    // a compositing layer among them, so start by inheriting the compositing
    // ancestor with subtreeIsCompositing set to false.
    ComputeStackingContextContext childState(compositingState);
    childState.subLayerHasGraphicsBuffer = false;

    if (willBeComposited) {
        // Tell the parent it has compositing descendants.
        compositingState.subLayerHasGraphicsBuffer = true;
        // This layer now acts as the ancestor for kids.
        childState.compositingAncestor = this;

        compositingState.pushCompsitedLayer(this);
        // overlapMap.pushCompositingContainer();
        // This layer is going to be composited, so children can safely ignore
        // the fact that there's an
        // animation running behind this layer, meaning they can rely on the
        // overlap map testing again.
        childState.testingOverlap = true;

        // computeExtent(overlapMap, layer, layerExtent);

        // childState.ancestorHasTransformAnimation |=
        // layerExtent.hasTransformAnimation;
        // Too hard to compute animated bounds if both us and some ancestor is
        // animating transform.
        // layerExtent.animationCausesExtentUncertainty |=
        // layerExtent.hasTransformAnimation &&
        // compositingState.ancestorHasTransformAnimation;
    }

    bool anyDescendantHas3DTransform = false;

    auto iter = m_childContexts.begin();
    while (iter != m_childContexts.end()) {
        StackingContextChild* child = *iter;
        int32_t num = child->at(0)->zIndex();
        if (num >= 0) {
            break;
        }
        auto iter2 = child->begin();
        while (iter2 != child->end()) {
            (*iter2)->computeStackingContextProperties(
                childState, this, anyDescendantHas3DTransform);

            // If we have to make a layer for this child, make one now so we can
            // have a contents layer
            // (since we need to ensure that the -ve z-order child renders
            // underneath our contents).
            if (!willBeComposited && childState.subLayerHasGraphicsBuffer &&
                canComposite(compositingState)) {
                // make layer compositing
                // layer.setIndirectCompositingReason(RenderLayer::IndirectCompositingReason::BackgroundLayer);
                compositingReason = BackgroundLayer;
                childState.compositingAncestor = this;
                // overlapMap.pushCompositingContainer();
                compositingState.pushCompsitedLayer(this);
                // This layer is going to be composited, so children can safely
                // ignore the fact that there's an
                // animation running behind this layer, meaning they can rely on
                // the overlap map testing again
                childState.testingOverlap = true;
                willBeComposited = true;
            }
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
                (*iter2)->computeStackingContextProperties(
                    childState, this, anyDescendantHas3DTransform);
                iter2++;
            }
        }
        iter++;
    }

    // If we just entered compositing mode, the root will have become composited
    // (as long as accelerated compositing is enabled).
    if (isRootContext()) {
        // if (inCompositingMode() && m_hasAcceleratedCompositing)
        if (compositingState.compositedLayers->size()) {
            willBeComposited = true;
        }
    }

    if (childState.compositingAncestor &&
        !(childState.compositingAncestor->parent() == nullptr)) {
        // addToOverlapMap(overlapMap, layer, layerExtent);
        (*compositingState.overlapMapFilled.get()) = true;
    }

    // Now check for reasons to become composited that depend on the state of
    // descendant layers.
    IndirectCompositingReason indirectCompositingReason;
    if (!willBeComposited && canComposite(compositingState) &&
        requiresCompositingForIndirectReason(
            this, childState.subLayerHasGraphicsBuffer,
            anyDescendantHas3DTransform, indirectCompositingReason)) {
        // layer.setIndirectCompositingReason(indirectCompositingReason);
        childState.compositingAncestor = this;
        // overlapMap.pushCompositingContainer();
        compositingState.pushCompsitedLayer(this);
        // addToOverlapMapRecursive(overlapMap, layer);
        (*compositingState.overlapMapFilled.get()) = true;
        willBeComposited = true;
    }

    // ASSERT(willBeComposited == needsToBeComposited(layer));
    // if (layer.reflectionLayer()) {
    // FIXME: Shouldn't we call computeCompositingRequirements to handle a
    // reflection overlapping with another renderer?
    // layer.reflectionLayer()->setIndirectCompositingReason(willBeComposited ?
    // RenderLayer::IndirectCompositingReason::Stacking :
    // RenderLayer::IndirectCompositingReason::None);
    // }

    // Subsequent layers in the parent stacking context also need to composite.
    if (childState.subLayerHasGraphicsBuffer)
        compositingState.subLayerHasGraphicsBuffer = true;

    // Set the flag to say that this layer has compositing children.
    // layer.setHasCompositingDescendant(childState.subtreeIsCompositing);
    /*
        // setHasCompositingDescendant() may have changed the answer to
       needsToBeComposited() when clipping, so test that again.
        bool isCompositedClippingLayer = canBeComposited(layer) &&
       clipsCompositingDescendants(layer);

        // Turn overlap testing off for later layers if it's already off, or if
       we have an animating transform.
        // Note that if the layer clips its descendants, there's no reason to
       propagate the child animation to the parent layers. That's because
        // we know for sure the animation is contained inside the clipping
       rectangle, which is already added to the overlap map.
        if ((!childState.testingOverlap && !isCompositedClippingLayer) ||
       layerExtent.knownToBeHaveExtentUncertainty())
            compositingState.testingOverlap = false;

        if (isCompositedClippingLayer) {
            if (!willBeComposited) {
                childState.compositingAncestor = &layer;
                overlapMap.pushCompositingContainer();
                addToOverlapMapRecursive(overlapMap, layer);
                willBeComposited = true;
             }
        }
    */

    compositingState.compositeFlagInfo->insert(
        std::make_pair(this, willBeComposited));

    descendantHas3DTransform |=
        anyDescendantHas3DTransform ||
        (m_rareData ? m_rareData->m_hasNon2DRectTransform : false);

    if (willBeComposited) {
        SkMatrix l = SkMatrix::I();
        LayoutRect visibleRect(0, 0, 0, 0);
        Frame::ComputeVisibleRectContext ctx(
            Frame::ComputeVisibleRectContext::GraphicsBuffer, this, l,
            visibleRect);
        m_owner->computeVisibleRect(ctx);

        if (visibleRect.isEmpty()) {
            compositingState.seenPossiblyNonCompositeLayersNow->push_back(this);
        }
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
    bool compositedBefore = needsGraphicsBuffer();
    bool willBeComposited = (*ctx.compositeFlagInfo)[this];

    if (inAnimation && compositedBefore && !willBeComposited) {
        willBeComposited = true;
    }

    if (m_rareData) {
        m_rareData->m_visibleRect = LayoutRect(0, 0, 0, 0);
    }

    if (willBeComposited) {
        ensureRareData();

        SkMatrix l = SkMatrix::I();
        Frame::ComputeVisibleRectContext ctx(
            Frame::ComputeVisibleRectContext::GraphicsBuffer, this, l,
            m_rareData->m_visibleRect);

        m_owner->computeVisibleRect(ctx);

        if (m_rareData->m_visibleRect.width() &&
            m_rareData->m_visibleRect.height()) {
            // TODO implement sub-visible rect painting & compositing
            LayoutRect visibleRect = m_owner->frameVisibleRect();
            if (m_rareData->m_visibleRect.width() < visibleRect.width() ||
                m_rareData->m_visibleRect.height() < visibleRect.height()) {
                m_rareData->m_visibleRect.setWidth(std::max(
                    visibleRect.width(), m_rareData->m_visibleRect.width()));
                m_rareData->m_visibleRect.setHeight(std::max(
                    visibleRect.height(), m_rareData->m_visibleRect.height()));
            }
        } else if (!isRootContext() && !inAnimation) {
            willBeComposited = false;
        }
    } else {
        if (m_rareData) {
            m_rareData->m_isVisibleRectComputedForNonGraphicsLayer = false;
        }
    }

    if (compositedBefore != willBeComposited) {
        if (compositedBefore) {
            StackingContext* p = m_parent;
            while (p) {
                if (p->needsGraphicsBuffer()) {
                    break;
                }
                p = p->parent();
            }
            if (!p) {
                p = this;
            }
            p->m_owner->node()->setNeedsPainting();
        } else {
            m_owner->node()->setNeedsPainting();
        }
    } else if (compositedBefore && compositedBefore == willBeComposited) {
        m_owner->node()->webView()->markNeedsCompositeConsiderInRendering();
    } else if (!compositedBefore && !willBeComposited) {
        if (m_catchedMatrixChangedWhileComputeStackingContextProperties) {
            m_owner->node()->setNeedsPainting();
        }
    }

    m_catchedMatrixChangedWhileComputeStackingContextProperties = false;
    if (willBeComposited) {
        ensureRareData()->m_needsGraphicsBuffer = true;
    } else {
        if (m_rareData) {
            m_rareData->m_needsGraphicsBuffer = false;
        }
    }
}

class FlagRestorer {
public:
    FlagRestorer(bool& flag)
        : m_target(flag)
        , m_initialValue(flag)
    {
    }

    ~FlagRestorer()
    {
        m_target = m_initialValue;
    }

    bool& m_target;
    bool m_initialValue;
};

void StackingContext::paintStackingContext(
    Canvas* canvas, bool needsPainting, bool parentGraphicsLayerNeedsPainting)
{
    FlagRestorer needsPaintingFlagRestorer(needsPainting);

    Canvas* oldCanvas = nullptr;
    LayoutRect visibleRect = StackingContext::visibleRect();
    LayoutUnit minX = visibleRect.x();
    LayoutUnit maxX = visibleRect.maxX();
    LayoutUnit minY = visibleRect.y();
    LayoutUnit maxY = visibleRect.maxY();

    minX = minX.floor();
    maxX = maxX.ceil();
    minY = minY.floor();
    maxY = maxY.ceil();

    size_t bufferWidth =
        (int)(maxX - minX) * m_owner->node()->window()->devicePixelRatio();
    size_t bufferHeight =
        (int)(maxY - minY) * m_owner->node()->window()->devicePixelRatio();

    bool hasGraphicsBuffer = needsGraphicsBuffer();

    if (hasGraphicsBuffer) {
        needsPainting = m_needsRepainting || parentGraphicsLayerNeedsPainting;
        if (m_needsRepainting || m_rareData->m_buffer == nullptr) {
            parentGraphicsLayerNeedsPainting = true;
            if (m_owner->hasOwnGraphicsBufferMethod()) {
                m_owner->createGraphicsBuffer(&m_rareData->m_buffer,
                                              bufferWidth, bufferHeight);
            } else {
                m_owner->node()->webView()->assignGraphicsBuffer(
                    &m_rareData->m_buffer, bufferWidth, bufferHeight);
            }
        }

        oldCanvas = canvas;
        if (m_rareData->m_buffer->pixelRatio() != 1) {
            canvas = Canvas::createGenericCanvas(
                m_owner->node()->starFish(), m_rareData->m_buffer->data(),
                m_rareData->m_buffer->bufferWidth(),
                m_rareData->m_buffer->bufferHeight());
        } else {
            canvas = Canvas::create(m_owner->node()->starFish(),
                                    m_rareData->m_buffer);
        }

        if (oldCanvas) {
            canvas->setTextDecorationData(oldCanvas->textDecorationData());
        }
        if (needsPainting) {
            canvas->clearColor(Unit::Color(0, 0, 0, 0));
        }
        if (m_rareData->m_buffer->pixelRatio() != 1) {
            canvas->scale(1.0 / m_rareData->m_buffer->pixelRatio(),
                          1.0 / m_rareData->m_buffer->pixelRatio());
        }
        canvas->translate(-minX, -minY);
    } else {
        clearGraphicsBuffer(false);
    }

    {
        // draw debug rect
        // canvas->save();
        // canvas->setColor(Color(0, 0, 255, 64));
        // canvas->drawRect(visibleRect);
        // canvas->restore();
    }

    canvas->save();

    std::unique_ptr<CanvasStateRestorer> canvasStateRestorerForFixedLayer;
    if (m_owner->style()->position() == PositionValue::FixedPositionValue) {
        canvasStateRestorerForFixedLayer.reset(
            new CanvasStateRestorer(canvas, this, parent()->owner()));
    }

    if (!hasGraphicsBuffer && owner()->style()->opacity() != 1) {
        canvas->beginOpacityLayer(owner()->style()->opacity());
    }

    if (!hasGraphicsBuffer) {
        SkMatrix m = transformMatrix();

        if (!m.isIdentity()) {
            SkMatrix test;
            bool testResult = m_rareData->m_matrix.invert(&test);
            if (!testResult) {
                // ignorePaintingDueToInvalidMatrix
                if (!hasGraphicsBuffer && owner()->style()->opacity() != 1) {
                    canvas->endOpacityLayer();
                }
                canvas->restore();
                if (hasGraphicsBuffer) {
                    delete canvas;
                }
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
    if (hasGraphicsBuffer) {
        canRejectPainting =
            canvas->canRejectPainting(StackingContext::visibleRect());
    } else if (owner()->shouldApplyOverflow()) {
        canRejectPainting =
            canvas->canRejectPainting(m_owner->frameVisibleRect());
    } else {
        if (!ensureRareData()->m_isVisibleRectComputedForNonGraphicsLayer) {
            ensureRareData()->m_visibleRect = m_owner->frameVisibleRect();
            SkMatrix l = SkMatrix::I();
            Frame::ComputeVisibleRectContext ctx(
                Frame::ComputeVisibleRectContext::Scrolling, owner(), l,
                ensureRareData()->m_visibleRect);

            m_owner->computeVisibleRect(ctx);

            ensureRareData()->m_isVisibleRectComputedForNonGraphicsLayer = true;
        }

        canRejectPainting =
            canvas->canRejectPainting(StackingContext::visibleRect());
    }

    if (needsPainting && !canRejectPainting) {
        m_owner->paintBackgroundAndBorders(canvas);
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
        canvas->translate(iframeBox->borderLeft() + iframeBox->paddingLeft(),
                          iframeBox->borderTop() + iframeBox->paddingTop());
        if (needsPainting) {
            m_owner->node()
                ->document()
                ->browsingContext()
                ->paintWindowBackground(canvas);
        }
    }

    if (!hasGraphicsBuffer && owner()->shouldApplyOverflow()) {
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
                CanvasStateRestorer r(canvas, sCtx, m_owner);
                sCtx->paintStackingContext(canvas, needsPainting,
                                           parentGraphicsLayerNeedsPainting);
                iter2++;
            }
            iter++;
        }
    }

    if (needsPainting && !canRejectPainting) {
        m_owner->paintStackingContextContent(canvas);
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
                    CanvasStateRestorer r(canvas, sCtx, m_owner);
                    sCtx->paintStackingContext(
                        canvas, needsPainting,
                        parentGraphicsLayerNeedsPainting);
                    iter2++;
                }
            }
            iter++;
        }
    }

    if (!hasGraphicsBuffer && owner()->style()->opacity() != 1) {
        canvas->endOpacityLayer();
    }

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
            if (needsPainting) {
                m_owner->node()
                    ->document()
                    ->browsingContext()
                    ->window()
                    ->scrolling()
                    ->paintScrollbars(canvas, document,
                                      document->appliedOverflowX(),
                                      document->appliedOverflowY());
            }
            canvas->restore();
        }
    }

    canvasStateRestorerForFixedLayer.reset(nullptr);

    canvas->restore();
    if (hasGraphicsBuffer) {
        delete canvas;
    }
    if (hasGraphicsBuffer || isRootContext()) {
        m_needsRepainting = false;
    }
}

void StackingContext::compositeStackingContext(Compositor* compositor)
{
    LayoutRect visibleRect = StackingContext::visibleRect();
    ComputedStyle* ownerStyle = m_owner->style();
    compositor->save();

    if (isIFrameStackingContext()) {
        compositor->clip(owner()->makeRect(BoxValue::PaddingBoxBoxValue));
        compositor->translate(
            -m_owner->layoutParent()->asFrameDocument()->scrollLeft(),
            -m_owner->layoutParent()->asFrameDocument()->scrollTop());
    }

    SkMatrix m = transformMatrix();
    if (!m.isIdentity()) {
        SkMatrix test;
        bool testResult = m_rareData->m_matrix.invert(&test);
        if (!testResult) {
            // ignorePaintingDueToInvalidMatrix
            compositor->restore();
            return;
        }

        LayoutLocation to = transformOrigin();
        compositor->translate(to.x(), to.y());
        compositor->postMatrix(m);
        compositor->translate(-to.x(), -to.y());
    }

    if (ownerStyle->opacity() != 1) {
        compositor->beginOpacityLayer(ownerStyle->opacity());
    }

    if (needsGraphicsBuffer()) {
        LayoutUnit minX = visibleRect.x();
        LayoutUnit maxX = visibleRect.maxX();
        LayoutUnit minY = visibleRect.y();
        LayoutUnit maxY = visibleRect.maxY();

        minX = minX.floor();
        maxX = maxX.ceil();
        minY = minY.floor();
        maxY = maxY.ceil();

        size_t bufferWidth = (int)(maxX - minX);
        size_t bufferHeight = (int)(maxY - minY);

        if (owner()->shouldApplyOverflow()) {
            compositor->clip(
                Unit::Rect(0, 0, owner()->width(), owner()->height()));
            if (m_owner->isFrameBlockBox()) {
                compositor->translate(-m_owner->asFrameBlockBox()->scrollLeft(),
                                      -m_owner->asFrameBlockBox()->scrollTop());
            }
        }

        if (bufferWidth && bufferHeight) {
            owner()->willCompsiteStackingContext(compositor);
            compositor->drawSurface(
                m_rareData->m_buffer,
                Unit::Rect(minX, minY, bufferWidth, bufferHeight));
            owner()->didCompsiteStackingContext(compositor);
        }
        // draw debug rect
        // canvas->setColor(Color(255, 0, 0, 128));
        // canvas->drawRect(Rect(minX, minY, bufferWidth, bufferHeight));
    } else {
        if (owner()->shouldApplyOverflow()) {
            compositor->clip(owner()->makeRect(BoxValue::BorderBoxBoxValue));
            if (m_owner->isFrameBlockBox()) {
                compositor->translate(-m_owner->asFrameBlockBox()->scrollLeft(),
                                      -m_owner->asFrameBlockBox()->scrollTop());
            }
        }
        owner()->compsitingStackingContext(compositor);
    }

    // Within each stacking context, the following layers are painted in
    // back-to-front order:

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
                compositor->save();

                {
                    CompositorStateRestorer r(compositor, sCtx, m_owner);
                    sCtx->compositeStackingContext(compositor);
                }

                compositor->restore();
                iter2++;
            }
            iter++;
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
                    compositor->save();

                    {
                        CompositorStateRestorer r(compositor, sCtx, m_owner);
                        sCtx->compositeStackingContext(compositor);
                    }

                    compositor->restore();
                    iter2++;
                }
            }
            iter++;
        }
    }

    if (ownerStyle->opacity() != 1) {
        compositor->endOpacityLayer();
    }

    compositor->restore();
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
}
