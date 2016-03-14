#include "StarFishConfig.h"
#include "StackingContext.h"

#include "FrameBox.h"

namespace StarFish {

StackingContext::StackingContext(FrameBox* owner, StackingContext* parent)
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
    m_matrix = m_owner->style()->transformsToMatrix();
    m_needsOwnBuffer = forceNeedsBuffer || childNeedsBuffer || m_owner->style()->opacity() != 1 || !m_matrix.isIdentity();
    return m_needsOwnBuffer;
}

void StackingContext::paintStackingContext(Canvas* canvas)
{
    STARFISH_ASSERT(m_owner->isEstablishesStackingContext());

    Canvas* oldCanvas = nullptr;
    LayoutRect visibleRect = m_owner->visibleRect();
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
            m_buffer = ImageData::create(bufferWidth, bufferHeight);
        }

        m_buffer->clear();
        oldCanvas = canvas;
        canvas = Canvas::create(m_buffer);
        canvas->translate(-minX, -minY);
    }

    // Within each stacking context, the following layers are painted in back-to-front order:

    // the background and borders of the element forming the stacking context.
    m_owner->paintBackgroundAndBorders(canvas);

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
                sCtx->owner()->stackingContext()->paintStackingContext(canvas);

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
                    sCtx->owner()->stackingContext()->paintStackingContext(canvas);

                    canvas->restore();
                    iter2++;
                }
            }
            iter++;
        }
    }

    if (m_needsOwnBuffer) {
        delete canvas;
    }
}

void StackingContext::compositeStackingContext(Canvas* canvas)
{
    STARFISH_ASSERT(m_owner->isEstablishesStackingContext());

    LayoutRect visibleRect = m_owner->visibleRect();

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

        canvas->save();
        ComputedStyle* ownerStyle = m_owner->style();
        if (ownerStyle->opacity() != 1) {
            canvas->beginOpacityLayer(ownerStyle->opacity());
        }
        if (!m_matrix.isIdentity()) {
            // TODO transform-origin
            LayoutUnit ox = m_owner->width() / 2;
            LayoutUnit oy = m_owner->height() / 2;
            canvas->translate(ox, oy);
            canvas->postMatrix(m_matrix);
            canvas->drawImage(m_buffer, Rect(minX - ox, minY - oy, bufferWidth, bufferHeight));
        } else {
            canvas->drawImage(m_buffer, Rect(minX, minY, bufferWidth, bufferHeight));
        }



        if (ownerStyle->opacity() != 1) {
            canvas->endOpacityLayer();
        }
        canvas->restore();
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
                sCtx->owner()->stackingContext()->compositeStackingContext(canvas);

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
                    sCtx->owner()->stackingContext()->compositeStackingContext(canvas);

                    canvas->restore();
                    iter2++;
                }
            }
            iter++;
        }
    }
}

Frame* StackingContext::hitTestStackingContext(LayoutUnit x, LayoutUnit y)
{
    STARFISH_ASSERT(m_owner->isEstablishesStackingContext());

    if (!m_matrix.isIdentity()) {
        SkMatrix invert;
        STARFISH_RELEASE_ASSERT(m_matrix.invert(&invert));

        // TODO transform-origin
        LayoutUnit ox = m_owner->width() / 2;
        LayoutUnit oy = m_owner->height() / 2;
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
                result = sCtx->owner()->stackingContext()->hitTestStackingContext(x, y);
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
