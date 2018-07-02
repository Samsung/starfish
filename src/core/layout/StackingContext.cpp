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
    std::unordered_map<StackingContext*, LayoutRect> extentPerLayer;
    std::unordered_map<StackingContext*, bool> compositeFlagInfo;
    std::vector<StackingContext*> compositedLayers;
    StackingContext* rootLayer;

    ComputeStackingContextContext(StackingContext* rootLayer)
        : rootLayer(rootLayer)
    {
    }

    LayoutRect computeLayerExtent(LayoutRect rt, const SkMatrix& m)
    {
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
        {
            auto iter = extentPerLayer.find(c);
            if (iter != extentPerLayer.end()) {
                return iter->second;
            }
        }

        std::vector<FrameBox*> frameList;
        frameList.reserve(32);
        Frame* f = c->owner();
        while (f) {
            frameList.push_back(f->asFrameBox());
            f = f->layoutParent();
        }

        LayoutRect vr;
        if (c->isRootContext()) {
            vr = LayoutRect(0, 0, c->owner()->asFrameBlockBox()->scrollWidth(),
                            c->owner()->asFrameBlockBox()->scrollHeight());
        } else {
            vr = c->owner()->frameVisibleRect();
        }

        SkMatrix m = SkMatrix::I();
        auto iter = frameList.rbegin();
        FrameBox* lastParentBox = nullptr;
        while (iter != frameList.rend()) {
            FrameBox* fBox = *iter;
            if (fBox->stackingContext()) {
                SkMatrix m2 = fBox->stackingContext()->transformMatrix();
                if (!m2.isIdentity()) {
                    LayoutLocation to =
                        fBox->stackingContext()->transformOrigin();
                    m.postTranslate((float)to.x(), (float)to.y());
                    m.preConcat(m2);
                    m.postTranslate(-(float)to.x(), -(float)to.y());
                }
            }

            LayoutLocation pos;
            if (fBox == c->owner() || fBox->isFrameDocument()) {
                pos = fBox->absolutePoint(lastParentBox);
            } else {
                pos = fBox->absolutePointIncludingScroll(lastParentBox);
            }
            m.postTranslate((float)pos.x(), (float)pos.y());
            lastParentBox = fBox;
            iter++;
        }

        LayoutRect rt = computeLayerExtent(vr, m);
        extentPerLayer.insert(std::make_pair(c, rt));

        return rt;
    }

    void pushCompsitedLayer(StackingContext* c)
    {
        compositedLayers.push_back(c);
    }

    bool isCompsitedLayer(StackingContext* c, size_t* idx = nullptr)
    {
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

    bool isOverlapWithAlreadyCompositedLayer(StackingContext* a)
    {
        auto extentA = screenExtentPerLayer(a);
        for (size_t i = 0; i < compositedLayers.size(); i++) {
            auto extentB = screenExtentPerLayer(compositedLayers.at(i));
            if (extentA.intersects(extentB)) {
                return true;
            }
        }
        return false;
    }
};

StackingContextRareData::StackingContextRareData()
    : m_visibleRect(0, 0, 0, 0)
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
    , m_needsGraphicsBuffer(false)
    , m_hasNon2DRectTransform(false)
    , m_isVisibleRectComputedForNonGraphicsLayer(false)
    , m_needsGraphicsBufferReason(
          NeedsGraphicsLayerReason::NeedsGraphicsLayerReasonNone)
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

        if (m_seenAbsBlock) {
            return false;
        }
        m_seenAbsBlock = m_seenAbsBlock || parent->isAbsolutePositioned();

        if (m_child->isAbsolutePositioned()) {
            if (m_seenContainingBlockForAbsBlock) {
                return parent->shouldApplyOverflow();
            }
            bool b =
                parent->canBeContainingBlockOfAbsolutePositionedBox(m_child);
            m_seenContainingBlockForAbsBlock =
                m_seenContainingBlockForAbsBlock || b;
            return b && parent->shouldApplyOverflow();
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
                     (!s->isEstablishesStackingContext() ||
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
                LayoutRect visibleRect = sc->visibleRect();
                LayoutUnit minX = visibleRect.x();
                LayoutUnit minY = visibleRect.y();

                minX = minX.floor();
                minY = minY.floor();

                canvas->translate(-minX, -minY);
                break;
            }
            sc = sc->parent();
        }

        auto iter = frameList.rbegin();
        shareWithStackingBuffer = nearstBufferedFrame == nullptr;
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
    {
        compositor->save();
        FrameBox* self = sCtx->owner();
        std::vector<FrameBox*> frameList;

        frameList.reserve(32);
        m_canApplyOverflowOrScrolls.reserve(32);

        {
            Frame* f = self;
            OverflowStatus status(f);
            bool canScroll =
                status.m_child->style()->position() != FixedPositionValue;

            while (f) {
                frameList.push_back(f->asFrameBox());

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

                if (canScroll) {
                    if (f && f->style() &&
                        f->style()->position() == FixedPositionValue) {
                        canScroll = false;
                    }
                }

                f = f->layoutParent();
            }
        }

        compositor->resetMatrixAndClip();
        auto iter = frameList.rbegin();
        while (iter != frameList.rend()) {
            FrameBox* b = *iter;
            StackingContext* ctx = b->stackingContext();
            compositor->translate(b->x(), b->y());
            if (b->style()) {
                auto overflowOrScroll = readFromCanApplyOverflowOrScrolls(b);

                if (ctx) {
                    SkMatrix m = ctx->transformMatrix();
                    if (!m.isIdentity()) {
                        auto o = ctx->transformOrigin();
                        compositor->translate(o.x(), o.y());
                        compositor->postMatrix(m);
                        compositor->translate(-o.x(), -o.y());
                    }
                }

                if (overflowOrScroll.first) {
                    Unit::Rect rt(b->borderLeft(), b->borderTop(),
                                  b->width() - b->borderWidth(),
                                  b->height() - b->borderHeight());
                    compositor->clip(rt);
                }

                if (b->isAbsolutePositioned()) {
                    RectData* rect = b->style()->clip();
                    if (rect) {
                        compositor->clip(Unit::Rect(
                            rect->left().numberData(), rect->top().numberData(),
                            rect->right().numberData(),
                            rect->bottom().numberData()));
                    }
                }

                if (overflowOrScroll.second) {
                    compositor->translate(-b->asFrameBlockBox()->scrollLeft(),
                                          -b->asFrameBlockBox()->scrollTop());
                }
            }

            iter++;
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

        m_hasNon2DRectTransform = m_owner->style()->has3DTransforms(m_owner) ||
                                  !m_rareData->m_matrix.rectStaysRect();

#ifdef PORT_CANVAS_BACKEND_EFL
        // force use graphics buffer with complex-transform
        // because efl canvas can't deal well with complex-transform
        m_hasNon2DRectTransform =
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

void StackingContext::computeStackingContextProperties()
{
    STARFISH_ASSERT(isRootContext());

    ComputeStackingContextContext ctx(this);
    computeStackingContextProperties(ctx);
    applyStackingContextProperties(ctx);
}

void StackingContext::computeStackingContextProperties(
    ComputeStackingContextContext& compositingState)
{
    auto oldMatrix = transformMatrix();
    computeTransformMatrix();
    if (oldMatrix != transformMatrix()) {
        m_catchedMatrixChangedWhileComputeStackingContextProperties = true;
    }

    NeedsGraphicsLayerReason reason =
        NeedsGraphicsLayerReason::NeedsGraphicsLayerReasonNone;
    bool compositedBySelf = m_owner->needsGraphicsBuffer() ||
                            m_owner->isRunningOpacityAnimation() ||
                            m_owner->isRunningTransformAnimation();
    if (compositedBySelf) {
        reason = NeedsGraphicsLayerReason::NeedsGraphicsLayerReasonBySelf;
    }
    bool willBeComposited = compositedBySelf;

    if (!willBeComposited && compositingState.seenCompsitedLayer()) {
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

        auto parentExtent =
            compositingState.screenExtentPerLayer(compositedAncestor);
        auto selfExtent = compositingState.screenExtentPerLayer(this);

        if (parentExtent.containsInVisual(selfExtent.x(), selfExtent.y()) &&
            parentExtent.containsInVisual(selfExtent.maxX(), selfExtent.y()) &&
            parentExtent.containsInVisual(selfExtent.x(), selfExtent.maxY()) &&
            parentExtent.containsInVisual(selfExtent.maxX(),
                                          selfExtent.maxY())) {
            canConveredByParentCompositedLayer = true;
        } else {
            reason = NeedsGraphicsLayerReason::
                NeedsGraphicsLayerReasonNotCoveredByParent;
        }

        if (canConveredByParentCompositedLayer) {
            auto& cv = compositingState.compositedLayers;
            for (size_t i = ancestorIndex + 1; i < cv.size(); i++) {
                auto extent = compositingState.screenExtentPerLayer(cv[i]);
                if (extent.intersects(selfExtent)) {
                    isCollapsedWithSilbingLayer = true;
                    reason = NeedsGraphicsLayerReason::
                        NeedsGraphicsLayerReasonCollapsedWithSiblingLayer;
                    break;
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
        StackingContext* p = parent();
        while (p) {
            if (p->isRootContext() && !compositingState.isCompsitedLayer(p)) {
                STARFISH_ASSERT(compositingState.compositedLayers.size() == 0);
                compositingState.pushCompsitedLayer(p);
                break;
            }
            p = p->parent();
        }
        compositingState.pushCompsitedLayer(this);
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

    if (isRootContext()) {
        if (compositingState.compositedLayers.size()) {
            willBeComposited = true;
            m_needsGraphicsBufferReason =
                NeedsGraphicsLayerReason::NeedsGraphicsLayerReasonBySelf;
        }
    }

    compositingState.compositeFlagInfo.insert(
        std::make_pair(this, willBeComposited));
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
    bool willBeComposited = ctx.compositeFlagInfo[this];

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
        m_isVisibleRectComputedForNonGraphicsLayer = false;
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
        m_needsGraphicsBuffer = true;
    } else {
        m_needsGraphicsBuffer = false;
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

    size_t bufferWidth = (int)(maxX - minX);
    size_t bufferHeight = (int)(maxY - minY);

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
        if (!m_isVisibleRectComputedForNonGraphicsLayer) {
            ensureRareData()->m_visibleRect = m_owner->frameVisibleRect();
            SkMatrix l = SkMatrix::I();
            Frame::ComputeVisibleRectContext ctx(
                Frame::ComputeVisibleRectContext::Scrolling, owner(), l,
                ensureRareData()->m_visibleRect);

            m_owner->computeVisibleRect(ctx);

            m_isVisibleRectComputedForNonGraphicsLayer = true;
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
        auto clipRect = iframeBox->makeRect(BoxValue::PaddingBoxBoxValue);
        HTMLIFrameElement* iframe =
            m_owner->node()->document()->browsingContext()->sourceElement();
        clipRect.setX(clipRect.x() +
                      m_owner->node()
                          ->document()
                          ->browsingContext()
                          ->window()
                          ->scrollX());
        clipRect.setY(clipRect.y() +
                      m_owner->node()
                          ->document()
                          ->browsingContext()
                          ->window()
                          ->scrollY());
        canvas->clip(clipRect);
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

    SkMatrix m = transformMatrix();
    if (!m.isIdentity()) {
        SkMatrix test;
        bool testResult = m_rareData->m_matrix.invert(&test);
        if (!testResult) {
            // ignorePaintingDueToInvalidMatrix
            compositor->restore();
            return;
        }
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

        if (bufferWidth && bufferHeight) {
            owner()->willCompsiteStackingContext(compositor);
#ifdef STARFISH_ENABLE_TEST
            if (owner()->node()->starFish()->startUpFlag() &
                StarFishStartUpFlag::enableDebugGraphicsLayer) {
                // debug compositing method
                switch (m_needsGraphicsBufferReason) {
                case NeedsGraphicsLayerReasonNone:
                    compositor->setColor(Unit::Color(255, 64, 0, 64));
                    break;
                case NeedsGraphicsLayerReasonBySelf:
                    compositor->setColor(Unit::Color(255, 0, 0, 64));
                    break;
                case NeedsGraphicsLayerReasonNotCoveredByParent:
                    compositor->setColor(Unit::Color(0, 255, 0, 64));
                    break;
                case NeedsGraphicsLayerReasonCollapsedWithSiblingLayer:
                    compositor->setColor(Unit::Color(0, 0, 255, 64));
                    break;
                default:
                    STARFISH_RELEASE_ASSERT_NOT_REACHED();
                }
                compositor->drawRect(
                    Unit::Rect(minX, minY, bufferWidth, bufferHeight));
                compositor->beginOpacityLayer(0.15);
                compositor->drawSurface(
                    m_rareData->m_buffer,
                    Unit::Rect(minX, minY, bufferWidth, bufferHeight));
                compositor->endOpacityLayer();
            } else {
                compositor->drawSurface(
                    m_rareData->m_buffer,
                    Unit::Rect(minX, minY, bufferWidth, bufferHeight));
            }
#else
            compositor->drawSurface(
                m_rareData->m_buffer,
                Unit::Rect(minX, minY, bufferWidth, bufferHeight));
#endif
            owner()->didCompsiteStackingContext(compositor);
        }
        // draw debug rect
        // canvas->setColor(Color(255, 0, 0, 128));
        // canvas->drawRect(Rect(minX, minY, bufferWidth, bufferHeight));
    } else {
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
