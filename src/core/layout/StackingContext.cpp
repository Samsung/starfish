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
        int32_t num = owner->style()->zIndex();
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
    return m_owner->style()->zIndex();
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

        if (m_rareData->m_visibleRect.isEmpty()) {
            m_rareData->m_needsOwnBuffer = false;
        }
    }

    return needsOwnBuffer();
}

void StackingContext::replaceCanvasState(Canvas* canvas, StackingContext* sCtx)
{
    FrameBox* self = sCtx->owner();
    CanvasState* state = canvas->getByFrame(self);
    if (state == nullptr) {
        LayoutLocation l = self->absolutePoint(m_owner);
        canvas->translate(l.x(), l.y());
    } else {
        canvas->replace(state, Canvas::ReplaceFlag::All);
#if defined(PORT_GRAPHIC_BACKEND_EFL)
        if (self->isAbsolutePositioned()) {
            bool isFixed = self->style()->position() == FixedPositionValue;
            Frame* parent = self->layoutParent();
            LayoutUnit offsetX = self->x(), offsetY = self->y();
            while (
                (!parent->style() || !parent->style()->hasTransforms(parent)) &&
                !parent->isFrameDocument() &&
                (isFixed || !parent->isPositioned())) {
                offsetX += parent->asFrameBox()->x();
                offsetY += parent->asFrameBox()->y();
                parent = parent->layoutParent();
            }
            CanvasState* parentState = canvas->getByFrame(parent);
            if (parentState == nullptr) {
                return;
            }

            canvas->replace(parentState, Canvas::ReplaceFlag::ClippingOnly);

            if (parent->shouldApplyOverflow()) {
                FrameBox* box = parent->asFrameBox();
                canvas->translate(-offsetX, -offsetY);
                canvas->clip(Unit::Rect(box->borderLeft(), box->borderTop(),
                                        box->width() - box->borderWidth(),
                                        box->height() - box->borderHeight()));
                if (box->isFrameBlockBox())
                    canvas->translate(-parent->asFrameBlockBox()->scrollLeft(),
                                      -parent->asFrameBlockBox()->scrollTop());
                canvas->translate(offsetX, offsetY);
            }
        }
#endif
    }
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
        // TODO treat when buffer is too large
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
        canvas = Canvas::create(m_rareData->m_buffer);
        canvas->setViewportWidthAndHeight(oldCanvas);
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
            m_owner->width(), m_owner->height(), canvas->viewportWidth(),
            canvas->viewportHeight(), m_owner->style()->hasTransforms(m_owner));

        if (!m.isIdentity()) {
            ensureRareData()->m_matrix = m;
            /*
            STARFISH_LOG_INFO("matrix\n[%f %f %f]\n[%f %f %f]\n[%f %f %f]\n"
                , m_matrix.get(0), m_matrix.get(1), m_matrix.get(2)
                , m_matrix.get(3), m_matrix.get(4), m_matrix.get(5)
                , m_matrix.get(6), m_matrix.get(7), m_matrix.get(8));
                */
            LayoutUnit ox = m_owner->width() / 2;
            LayoutUnit oy = m_owner->height() / 2;
            if (m_owner->style()->hasTransformOrigin()) {
                ox = m_owner->style()
                         ->transformOrigin()
                         ->originValue()
                         ->getXAxis()
                         .specifiedValue(m_owner->width(),
                                         canvas->viewportWidth());
                oy = m_owner->style()
                         ->transformOrigin()
                         ->originValue()
                         ->getYAxis()
                         .specifiedValue(m_owner->height(),
                                         canvas->viewportHeight());
            }
            canvas->translate(ox, oy);
            canvas->postMatrix(m_rareData->m_matrix);
            canvas->translate(-ox, -oy);
        }
    }

    if (!owner()->isNormalFlow() ||
        owner()->style()->display() == InlineBlockDisplayValue ||
        owner()->style()->display() == InlineTableDisplayValue) {
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
    m_owner->paintBackgroundAndBorders(canvas);

    if (!hasStackingBuffer && owner()->shouldApplyOverflow()) {
        canvas->clip(Unit::Rect(owner()->borderLeft(), owner()->borderTop(),
                                owner()->width() - owner()->borderWidth(),
                                owner()->height() - owner()->borderHeight()));
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

                replaceCanvasState(canvas, sCtx);

                sCtx->paintStackingContext(canvas);

                canvas->restore();
                iter2++;
            }
            iter++;
        }
    }

    m_owner->paintStackingContextContent(canvas);

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

                    replaceCanvasState(canvas, sCtx);
                    sCtx->paintStackingContext(canvas);

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
            m_owner->width(), m_owner->height(), canvas->viewportWidth(),
            canvas->viewportHeight(), ownerStyle->hasTransforms(m_owner));

        if (!m_rareData->m_matrix.isIdentity()) {
            /* STARFISH_LOG_INFO("matrix [%f %f %f][%f %f %f][%f %f %f]\n"
                , m_matrix.get(0), m_matrix.get(1), m_matrix.get(2)
                , m_matrix.get(3), m_matrix.get(4), m_matrix.get(5)
                , m_matrix.get(6), m_matrix.get(7), m_matrix.get(8)); */
            LayoutUnit ox = m_owner->width() / 2;
            LayoutUnit oy = m_owner->height() / 2;
            if (m_owner->style()->hasTransformOrigin()) {
                ox = m_owner->style()
                         ->transformOrigin()
                         ->originValue()
                         ->getXAxis()
                         .specifiedValue(m_owner->width(),
                                         canvas->viewportWidth());
                oy = m_owner->style()
                         ->transformOrigin()
                         ->originValue()
                         ->getYAxis()
                         .specifiedValue(m_owner->height(),
                                         canvas->viewportHeight());
            }
            canvas->translate(ox, oy);
            canvas->postMatrix(m_rareData->m_matrix);
            canvas->translate(-ox, -oy);
        }

        if (owner()->shouldApplyOverflow()) {
            canvas->clip(Unit::Rect(0, 0, owner()->width(), owner()->height()));
            if (m_owner->isFrameBlockBox())
                canvas->translate(-m_owner->asFrameBlockBox()->scrollLeft(),
                                  -m_owner->asFrameBlockBox()->scrollTop());
        }

        owner()->willCompsiteStackingContext(canvas);
        canvas->drawImage(m_rareData->m_buffer,
                          Unit::Rect(minX, minY, bufferWidth, bufferHeight));
        owner()->didCompsiteStackingContext(canvas);

        // draw debug rect
        // canvas->setColor(Color(255, 0, 0, 128));
        // canvas->drawRect(Rect(minX, minY, bufferWidth, bufferHeight));
    } else {
        if (owner()->shouldApplyOverflow()) {
            canvas->clip(Unit::Rect(0, 0, owner()->width(), owner()->height()));
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

                LayoutLocation l = sCtx->owner()->absolutePoint(m_owner);
                canvas->translate(l.x(), l.y());
                sCtx->compositeStackingContext(canvas);

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

                    LayoutLocation l = sCtx->owner()->absolutePoint(m_owner);
                    canvas->translate(l.x(), l.y());
                    sCtx->compositeStackingContext(canvas);

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
        if (m_owner->style()->hasTransformOrigin()) {
            ox = m_owner->style()
                     ->transformOrigin()
                     ->originValue()
                     ->getXAxis()
                     .specifiedValue(m_owner->width(), from->window()->width());
            oy = m_owner->style()
                     ->transformOrigin()
                     ->originValue()
                     ->getYAxis()
                     .specifiedValue(m_owner->height(),
                                     from->window()->height());
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

    if (owner()->style()->overflowX() != OverflowValue::VisibleOverflow ||
        owner()->style()->overflowY() != OverflowValue::VisibleOverflow) {
        if (owner()->FrameBox::hitTest(x, y, HitTestStageEnd) == nullptr) {
            return nullptr;
        }
    }

    if (m_owner->isFrameBlockBox()) {
        x -= m_owner->asFrameBlockBox()->scrollLeft();
        y -= m_owner->asFrameBlockBox()->scrollTop();
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
                    LayoutLocation l = sCtx->owner()->absolutePoint(m_owner);
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

                LayoutLocation l = sCtx->owner()->absolutePoint(m_owner);
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
        x += m_owner->asFrameBlockBox()->scrollLeft();
        y += m_owner->asFrameBlockBox()->scrollTop();
    }

    // the background and borders of the element forming the stacking context.
    result = m_owner->FrameBox::hitTest(x, y, HitTestNormalFlowBlock);
    if (result) {
        return result;
    }

    return nullptr;
}
}
