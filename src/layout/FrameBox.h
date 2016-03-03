#ifndef __StarFishFrameBox__
#define __StarFishFrameBox__

#include "layout/Frame.h"

namespace StarFish {

class InlineBox;
class LineBox;

class FrameBox : public Frame {
public:
    FrameBox(Node* node, ComputedStyle* style)
        : Frame(node, style)
        , m_frameRect(0, 0, 0, 0)
        , m_stackingContext(nullptr)
    {
    }

    virtual bool isFrameBox()
    {
        return true;
    }

    virtual bool isLineBox()
    {
        return false;
    }

    virtual bool isInlineBox()
    {
        return false;
    }

    virtual bool isSelfCollapsingBlock()
    {
        return false;
    }

    LineBox* asLineBox()
    {
        STARFISH_ASSERT(isLineBox());
        return (LineBox*)this;
    }

    InlineBox* asInlineBox()
    {
        STARFISH_ASSERT(isInlineBox());
        return (InlineBox*)this;
    }

    virtual void dump(int depth)
    {
        Frame::dump(depth);
        printf(" frameRect(%g,%g,%g,%g) ", (float)x(), (float)y(), (float)width(), (float)height());
    }

    StackingContext* stackingContext()
    {
        return m_stackingContext;
    }

    const LayoutRect& frameRect() { return m_frameRect; }

    LayoutUnit x() const { return m_frameRect.x(); }
    LayoutUnit y() const { return m_frameRect.y(); }
    LayoutUnit width() const { return m_frameRect.width(); }
    LayoutUnit height() const { return m_frameRect.height(); }

    void setX(LayoutUnit x) { m_frameRect.setX(x); }
    void setY(LayoutUnit y) { m_frameRect.setY(y); }
    void moveX(LayoutUnit t) { setX(x() + t); }
    void moveY(LayoutUnit t) { setY(y() + t); }
    void setWidth(LayoutUnit width) { m_frameRect.setWidth(width); }
    void setHeight(LayoutUnit height) { m_frameRect.setHeight(height); }

    void setContentWidth(LayoutUnit width) { m_frameRect.setWidth(width + paddingWidth() + borderWidth()); }
    void setContentHeight(LayoutUnit height) { m_frameRect.setHeight(height + paddingHeight() + borderHeight()); }

    void setPaddingTop(LayoutUnit t) { m_padding.setTop(t); }
    void setPaddingRight(LayoutUnit t) { m_padding.setRight(t); }
    void setPaddingBottom(LayoutUnit t) { m_padding.setBottom(t); }
    void setPaddingLeft(LayoutUnit t) { m_padding.setLeft(t); }

    LayoutUnit paddingTop() { return m_padding.top(); }
    LayoutUnit paddingRight() { return m_padding.right(); }
    LayoutUnit paddingBottom() { return m_padding.bottom(); }
    LayoutUnit paddingLeft() { return m_padding.left(); }

    void setBorderTop(LayoutUnit t) { m_border.setTop(t); }
    void setBorderRight(LayoutUnit t) { m_border.setRight(t); }
    void setBorderBottom(LayoutUnit t) { m_border.setBottom(t); }
    void setBorderLeft(LayoutUnit t) { m_border.setLeft(t); }

    LayoutUnit borderTop() { return m_border.top(); }
    LayoutUnit borderRight() { return m_border.right(); }
    LayoutUnit borderBottom() { return m_border.bottom(); }
    LayoutUnit borderLeft() { return m_border.left(); }

    void setMarginTop(LayoutUnit t) { m_margin.setTop(t); }
    void setMarginRight(LayoutUnit t) { m_margin.setRight(t); }
    void setMarginBottom(LayoutUnit t) { m_margin.setBottom(t); }
    void setMarginLeft(LayoutUnit t) { m_margin.setLeft(t); }

    LayoutUnit marginTop() { return m_margin.top(); }
    LayoutUnit marginRight() { return m_margin.right(); }
    LayoutUnit marginBottom() { return m_margin.bottom(); }
    LayoutUnit marginLeft() { return m_margin.left(); }

    LayoutUnit paddingWidth()
    {
        return m_padding.left() + m_padding.right();
    }

    LayoutUnit paddingHeight()
    {
        return m_padding.top() + m_padding.bottom();
    }

    LayoutUnit borderWidth()
    {
        return m_border.left() + m_border.right();
    }

    LayoutUnit borderHeight()
    {
        return m_border.top() + m_border.bottom();
    }

    LayoutUnit marginWidth()
    {
        return m_margin.left() + m_margin.right();
    }

    LayoutUnit marginHeight()
    {
        return m_margin.top() + m_margin.bottom();
    }

    LayoutUnit contentWidth()
    {
        return m_frameRect.width() - paddingWidth() - borderWidth();
    }

    LayoutUnit contentHeight()
    {
        return m_frameRect.height() - paddingHeight() - borderHeight();
    }

    virtual void paintChildrenWith(Canvas* canvas, PaintingStage stage)
    {
        Frame* child = firstChild();
        while (child) {
            canvas->save();
            canvas->translate(child->asFrameBox()->x(), child->asFrameBox()->y());
            child->paint(canvas, stage);
            canvas->restore();
            child = child->next();
        }
    }

    static void paintBackground(Canvas* canvas, ComputedStyle* style, LayoutRect bgRect, LayoutRect borderBox)
    {
        if (!style->bgColor().isTransparent()) {
            canvas->save();
            canvas->setColor(style->bgColor());
            canvas->drawRect(borderBox);
            canvas->restore();
        }

        ImageData* id = style->bgImageData();
        if (id && id->width() && id->height()) {
            // TODO background-position
            canvas->save();
            canvas->translate(bgRect.x(), bgRect.y());
            float bw = bgRect.width();
            float bh = bgRect.height();
            canvas->clip(Rect(0, 0, bgRect.width(), bgRect.height()));

            if (style->bgSizeType() == BackgroundSizeType::Cover) {
                canvas->drawImage(id, Rect(0, 0, bw, bh));
            } else if (style->bgSizeType() == BackgroundSizeType::Contain) {
                float boxR = bw / bh;
                float imgR = id->width() / (float)id->height();
                if (boxR > imgR) {
                    float start = bh * (float)id->width() / (float)id->height();
                    canvas->drawImage(id, Rect(0, 0, start, bh));

                    if (style->backgroundRepeatX() == BackgroundRepeatValue::RepeatRepeatValue) {
                        for (float s = start; s < bw; s += start) {
                            canvas->drawImage(id, Rect(s, 0, start, bh));
                        }
                    }
                } else {
                    float start = bw * (float)id->height() / (float)id->width();
                    canvas->drawImage(id, Rect(0, 0, bw, start));

                    if (style->backgroundRepeatY() == BackgroundRepeatValue::RepeatRepeatValue) {
                        for (float s = start; s < bw; s += start) {
                            canvas->drawImage(id, Rect(0, s, bw, start));
                        }
                    }
                }

            } else if (style->bgSizeType() == BackgroundSizeType::SizeValue) {
                float w, h;

                if (style->bgSizeValue()->width().isAuto() && style->bgSizeValue()->height().isAuto()) {
                    w = id->width();
                    h = id->height();
                } else if (style->bgSizeValue()->width().isAuto() && !style->bgSizeValue()->height().isAuto()) {
                    h = style->bgSizeValue()->height().specifiedValue(bh);
                    w = h * id->width() / id->height();
                } else if (!style->bgSizeValue()->width().isAuto() && style->bgSizeValue()->height().isAuto()) {
                    w = style->bgSizeValue()->width().specifiedValue(bw);
                    h = w * id->height() / id->width();
                } else {
                    w = style->bgSizeValue()->width().specifiedValue(bw);
                    h = style->bgSizeValue()->height().specifiedValue(bh);
                }

                canvas->drawImage(id, Rect(0, 0, w, h));
                if (style->backgroundRepeatX() == BackgroundRepeatValue::RepeatRepeatValue) {
                    if (style->backgroundRepeatY() == BackgroundRepeatValue::RepeatRepeatValue) {
                        for (float x = 0; x < bw; x += w) {
                            for (float y = 0; y < bh; y += h) {
                                canvas->drawImage(id, Rect(x, y, w, h));
                            }
                        }
                    } else {
                        for (float x = 0; x < bw; x += w) {
                            canvas->drawImage(id, Rect(x, 0, w, h));
                        }
                    }
                } else {
                    if (style->backgroundRepeatY() == BackgroundRepeatValue::RepeatRepeatValue) {
                        for (float y = 0; y < bh; y += h) {
                            canvas->drawImage(id, Rect(0, y, w, h));
                        }
                    }
                }
            } else {
                STARFISH_ASSERT(style->bgSizeType() == BackgroundSizeType::SizeNone);
                STARFISH_ASSERT_NOT_REACHED();
            }
            canvas->restore();
        }
    }

    virtual void paintBackgroundAndBorders(Canvas* canvas)
    {
        do {
            if (node() && node()->isElement() && node()->asElement()->isHTMLElement() && node()->asElement()->asHTMLElement()->isHTMLHtmlElement()) {
                break;
            }

            if (node() && node()->isElement() && node()->asElement()->isHTMLElement() && node()->asElement()->asHTMLElement()->isHTMLBodyElement()) {
                if (!node()->document()->window()->hasRootElementBackground()) {
                    break;
                }
            }

            LayoutRect bgRect(borderLeft(), borderTop(), m_frameRect.width() - borderWidth(), m_frameRect.height() - borderHeight());
            paintBackground(canvas, style(), bgRect, LayoutRect(0, 0, width(), height()));

        } while (false);

        // draw border
        if (style()->hasBorderStyle()) {
            // TODO border-join
            canvas->save();

            // top
            canvas->setColor(style()->borderTopColor());
            canvas->drawRect(LayoutRect(0, 0, width(), borderTop()));
            // right
            canvas->setColor(style()->borderRightColor());
            canvas->drawRect(LayoutRect(width()-borderRight(), 0, borderRight(), height()));
            // bottom
            canvas->setColor(style()->borderBottomColor());
            canvas->drawRect(LayoutRect(0, height()-borderBottom(), width(), borderBottom()));
            // left
            canvas->setColor(style()->borderLeftColor());
            canvas->drawRect(LayoutRect(0, 0, borderLeft(), height()));

            canvas->restore();
        }
    }

    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage)
    {
        if (x >= 0 && x < m_frameRect.width() && y >= 0 && y < m_frameRect.height()) {
            return this;
        }
        return nullptr;
    }

    virtual Frame* hitTestChildrenWith(LayoutUnit x, LayoutUnit y, HitTestStage stage)
    {
        Frame* child = lastChild();
        Frame* result = nullptr;
        while (child) {
            LayoutUnit cx = x - child->asFrameBox()->x();
            LayoutUnit cy = y - child->asFrameBox()->y();
            result = child->hitTest(cx, cy, stage);
            if (result)
                return result;
            child = child->previous();
        }
        return result;
    }

    Location absolutePoint(FrameBox* top)
    {
        Location l(0, 0);
        Frame* p = this;
        while (top != p) {
            l.setX(l.x() + p->asFrameBox()->x());
            l.setY(l.y() + p->asFrameBox()->y());
            p = p->layoutParent();
        }
        return l;
    }

    void computeBorderMarginPadding(LayoutUnit parentContentWidth)
    {
        // padding
        if (style()->paddingLeft().isSpecified()) {
            setPaddingLeft(style()->paddingLeft().specifiedValue(parentContentWidth));
        }
        if (style()->paddingTop().isSpecified()) {
            setPaddingTop(style()->paddingTop().specifiedValue(parentContentWidth));
        }
        if (style()->paddingRight().isSpecified()) {
            setPaddingRight(style()->paddingRight().specifiedValue(parentContentWidth));
        }
        if (style()->paddingBottom().isSpecified()) {
            setPaddingBottom(style()->paddingBottom().specifiedValue(parentContentWidth));
        }

        // border
        if (style()->hasBorderStyle()) {
            if (style()->borderLeftWidth().isSpecified()) {
                setBorderLeft(style()->borderLeftWidth().specifiedValue(parentContentWidth));
            }
            if (style()->borderTopWidth().isSpecified()) {
                setBorderTop(style()->borderTopWidth().specifiedValue(parentContentWidth));
            }
            if (style()->borderRightWidth().isSpecified()) {
                setBorderRight(style()->borderRightWidth().specifiedValue(parentContentWidth));
            }
            if (style()->borderBottomWidth().isSpecified()) {
                setBorderBottom(style()->borderBottomWidth().specifiedValue(parentContentWidth));
            }
        }

        // margin
        if (style()->marginLeft().isSpecified()) {
            setMarginLeft(style()->marginLeft().specifiedValue(parentContentWidth));
        }
        if (style()->marginTop().isSpecified()) {
            setMarginTop(style()->marginTop().specifiedValue(parentContentWidth));
        }
        if (style()->marginRight().isSpecified()) {
            setMarginRight(style()->marginRight().specifiedValue(parentContentWidth));
        }
        if (style()->marginBottom().isSpecified()) {
            setMarginBottom(style()->marginBottom().specifiedValue(parentContentWidth));
        }
    }

    void establishesStackingContextIfNeeds()
    {
        if (isEstablishesStackingContext()) {
            if (!isRootElement()) {
                FrameBox* p = layoutParent()->asFrameBox();
                while (!p->isEstablishesStackingContext()) {
                    p = p->layoutParent()->asFrameBox();
                }
                m_stackingContext = new StackingContext(this, p->stackingContext());
            } else {
                m_stackingContext = new StackingContext(this, nullptr);
            }

        }
    }

    void paintStackingContext(Canvas* canvas);
    virtual void paintStackingContextContent(Canvas* canvas);

    Frame* hitTestStackingContext(LayoutUnit x, LayoutUnit y);



protected:
    // content + padding + border
    LayoutRect m_frameRect;
    LayoutBoxSurroundData m_padding, m_border, m_margin;

    StackingContext* m_stackingContext;
};

}

#endif
