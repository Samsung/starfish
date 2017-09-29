/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"

#include "core/layout/StackingContext.h"

#include "StarFish.h"
#include "core/dom/Node.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameReplaced.h"
#include "core/page/BrowsingContext.h"
#include "core/modules/canvas/Canvas.h"
#include "core/page/Window.h"
#include "platform/window/PlatformWindow.h"

namespace StarFish {

StackingContextRareData::StackingContextRareData()
    : m_needsOwnBuffer(false)
    , m_visibleRect(0, 0, 0, 0)
    , m_buffer(nullptr)
{
}

void* StackingContextRareData::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(StackingContextRareData)] = { 0 };
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(StackingContextRareData, m_buffer));
        descr = GC_make_descriptor(obj_bitmap,
                                   GC_WORD_LEN(StackingContextRareData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

StackingContext::StackingContext(FrameBox* owner, StackingContext* parent)
    : m_rareData(nullptr)
{
    m_owner = owner;
    m_parent = parent;
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
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(StackingContext)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(StackingContext, m_rareData));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(StackingContext, m_owner));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(StackingContext, m_parent));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(StackingContext, m_childContexts));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(StackingContext));
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

VisibleRectContext::VisibleRectContext(FrameBox* box, LayoutLocation* loc)
    : m_box(box)
    , m_loc(loc)
{
    m_loc->setX(m_loc->x() + m_box->x());
    m_loc->setY(m_loc->y() + m_box->y());
}

VisibleRectContext::~VisibleRectContext()
{
    m_loc->setX(m_loc->x() - m_box->x());
    m_loc->setY(m_loc->y() - m_box->y());
}

void StackingContext::clearOwnBuffer(bool needsDetachNative)
{
    if (m_rareData && m_rareData->m_buffer) {
        if (needsDetachNative) {
            m_rareData->m_buffer->detachNativeBuffer();
        }
        m_rareData->m_buffer = nullptr;
    }
}

class CanvasStateRestorer {
private:
    std::unordered_map<Frame*, std::pair<bool, bool>>
        m_canApplyOverflowOrScrolls;
    struct OverflowStatus {
        Frame* m_child;

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
                return parent->canBeContainingBlockOfAbsolutePositionedBox(
                           m_child) &&
                       parent->shouldApplyOverflow();
            }

            return parent->shouldApplyOverflow();
        }

        void reset(Frame* f)
        {
            m_child = f;
        }
    };

public:
    CanvasStateRestorer(Canvas* canvas, StackingContext* sCtx, FrameBox* owner,
                        bool isCompositing)
        : m_canvas(canvas)
    {
        canvas->save();

        FrameBox* self = sCtx->owner();
        if (isCompositing) {
            LayoutLocation l = self->absolutePoint(owner);
            canvas->translate(l.x(), l.y());

            if (self->style()->position() == FixedPositionValue) {
                Frame* parent = self->layoutParent();
                LayoutUnit offsetX = self->x(), offsetY = self->y();
                while (parent->isLineBox() ||
                       !parent->canBeContainingBlockOfAbsolutePositionedBox(
                           self)) {
                    offsetX += parent->asFrameBox()->x();
                    offsetY += parent->asFrameBox()->y();
                    parent = parent->layoutParent();
                }

                if (parent->isFrameBlockBox()) {
                    canvas->translate(parent->asFrameBlockBox()->scrollLeft(),
                                      parent->asFrameBlockBox()->scrollTop());
                }
            }
            return;
        }

        std::vector<FrameBox*> frameList;
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
                    f->asFrameBox()->stackingContext()->needsOwnBuffer()) {
                    nearstBufferedFrame = f;
                    shareWithStackingBuffer = false;
                }

                if (shareWithStackingBuffer) {
                    if (status.canApplyOverflow(f)) {
                        m_canApplyOverflowOrScrolls[f] = std::make_pair(
                            true, canScroll && f && f->isFrameBlockBox());
                        status.reset(f);
                        canScroll = status.m_child->style()->position() !=
                                    FixedPositionValue;
                    } else {
                        m_canApplyOverflowOrScrolls[f] = std::make_pair(
                            false, canScroll && f && f->isFrameBlockBox());
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
            if (sc->needsOwnBuffer()) {
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
                canvas->translate(b->x(), b->y());
            }

            if (b->style()) {
                auto overflowOrScroll = m_canApplyOverflowOrScrolls[b];

                if (b != self) {
                    if (b != nearstBufferedFrame) {
                        SkMatrix m = b->style()->transformsToMatrix(
                            b->width(), b->height(), b,
                            b->style()->hasTransforms(b));
                        if (!m.isIdentity()) {
                            LayoutUnit ox = b->width() / 2;
                            LayoutUnit oy = b->height() / 2;
                            if (b->style()->hasTransformOrigin()) {
                                ox = b->style()
                                         ->transformOrigin()
                                         ->originValue()
                                         ->getXAxis()
                                         .specifiedValue(b->width(), b);
                                oy = b->style()
                                         ->transformOrigin()
                                         ->originValue()
                                         ->getYAxis()
                                         .specifiedValue(b->height(), b);
                            }
                            canvas->translate(ox, oy);
                            canvas->postMatrix(m);
                            canvas->translate(-ox, -oy);
                        }
                    }

                    if (overflowOrScroll.first) {
                        canvas->clip(
                            Unit::Rect(b->borderLeft(), b->borderTop(),
                                       b->width() - b->borderWidth(),
                                       b->height() - b->borderHeight()));
                    }

                    if (overflowOrScroll.second) {
                        canvas->translate(-b->asFrameBlockBox()->scrollLeft(),
                                          -b->asFrameBlockBox()->scrollTop());
                    }
                }
            }

            iter++;
        }
    }
    ~CanvasStateRestorer()
    {
        m_canvas->restore();
    }

    Canvas* m_canvas;
};

bool StackingContext::computeStackingContextProperties(bool forceNeedsBuffer)
{
    bool childNeedsBuffer = false;
    auto iter = m_childContexts.begin();
    while (iter != m_childContexts.end()) {
        StackingContextChild* child = *iter;
        auto iter2 = child->begin();
        while (iter2 != child->end()) {
            childNeedsBuffer |=
                (*iter2)->computeStackingContextProperties(childNeedsBuffer);
            iter2++;
        }
        iter++;
    }

    if (m_rareData) {
        m_rareData->m_matrix.reset();
    }
    if (forceNeedsBuffer || childNeedsBuffer ||
        m_owner->needsGraphicsBuffer()) {
        ensureRareData()->m_needsOwnBuffer = true;
    } else {
        if (m_rareData) {
            m_rareData->m_needsOwnBuffer = false;
        }
    }

    if (needsOwnBuffer()) {
        LayoutLocation l(-m_owner->frameRect().location().x(),
                         -m_owner->frameRect().location().y());

        m_rareData->m_visibleRect = LayoutRect(0, 0, 0, 0);
        m_owner->computeVisibleRect(this, l, m_rareData->m_visibleRect);
    }

    return needsOwnBuffer();
}

void StackingContext::paintStackingContext(Canvas* canvas)
{
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

    bool hasStackingBuffer = needsOwnBuffer();

    if (hasStackingBuffer) {
        if (!m_rareData->m_buffer ||
            ((m_rareData->m_buffer->width() != bufferWidth) &&
             (m_rareData->m_buffer->height() != bufferHeight))) {
            if (m_rareData->m_buffer) {
                m_rareData->m_buffer->detachNativeBuffer();
            }
            m_rareData->m_buffer = CanvasSurface::create(
                m_owner->node()->window()->starFish()->platformWindow(),
                bufferWidth, bufferHeight);
        }

        m_rareData->m_buffer->clear();
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
        canvas->setTextDecorationData(oldCanvas->textDecorationData());
        if (m_rareData->m_buffer->pixelRatio() != 1) {
            canvas->scale(1.0 / m_rareData->m_buffer->pixelRatio(),
                          1.0 / m_rareData->m_buffer->pixelRatio());
        }
        canvas->translate(-minX, -minY);
    } else {
        if (m_rareData && m_rareData->m_buffer) {
            m_rareData->m_buffer->detachNativeBuffer();
            m_rareData->m_buffer = nullptr;
        }
    }

    {
        // draw debug rect
        // canvas->save();
        // canvas->setColor(Color(0, 0, 255, 64));
        // canvas->drawRect(visibleRect);
        // canvas->restore();
    }

    canvas->save();

    if (!hasStackingBuffer && owner()->style()->opacity() != 1) {
        canvas->beginOpacityLayer(owner()->style()->opacity());
    }

    if (!hasStackingBuffer) {
        SkMatrix m = m_owner->style()->transformsToMatrix(
            m_owner->width(), m_owner->height(), m_owner,
            m_owner->style()->hasTransforms(m_owner));

        if (!m.isIdentity()) {
            ensureRareData()->m_matrix = m;
            /*STARFISH_LOG_INFO("matrix [%f %f %f][%f %f %f][%f %f %f]\n",
                              m_rareData->m_matrix.getScaleX(),
                              m_rareData->m_matrix.getSkewX(),
                              m_rareData->m_matrix.getTranslateX(),
                              m_rareData->m_matrix.getSkewY(),
                              m_rareData->m_matrix.getScaleY(),
                              m_rareData->m_matrix.getTranslateY(),
                              m_rareData->m_matrix.getPerspX(),
                              m_rareData->m_matrix.getPerspY(),
                              m_rareData->m_matrix.get(8));*/
            LayoutUnit ox = m_owner->width() / 2;
            LayoutUnit oy = m_owner->height() / 2;
            if (m_owner->style()->hasTransformOrigin()) {
                ox = m_owner->style()
                         ->transformOrigin()
                         ->originValue()
                         ->getXAxis()
                         .specifiedValue(m_owner->width(), m_owner);
                oy = m_owner->style()
                         ->transformOrigin()
                         ->originValue()
                         ->getYAxis()
                         .specifiedValue(m_owner->height(), m_owner);
            }
            SkMatrix test;
            bool testResult = m_rareData->m_matrix.invert(&test);
            if (!testResult) {
                // ignorePaintingDueToInvalidMatrix
                if (!hasStackingBuffer && owner()->style()->opacity() != 1) {
                    canvas->endOpacityLayer();
                }
                canvas->restore();
                if (hasStackingBuffer) {
                    delete canvas;
                }
                return;
            } else {
            }
            canvas->translate(ox, oy);
            canvas->postMatrix(m_rareData->m_matrix);
            canvas->translate(-ox, -oy);
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
    // Within each stacking context, the following layers are painted in
    // back-to-front order:
    // the background and borders of the element forming the stacking context.
    if (m_owner->layoutParent()->isFrameDocument()) {
        if (!m_owner->node()
                 ->document()
                 ->browsingContext()
                 ->isMainBrowsingContext()) {
            FrameBlockBox* document =
                m_owner->layoutParent()->asFrameBlockBox();
            canvas->translate(document->scrollLeft(), document->scrollTop());
            m_owner->node()
                ->document()
                ->browsingContext()
                ->paintWindowBackground(canvas);
            canvas->translate(-document->scrollLeft(), -document->scrollTop());
        }
    }
    m_owner->paintBackgroundAndBorders(canvas);

    if (!hasStackingBuffer && owner()->shouldApplyOverflow()) {
        canvas->clip(owner()->makeRect(BoxValue::PaddingBoxBoxValue));
        if (m_owner->isFrameBlockBox())
            canvas->translate(-m_owner->asFrameBlockBox()->scrollLeft(),
                              -m_owner->asFrameBlockBox()->scrollTop());
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
                canvas->save();

                {
                    CanvasStateRestorer r(canvas, sCtx, m_owner, false);
                    sCtx->paintStackingContext(canvas);
                }

                canvas->restore();
                iter2++;
            }
            iter++;
        }
    }

    m_owner->paintStackingContextContent(canvas);
    m_owner->paintOutline(canvas);

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
                    canvas->save();

                    {
                        CanvasStateRestorer r(canvas, sCtx, m_owner, false);
                        sCtx->paintStackingContext(canvas);
                    }

                    canvas->restore();
                    iter2++;
                }
            }
            iter++;
        }
    }

    if (!hasStackingBuffer && owner()->style()->opacity() != 1) {
        canvas->endOpacityLayer();
    }

    if (m_owner->layoutParent()->isFrameDocument()) {
        if (!m_owner->node()
                 ->document()
                 ->browsingContext()
                 ->isMainBrowsingContext()) {
            HTMLIFrameElement* iframe =
                m_owner->node()->document()->browsingContext()->sourceElement();
            if (!iframe->scrolling()->toASCIILower()->equals("no")) {
                FrameBlockBox* document =
                    m_owner->layoutParent()->asFrameBlockBox();
                m_owner->node()
                    ->document()
                    ->browsingContext()
                    ->window()
                    ->scrolling()
                    ->paintScrollbars(canvas, document,
                                      document->appliedOverflowX(),
                                      document->appliedOverflowY());
            }
        }
    }

    canvas->restore();
    if (hasStackingBuffer) {
        delete canvas;
    }
}

void StackingContext::compositeStackingContext(Canvas* canvas)
{
    LayoutRect visibleRect = StackingContext::visibleRect();
    ComputedStyle* ownerStyle = m_owner->style();
    canvas->save();

    if (needsOwnBuffer()) {
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

        if (ownerStyle->opacity() != 1) {
            canvas->beginOpacityLayer(ownerStyle->opacity());
        }

        m_rareData->m_matrix = m_owner->style()->transformsToMatrix(
            m_owner->width(), m_owner->height(), m_owner,
            ownerStyle->hasTransforms(m_owner));

        if (bufferWidth && bufferHeight) {
            if (!m_rareData->m_matrix.isIdentity()) {
                /* STARFISH_LOG_INFO("matrix [%f %f %f][%f %f %f][%f %f %f]\n",
                                   m_rareData->m_matrix.getScaleX(),
                                   m_rareData->m_matrix.getSkewX(),
                                   m_rareData->m_matrix.getTranslateX(),
                                   m_rareData->m_matrix.getSkewY(),
                                   m_rareData->m_matrix.getScaleY(),
                                   m_rareData->m_matrix.getTranslateY(),
                                   m_rareData->m_matrix.getPerspX(),
                                   m_rareData->m_matrix.getPerspY(),
                                   m_rareData->m_matrix.get(8));*/
                LayoutUnit ox = m_owner->width() / 2;
                LayoutUnit oy = m_owner->height() / 2;
                if (m_owner->style()->hasTransformOrigin()) {
                    ox = m_owner->style()
                             ->transformOrigin()
                             ->originValue()
                             ->getXAxis()
                             .specifiedValue(m_owner->width(), m_owner);
                    oy = m_owner->style()
                             ->transformOrigin()
                             ->originValue()
                             ->getYAxis()
                             .specifiedValue(m_owner->height(), m_owner);
                }
                SkMatrix test;
                bool testResult = m_rareData->m_matrix.invert(&test);
                if (!testResult) {
                    // ignorePaintingDueToInvalidMatrix
                    if (ownerStyle->opacity() != 1) {
                        canvas->endOpacityLayer();
                    }
                    canvas->restore();
                    return;
                }
                canvas->translate(ox, oy);
                canvas->postMatrix(m_rareData->m_matrix);
                canvas->translate(-ox, -oy);
            }

            if (owner()->shouldApplyOverflow()) {
                canvas->clip(
                    Unit::Rect(0, 0, owner()->width(), owner()->height()));
                if (m_owner->isFrameBlockBox())
                    canvas->translate(-m_owner->asFrameBlockBox()->scrollLeft(),
                                      -m_owner->asFrameBlockBox()->scrollTop());
            }

            owner()->willCompsiteStackingContext(canvas);
            canvas->drawImage(
                m_rareData->m_buffer,
                Unit::Rect(minX, minY, bufferWidth, bufferHeight));
            owner()->didCompsiteStackingContext(canvas);
        }
        // draw debug rect
        // canvas->setColor(Color(255, 0, 0, 128));
        // canvas->drawRect(Rect(minX, minY, bufferWidth, bufferHeight));
    } else {
        if (owner()->shouldApplyOverflow()) {
            canvas->clip(owner()->makeRect(BoxValue::BorderBoxBoxValue));
            if (m_owner->isFrameBlockBox())
                canvas->translate(-m_owner->asFrameBlockBox()->scrollLeft(),
                                  -m_owner->asFrameBlockBox()->scrollTop());
        }
        owner()->compsitingStackingContext(canvas);
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
                canvas->save();

                {
                    CanvasStateRestorer r(canvas, sCtx, m_owner, true);
                    sCtx->compositeStackingContext(canvas);
                }

                canvas->restore();
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
                    canvas->save();

                    {
                        CanvasStateRestorer r(canvas, sCtx, m_owner, true);
                        sCtx->compositeStackingContext(canvas);
                    }

                    canvas->restore();
                    iter2++;
                }
            }
            iter++;
        }
    }

    if (needsOwnBuffer()) {
        if (ownerStyle->opacity() != 1) {
            canvas->endOpacityLayer();
        }
    }

    canvas->restore();
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
    }

    return l;
}

Frame* StackingContext::hitTestStackingContext(LayoutUnit x, LayoutUnit y,
                                               BrowsingContext* from)
{
    if (m_rareData && !m_rareData->m_matrix.isIdentity()) {
        SkMatrix invert;
        if (!m_rareData->m_matrix.invert(&invert)) {
            return nullptr;
        }

        LayoutUnit ox = m_owner->width() / 2;
        LayoutUnit oy = m_owner->height() / 2;
        LayoutUnit vw = from->window()->width();
        LayoutUnit vh = from->window()->height();
        if (m_owner->style()->hasTransformOrigin()) {
            ox = m_owner->style()
                     ->transformOrigin()
                     ->originValue()
                     ->getXAxis()
                     .specifiedValue(m_owner->width(), m_owner);
            oy = m_owner->style()
                     ->transformOrigin()
                     ->originValue()
                     ->getYAxis()
                     .specifiedValue(m_owner->height(), m_owner);
        }
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
        if (owner()->FrameBox::hitTest(x, y, HitTestStageEnd) == nullptr) {
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
