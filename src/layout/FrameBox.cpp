#include "StarFishConfig.h"
#include "FrameBox.h"

#include "FrameBlockBox.h"

namespace StarFish {

void FrameBox::paintStackingContext(Canvas* canvas)
{
    STARFISH_ASSERT(isEstablishesStackingContext());
    STARFISH_ASSERT(m_stackingContext);

    if (style()->opacity() != 1) {
        canvas->beginOpacityLayer(style()->opacity());
    }
    // Within each stacking context, the following layers are painted in back-to-front order:

    // the background and borders of the element forming the stacking context.
    paintBackgroundAndBorders(canvas);

    // the child stacking contexts with negative stack levels (most negative first).
    {
        auto iter = m_stackingContext->childContexts().begin();
        while (iter != m_stackingContext->childContexts().end()) {
            int32_t num = iter->first;
            if (num >= 0)
                break;
            auto iter2 = iter->second->begin();
            while (iter2 != iter->second->end()) {
                StackingContext* sCtx = *iter2;
                canvas->save();

                LayoutLocation l = sCtx->owner()->absolutePoint(this);
                canvas->translate(l.x(), l.y());
                sCtx->owner()->paintStackingContext(canvas);

                canvas->restore();
                iter2++;
            }
            iter++;
        }
    }

    paintStackingContextContent(canvas);

    // the child stacking contexts with positive stack levels (least positive first).
    {
        auto iter = m_stackingContext->childContexts().begin();
        while (iter != m_stackingContext->childContexts().end()) {
            int32_t num = iter->first;
            if (num >= 0) {
                auto iter2 = iter->second->begin();
                while (iter2 != iter->second->end()) {
                    StackingContext* sCtx = *iter2;
                    canvas->save();

                    LayoutLocation l = sCtx->owner()->absolutePoint(this);
                    canvas->translate(l.x(), l.y());
                    sCtx->owner()->paintStackingContext(canvas);

                    canvas->restore();
                    iter2++;
                }
            }
            iter++;
        }
    }

    if (style()->opacity() != 1) {
        canvas->endOpacityLayer();
    }

}

void FrameBox::paintStackingContextContent(Canvas* canvas)
{
    // the in-flow, non-inline-level, non-positioned descendants.
    paintChildrenWith(canvas, PaintingNormalFlowBlock);

    // TODO the non-positioned floats.
    // paintChildrenWith(canvas, ctx, PaintingNonPositionedFloats);

    // the in-flow, inline-level, non-positioned descendants, including inline tables and inline blocks.
    paintChildrenWith(canvas, PaintingNormalFlowInline);

    // the child stacking contexts with stack level 0 and the positioned descendants with stack level 0.
    paintChildrenWith(canvas, PaintingPositionedElements);
}

Frame* FrameBox::hitTestStackingContext(LayoutUnit x, LayoutUnit y)
{
    STARFISH_ASSERT(isEstablishesStackingContext());
    STARFISH_ASSERT(m_stackingContext);

    Frame* result = nullptr;
    // TODO the child stacking contexts with positive stack levels (least positive first).
    {
        auto iter = m_stackingContext->childContexts().rbegin();
        while (iter != m_stackingContext->childContexts().rend()) {
            int32_t num = iter->first;
            if (num > 0) {
                auto iter2 = iter->second->rbegin();
                LayoutUnit oldX = x;
                LayoutUnit oldY = y;
                while (iter2 != iter->second->rend()) {
                    StackingContext* sCtx = *iter2;
                    LayoutLocation l = sCtx->owner()->absolutePoint(this);
                    x -= l.x();
                    y -= l.y();
                    result = sCtx->owner()->hitTestStackingContext(x, y);
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
    result = hitTestChildrenWith(x, y, HitTestPositionedElements);
    if (result)
        return result;

    // the in-flow, inline-level, non-positioned descendants, including inline tables and inline blocks.
    result = hitTestChildrenWith(x, y, HitTestNormalFlowInline);
    if (result)
        return result;

    // TODO the non-positioned floats.

    // the in-flow, non-inline-level, non-positioned descendants.
    result = hitTestChildrenWith(x, y, HitTestNormalFlowBlock);
    if (result)
        return result;

    // TODO the child stacking contexts with negative stack levels (most negative first).
    {
        auto iter = m_stackingContext->childContexts().rbegin();
        while (iter != m_stackingContext->childContexts().rend()) {
            int32_t num = iter->first;
            if (num > 0)
                break;
            auto iter2 = iter->second->rbegin();
            LayoutUnit oldX = x;
            LayoutUnit oldY = y;
            while (iter2 != iter->second->rend()) {
                StackingContext* sCtx = *iter2;

                LayoutLocation l = sCtx->owner()->absolutePoint(this);
                x -= l.x();
                y -= l.y();
                result = sCtx->owner()->hitTestStackingContext(x, y);
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
    result = FrameBox::hitTest(x, y, HitTestNormalFlowBlock);
    if (result)
        return result;

    return nullptr;
}

}
