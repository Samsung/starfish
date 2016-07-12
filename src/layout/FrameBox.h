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

#ifndef __StarFishFrameBox__
#define __StarFishFrameBox__

#include "layout/Frame.h"

namespace StarFish {

class InlineBox;
class LineBox;

struct MarginCollapseResult {
    LayoutUnit m_advanceY;
    LayoutUnit m_normalFlowHeightAdvance;
};

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

#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth)
    {
        Frame::dump(depth);
        printf(" frameRect(%g,%g,%g,%g) ", (float)x(), (float)y(), (float)width(), (float)height());
        LayoutRect rt = visibleRect();
        printf(" visibleRect(%g,%g,%g,%g) ", (float)rt.x(), (float)rt.y(), (float)rt.width(), (float)rt.height());
        printf(" padding(%g,%g,%g,%g) ", (float)paddingTop(), (float)paddingRight(), (float)paddingBottom(), (float)paddingLeft());
        printf(" border(%g,%g,%g,%g) ", (float)borderTop(), (float)borderRight(), (float)borderBottom(), (float)borderLeft());
        printf(" margin(%g,%g,%g,%g) ", (float)marginTop(), (float)marginRight(), (float)marginBottom(), (float)marginLeft());
    }
#endif

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

    void setMarginCollapseResult(const MarginCollapseResult& r)
    {
        m_marginCollapseResult = r;
    }

    const MarginCollapseResult& marginCollapseResult()
    {
        return m_marginCollapseResult;
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

    static void paintBackground(Canvas* canvas, ComputedStyle* style, LayoutRect imageRect, LayoutRect colorRect, bool isRootElement)
    {
        if (!style->backgroundColor().isTransparent() && style->visibility() == VisibilityValue::VisibleVisibilityValue) {
            canvas->save();
            canvas->setColor(style->backgroundColor());
            canvas->drawRect(colorRect);
            canvas->restore();
        }

        ImageData* id = style->backgroundImageData();
        if (id && id->width() && id->height()) {
            canvas->save();
            if (isRootElement) {
                canvas->translate(0, 0);
                canvas->clip(Rect(0, 0, colorRect.width(), colorRect.height()));
            } else {
                canvas->translate(imageRect.x(), imageRect.y());
                canvas->clip(Rect(0, 0, imageRect.width(), imageRect.height()));
            }

            float bw = imageRect.width();
            float bh = imageRect.height();

            if (style->bgSizeType() == BackgroundSizeType::Cover) {
                float imgR = id->width() / (float)id->height();
                if (id->width() < id->height())
                    canvas->drawImage(id, Rect(0, 0, bw, bh / imgR));
                else
                    canvas->drawImage(id, Rect(0, 0, bw*imgR, bh));

            } else if (style->bgSizeType() == BackgroundSizeType::Contain) {
                float boxR = bw / bh;
                float imgR = id->width() / (float)id->height();
                if (boxR > imgR) {
                    if (style->backgroundRepeatX() == BackgroundRepeatValue::RepeatRepeatValue) {
                        canvas->drawRepeatImage(id, Rect(0, 0, bw, bh), bh*imgR, bh, true, false, isRootElement);
                    } else {
                        canvas->drawImage(id, Rect(0, 0, bh*imgR, bh));
                    }
                } else {
                    if (style->backgroundRepeatX() == BackgroundRepeatValue::RepeatRepeatValue) {
                        canvas->drawRepeatImage(id, Rect(0, 0, bw, bh), bw, bw / imgR, true, false, isRootElement);
                    } else {
                        canvas->drawImage(id, Rect(0, 0, bw, bw / imgR));
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

                LayoutUnit x = style->backgroundPositionValue()->x().specifiedValue(bw - w);
                LayoutUnit y = style->backgroundPositionValue()->y().specifiedValue(bh - h);

                if (isRootElement) {
                    x += imageRect.x();
                    y += imageRect.y();
                    bw = colorRect.width();
                    bh = colorRect.height();
                }

                auto repeatX = style->backgroundRepeatX();
                auto repeatY = style->backgroundRepeatY();

                if (repeatX == BackgroundRepeatValue::RepeatRepeatValue && repeatY == BackgroundRepeatValue::RepeatRepeatValue) {
                    canvas->drawRepeatImage(id, Rect(x, y, bw, bh), w, h, true, true, isRootElement);
                } else if (repeatX == BackgroundRepeatValue::NoRepeatRepeatValue && repeatY == BackgroundRepeatValue::RepeatRepeatValue) {
                    canvas->drawRepeatImage(id, Rect(x, y, w, bh), w, h, false, true, isRootElement);
                } else if (repeatX == BackgroundRepeatValue::RepeatRepeatValue && repeatY == BackgroundRepeatValue::NoRepeatRepeatValue) {
                    canvas->drawRepeatImage(id, Rect(x, y, bw, h), w, h, true, false, isRootElement);
                } else {
                    canvas->drawImage(id, Rect(x, y, w, h));
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
            paintBackground(canvas, style(), bgRect, LayoutRect(0, 0, width(), height()), false);

        } while (false);

        canvas->save();

        // draw border-image
        if (style()->hasBorderImageData()) {
            double bWidth = style()->surround()->border.top().width().specifiedValue(height());
            double bImgWidth = style()->surround()->border.image().widths().top().specifiedValue(bWidth);
            double bImgSlice = style()->surround()->border.image().slices().top().specifiedValue(height());

            size_t imgWidth = style()->surround()->border.image().imageData()->width();
            size_t imgHeight = style()->surround()->border.image().imageData()->height();

            size_t lSlice = style()->surround()->border.image().slices().left().specifiedValue(width());
            size_t tSlice = style()->surround()->border.image().slices().top().specifiedValue(height());
            size_t rSlice = style()->surround()->border.image().slices().right().specifiedValue(width());
            size_t bSlice = style()->surround()->border.image().slices().bottom().specifiedValue(height());

            ImageData* imgData = style()->surround()->border.image().imageData();

            if (bImgSlice > imgWidth || bImgSlice > imgHeight)
                bImgSlice = std::min(imgWidth, imgHeight);

            double value = std::min((float)width() / (bImgWidth*2), (float)height() / (bImgWidth*2));
            if (value < 1)
                bImgWidth *= value;

            double scale = bImgWidth / bImgSlice;
            bool isFill = false;

            if ((lSlice + rSlice > imgWidth) || (tSlice + bSlice > imgHeight)) {
                float drawRect = std::min((float)width(), (float)height()) / 2.0;

                if (drawRect > bImgWidth)
                    drawRect = bImgWidth;

                // left-top
                canvas->drawBorderImage(imgData, Rect(0, 0, drawRect, drawRect), lSlice, tSlice, 0, 0, scale, isFill);
                // right-top
                canvas->drawBorderImage(imgData, Rect((float)width() - drawRect, 0, drawRect, drawRect), 0, tSlice, rSlice, 0, scale, isFill);
                // right-bottom
                canvas->drawBorderImage(imgData, Rect((float)width() - drawRect, (float)height() - drawRect, drawRect, drawRect), 0, 0, rSlice, bSlice, scale, isFill);
                // left-bottom
                canvas->drawBorderImage(imgData, Rect(0, (float)height() - drawRect, drawRect, drawRect), lSlice, 0, 0, bSlice, scale, isFill);
            } else {
                isFill = style()->surround()->border.image().sliceFill();
                canvas->drawBorderImage(imgData, Rect(0, 0, width(), height()), lSlice, tSlice, rSlice, bSlice, scale, isFill);
            }
        } else if (style()->hasBorderStyle()) {
            // draw border
            // TODO border-join

            if ((style()->borderTopColor() == style()->borderRightColor())
                && (style()->borderRightColor() == style()->borderBottomColor())
                && (style()->borderBottomColor() == style()->borderLeftColor())) {
                // if 4-colors are same.

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
            } else {

                // top
                canvas->setColor(style()->borderTopColor());
                canvas->drawRect(
                    LayoutLocation(0, 0),
                    LayoutLocation(width(), 0),
                    LayoutLocation(width() - borderRight(), borderTop()),
                    LayoutLocation(borderLeft(), borderTop())
                );

                // right
                canvas->setColor(style()->borderRightColor());
                canvas->drawRect(
                    LayoutLocation(width() - borderRight(), borderTop()),
                    LayoutLocation(width(), 0),
                    LayoutLocation(width(), height()),
                    LayoutLocation(width() - borderRight(), height() - borderBottom())
                );

                // bottom
                canvas->setColor(style()->borderBottomColor());
                canvas->drawRect(
                    LayoutLocation(borderLeft(), height() - borderBottom()),
                    LayoutLocation(width() - borderRight(), height() - borderBottom()),
                    LayoutLocation(width(), height()),
                    LayoutLocation(0, height())
                );

                // left
                canvas->setColor(style()->borderLeftColor());
                canvas->drawRect(
                    LayoutLocation(0, 0),
                    LayoutLocation(borderLeft(), borderTop()),
                    LayoutLocation(borderLeft(), height() - borderBottom()),
                    LayoutLocation(0, height())
                );
            }
        }

        canvas->restore();
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

    virtual LayoutRect visibleRect()
    {
        return LayoutRect(LayoutLocation(0, 0), LayoutSize(m_frameRect.size()));
    }
    virtual void setVisibleRect(LayoutRect vis)
    {
        STARFISH_ASSERT_NOT_REACHED();
    }

    LayoutLocation absolutePoint(FrameBox* top)
    {
        LayoutLocation l(0, 0);
        Frame* p = this;
        while (top != p) {
            l.setX(l.x() + p->asFrameBox()->x());
            l.setY(l.y() + p->asFrameBox()->y());
            p = p->layoutParent();
        }
        return l;
    }

    LayoutRect absoluteRect(FrameBox* top)
    {
        return LayoutRect(absolutePoint(top), frameRect().size());
    }

    void computeBorderMarginPadding(LayoutUnit parentContentWidth)
    {
        // padding
        if (style()->paddingLeft().isSpecified() && !m_flags.m_isLeftMBPCleared) {
            setPaddingLeft(style()->paddingLeft().specifiedValue(parentContentWidth));
        } else {
            setPaddingLeft(0);
        }
        if (style()->paddingTop().isSpecified()) {
            setPaddingTop(style()->paddingTop().specifiedValue(parentContentWidth));
        } else {
            setPaddingTop(0);
        }
        if (style()->paddingRight().isSpecified() && !m_flags.m_isRightMBPCleared) {
            setPaddingRight(style()->paddingRight().specifiedValue(parentContentWidth));
        } else {
            setPaddingRight(0);
        }
        if (style()->paddingBottom().isSpecified()) {
            setPaddingBottom(style()->paddingBottom().specifiedValue(parentContentWidth));
        } else {
            setPaddingBottom(0);
        }

        // border
        if (style()->hasBorderStyle()) {
            if (style()->borderLeftWidth().isSpecified() && !m_flags.m_isLeftMBPCleared) {
                setBorderLeft(style()->borderLeftWidth().specifiedValue(parentContentWidth));
            } else {
                setBorderLeft(0);
            }
            if (style()->borderTopWidth().isSpecified()) {
                setBorderTop(style()->borderTopWidth().specifiedValue(parentContentWidth));
            } else {
                setBorderTop(0);
            }
            if (style()->borderRightWidth().isSpecified() && !m_flags.m_isRightMBPCleared) {
                setBorderRight(style()->borderRightWidth().specifiedValue(parentContentWidth));
            } else {
                setBorderRight(0);
            }
            if (style()->borderBottomWidth().isSpecified()) {
                setBorderBottom(style()->borderBottomWidth().specifiedValue(parentContentWidth));
            } else {
                setBorderBottom(0);
            }
        }

        // margin
        if (style()->marginLeft().isSpecified() && !m_flags.m_isLeftMBPCleared) {
            setMarginLeft(style()->marginLeft().specifiedValue(parentContentWidth));
        } else {
            setMarginLeft(0);
        }
        if (style()->marginTop().isSpecified()) {
            setMarginTop(style()->marginTop().specifiedValue(parentContentWidth));
        } else {
            setMarginTop(0);
        }
        if (style()->marginRight().isSpecified() && !m_flags.m_isRightMBPCleared) {
            setMarginRight(style()->marginRight().specifiedValue(parentContentWidth));
        } else {
            setMarginRight(0);
        }
        if (style()->marginBottom().isSpecified()) {
            setMarginBottom(style()->marginBottom().specifiedValue(parentContentWidth));
        } else {
            setMarginBottom(0);
        }
    }

    void establishesStackingContextIfNeeds()
    {
        if (isEstablishesStackingContext()) {
            STARFISH_ASSERT(isRootElement() || m_stackingContext == nullptr);
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

    void clearStackingContextIfNeeds()
    {
        if (m_stackingContext) {
            delete m_stackingContext;
            m_stackingContext = nullptr;
        }
    }


    virtual void paintStackingContextContent(Canvas* canvas);

    virtual void iterateChildBoxes(const std::function<void(FrameBox*)>& fn)
    {
        fn(this);

        if (firstChild()) {
            FrameBox* child = firstChild()->asFrameBox();
            while (true) {
                child->iterateChildBoxes(fn);
                if (child->next())
                    child = child->next()->asFrameBox();
                else
                    break;
            }
        }
    }

protected:
    // content + padding + border
    LayoutRect m_frameRect;
    LayoutBoxSurroundData m_padding, m_border, m_margin;
    MarginCollapseResult m_marginCollapseResult;
    StackingContext* m_stackingContext;
};

}

#endif
