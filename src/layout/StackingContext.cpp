/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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
#include "StackingContext.h"

#include "FrameBox.h"

namespace StarFish {

StackingContext::StackingContext(FrameBox* owner, StackingContext* parent)
    : m_visibleRect(0, 0, 0, 0)
{
    m_owner = owner;
    m_parent = parent;
    if (m_parent) {
        int32_t num = owner->style()->zIndex();
        auto iter = m_parent->m_childContexts.find(num);
        if (iter == m_parent->m_childContexts.end()) {
            iter = m_parent->m_childContexts.insert(std::make_pair(num, new StackingContextChild())).first;
        }
        iter->second->push_back(this);
    }
    m_needsOwnBuffer = false;
    m_buffer = nullptr;
}

StackingContext::~StackingContext()
{
    clearChildContexts();
    delete m_buffer;
}

bool StackingContext::computeStackingContextProperties(bool forceNeedsBuffer)
{
    bool childNeedsBuffer = false;
    auto iter = m_childContexts.begin();
    while (iter != m_childContexts.end()) {
        auto iter2 = iter->second->begin();
        while (iter2 != iter->second->end()) {
            childNeedsBuffer = (*iter2)->computeStackingContextProperties(childNeedsBuffer);
            iter2++;
        }
        iter++;
    }

    m_matrix.reset();
    m_needsOwnBuffer = forceNeedsBuffer || childNeedsBuffer || m_owner->needsGraphicsBuffer();

    if (m_needsOwnBuffer) {
        LayoutLocation l(-m_owner->frameRect().location().x(), -m_owner->frameRect().location().y());
        m_owner->iterateChildBoxes([&](FrameBox* box) -> bool
        {
            if (box != m_owner && box->stackingContext() && box->stackingContext()->needsOwnBuffer())
                return false;
            LayoutRect r = box->frameRect();
            r.setX(r.x() + l.x());
            r.setY(r.y() + l.y());
            m_visibleRect.unite(r);
            if (box->style() && box->style()->overflow() == OverflowValue::HiddenOverflow) {
                return false;
            }
            return true;
        }, [&](FrameBox* box)
        {
            l.setX(l.x() + box->x());
            l.setY(l.y() + box->y());
        }, [&](FrameBox* box)
        {
            l.setX(l.x() - box->x());
            l.setY(l.y() - box->y());
        });
    }

    return m_needsOwnBuffer;
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

    if (m_needsOwnBuffer) {
        // TODO treat when buffer is too large
        if (!m_buffer || ((m_buffer->width() != bufferWidth) && (m_buffer->height() != bufferHeight))) {
            if (!m_buffer) {
                delete m_buffer;
            }
            m_buffer = CanvasSurface::create(m_owner->node()->document()->window(), bufferWidth, bufferHeight);
        }

        m_buffer->clear();
        oldCanvas = canvas;
        canvas = Canvas::create(m_buffer);
        canvas->translate(-minX, -minY);
    }

    {
        // draw debug rect
        // canvas->save();
        // canvas->setColor(Color(0, 0, 255, 64));
        // canvas->drawRect(visibleRect);
        // canvas->restore();
    }

    // Within each stacking context, the following layers are painted in back-to-front order:

    // the background and borders of the element forming the stacking context.
    m_owner->paintBackgroundAndBorders(canvas);

    if (!m_needsOwnBuffer && owner()->shouldApplyOverflow()) {
        canvas->save();
        canvas->clip(Rect(owner()->borderLeft(), owner()->borderTop(), owner()->width()-owner()->borderWidth(), owner()->height()-owner()->borderHeight()));
    }

    // the child stacking contexts with negative stack levels (most negative first).
    {
        auto iter = childContexts().begin();
        while (iter != childContexts().end()) {
            int32_t num = iter->first;
            if (num >= 0)
                break;
            auto iter2 = iter->second->begin();
            while (iter2 != iter->second->end()) {
                StackingContext* sCtx = *iter2;
                canvas->save();

                LayoutLocation l = sCtx->owner()->absolutePoint(m_owner);
                canvas->translate(l.x(), l.y());
                sCtx->paintStackingContext(canvas);

                canvas->restore();
                iter2++;
            }
            iter++;
        }
    }

    m_owner->paintStackingContextContent(canvas);

    // the child stacking contexts with positive stack levels (least positive first).
    {
        auto iter = childContexts().begin();
        while (iter != childContexts().end()) {
            int32_t num = iter->first;
            if (num >= 0) {
                auto iter2 = iter->second->begin();
                while (iter2 != iter->second->end()) {
                    StackingContext* sCtx = *iter2;
                    canvas->save();

                    LayoutLocation l = sCtx->owner()->absolutePoint(m_owner);
                    canvas->translate(l.x(), l.y());
                    sCtx->paintStackingContext(canvas);

                    canvas->restore();
                    iter2++;
                }
            }
            iter++;
        }
    }

    if (m_needsOwnBuffer) {
        delete canvas;
    } else {
        if (owner()->shouldApplyOverflow()) {
            canvas->restore();
        }
    }
}

void StackingContext::compositeStackingContext(Canvas* canvas)
{
    LayoutRect visibleRect = StackingContext::visibleRect();
    ComputedStyle* ownerStyle = m_owner->style();
    canvas->save();

    if (m_needsOwnBuffer) {

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

        m_matrix = m_owner->style()->transformsToMatrix(m_owner->width(), m_owner->height(), m_owner->isFrameBlockBox() || m_owner->isFrameReplaced());

        if (!m_matrix.isIdentity()) {
            /* printf("matrix [%f %f %f][%f %f %f][%f %f %f]\n"
                , m_matrix.get(0), m_matrix.get(1), m_matrix.get(2)
                , m_matrix.get(3), m_matrix.get(4), m_matrix.get(5)
                , m_matrix.get(6), m_matrix.get(7), m_matrix.get(8)); */
            LayoutUnit ox = m_owner->width() / 2;
            LayoutUnit oy = m_owner->height() / 2;
            if (m_owner->style()->hasTransformOrigin()) {
                ox = m_owner->style()->transformOrigin()->originValue()->getXAxis().specifiedValue(m_owner->width());
                oy = m_owner->style()->transformOrigin()->originValue()->getYAxis().specifiedValue(m_owner->height());
            }
            canvas->translate(ox, oy);
            canvas->postMatrix(m_matrix);
            canvas->translate(-ox, -oy);
        }

        if (owner()->shouldApplyOverflow()) {
            canvas->clip(Rect(0, 0, owner()->width(), owner()->height()));
        }
        canvas->drawImage(m_buffer, Rect(minX, minY, bufferWidth, bufferHeight));

        // draw debug rect
        // canvas->setColor(Color(255, 0, 0, 128));
        // canvas->drawRect(Rect(minX, minY, bufferWidth, bufferHeight));
    } else {
        if (owner()->shouldApplyOverflow()) {
            canvas->clip(Rect(0, 0, owner()->width(), owner()->height()));
        }
    }

    // Within each stacking context, the following layers are painted in back-to-front order:

    // the child stacking contexts with negative stack levels (most negative first).
    {
        auto iter = childContexts().begin();
        while (iter != childContexts().end()) {
            int32_t num = iter->first;
            if (num >= 0)
                break;
            auto iter2 = iter->second->begin();
            while (iter2 != iter->second->end()) {
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

    // the child stacking contexts with positive stack levels (least positive first).
    {
        auto iter = childContexts().begin();
        while (iter != childContexts().end()) {
            int32_t num = iter->first;
            if (num >= 0) {
                auto iter2 = iter->second->begin();
                while (iter2 != iter->second->end()) {
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

    if (m_needsOwnBuffer) {
        if (ownerStyle->opacity() != 1) {
            canvas->endOpacityLayer();
        }
    }
    canvas->restore();
}

Frame* StackingContext::hitTestStackingContext(LayoutUnit x, LayoutUnit y)
{
    if (!m_matrix.isIdentity()) {
        SkMatrix invert;
        if (!m_matrix.invert(&invert)) {
            return nullptr;
        }

        LayoutUnit ox = m_owner->width() / 2;
        LayoutUnit oy = m_owner->height() / 2;
        if (m_owner->style()->hasTransformOrigin()) {
            ox = m_owner->style()->transformOrigin()->originValue()->getXAxis().specifiedValue(m_owner->width());
            oy = m_owner->style()->transformOrigin()->originValue()->getYAxis().specifiedValue(m_owner->height());
        }
        x -= ox;
        y -= oy;
        SkPoint pt = SkPoint::Make((float)x, (float)y);
        invert.mapPoints(&pt, 1);
        x = pt.x() + ox;
        y = pt.y() + oy;
    }

    Frame* result = nullptr;
    // the child stacking contexts with positive stack levels (least positive first).
    {
        auto iter = childContexts().rbegin();
        while (iter != childContexts().rend()) {
            int32_t num = iter->first;
            if (num >= 0) {
                auto iter2 = iter->second->rbegin();
                LayoutUnit oldX = x;
                LayoutUnit oldY = y;
                while (iter2 != iter->second->rend()) {
                    StackingContext* sCtx = *iter2;
                    LayoutLocation l = sCtx->owner()->absolutePoint(m_owner);
                    x -= l.x();
                    y -= l.y();
                    result = sCtx->hitTestStackingContext(x, y);
                    x = oldX;
                    y = oldY;
                    if (result)
                        return result;

                    iter2++;
                }
            }
            iter++;
        }
    }

    // the child stacking contexts with stack level 0 and the positioned descendants with stack level 0.
    result = m_owner->hitTestChildrenWith(x, y, HitTestPositionedElements);
    if (result)
        return result;

    // the in-flow, inline-level, non-positioned descendants, including inline tables and inline blocks.
    result = m_owner->hitTestChildrenWith(x, y, HitTestNormalFlowInline);
    if (result)
        return result;

    // TODO the non-positioned floats.

    // the in-flow, non-inline-level, non-positioned descendants.
    result = m_owner->hitTestChildrenWith(x, y, HitTestNormalFlowBlock);
    if (result)
        return result;

    // TODO the child stacking contexts with negative stack levels (most negative first).
    {
        auto iter = childContexts().rbegin();
        while (iter != childContexts().rend()) {
            int32_t num = iter->first;
            if (num > 0)
                break;
            auto iter2 = iter->second->rbegin();
            LayoutUnit oldX = x;
            LayoutUnit oldY = y;
            while (iter2 != iter->second->rend()) {
                StackingContext* sCtx = *iter2;

                LayoutLocation l = sCtx->owner()->absolutePoint(m_owner);
                x -= l.x();
                y -= l.y();
                result = sCtx->hitTestStackingContext(x, y);
                if (result)
                    return result;

                x = oldX;
                y = oldY;
                iter2++;
            }
            iter++;
        }
    }


    // the background and borders of the element forming the stacking context.
    result = m_owner->FrameBox::hitTest(x, y, HitTestNormalFlowBlock);
    if (result)
        return result;

    return nullptr;
}

}
