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

#ifndef __StarFishFrameBox__
#define __StarFishFrameBox__

#include "layout/Frame.h"
#include "platform/canvas/Canvas.h"

namespace StarFish {

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
        , m_padding()
        , m_border()
        , m_margin()
        , m_marginCollapseResult()
        , m_stackingContext(nullptr)
        , m_inlineBoxIndex(SIZE_MAX)
    {
    }

    virtual bool isFrameBox()
    {
        return true;
    }

#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth)
    {
        Frame::dump(depth);
        printf(" frameRect(%g,%g,%g,%g) ", (float)x(), (float)y(),
               (float)width(), (float)height());
        printf(" padding(%g,%g,%g,%g) ", (float)paddingTop(),
               (float)paddingRight(), (float)paddingBottom(),
               (float)paddingLeft());
        printf(" border(%g,%g,%g,%g) ", (float)borderTop(),
               (float)borderRight(), (float)borderBottom(),
               (float)borderLeft());
        printf(" margin(%g,%g,%g,%g) ", (float)marginTop(),
               (float)marginRight(), (float)marginBottom(),
               (float)marginLeft());
    }
#endif

    StackingContext* stackingContext()
    {
        return m_stackingContext;
    }

    const LayoutRect& frameRect()
    {
        return m_frameRect;
    }

    LayoutUnit x() const
    {
        return m_frameRect.x();
    }

    LayoutUnit y() const
    {
        return m_frameRect.y();
    }

    LayoutUnit width() const
    {
        return m_frameRect.width();
    }

    LayoutUnit height() const
    {
        return m_frameRect.height();
    }

    void setX(LayoutUnit x)
    {
        m_frameRect.setX(x);
    }

    void setY(LayoutUnit y)
    {
        m_frameRect.setY(y);
    }

    void setAbsX(LayoutUnit x, LayoutUnit absX)
    {
        m_frameRect.setX(x - absX);
    }

    void setAbsY(LayoutUnit y, LayoutUnit absY)
    {
        m_frameRect.setY(y - absY);
    }

    void moveX(LayoutUnit t)
    {
        setX(x() + t);
    }

    void moveY(LayoutUnit t)
    {
        setY(y() + t);
    }

    void setWidth(LayoutUnit width)
    {
        m_frameRect.setWidth(width);
    }

    void setHeight(LayoutUnit height)
    {
        m_frameRect.setHeight(height);
    }

    void applyMinMaxWidthIfNeeds(LayoutUnit width, LayoutUnit parentWidth,
                                 bool parentHasFixedValue = true)
    {
        setContentWidth(
            minMaxWidthAppliedIfNeeds(width, parentWidth, parentHasFixedValue));
    }

    void applyMinMaxHeightIfNeeds(LayoutUnit height, LayoutUnit parentHeight,
                                  bool parentHasFixedValue = true)
    {
        setContentHeight(minMaxHeightAppliedIfNeeds(height, parentHeight,
                                                    parentHasFixedValue));
    }

    void setContentWidth(LayoutUnit width)
    {
        m_frameRect.setWidth(width + paddingWidth() + borderWidth());
    }

    void setContentHeight(LayoutUnit height)
    {
        m_frameRect.setHeight(height + paddingHeight() + borderHeight());
    }

    void setPaddingTop(LayoutUnit t)
    {
        m_padding.setTop(t);
    }

    void setPaddingRight(LayoutUnit t)
    {
        m_padding.setRight(t);
    }

    void setPaddingBottom(LayoutUnit t)
    {
        m_padding.setBottom(t);
    }

    void setPaddingLeft(LayoutUnit t)
    {
        m_padding.setLeft(t);
    }

    LayoutUnit paddingTop() const
    {
        return m_padding.top();
    }

    LayoutUnit paddingRight() const
    {
        return m_padding.right();
    }

    LayoutUnit paddingBottom() const
    {
        return m_padding.bottom();
    }

    LayoutUnit paddingLeft() const
    {
        return m_padding.left();
    }

    void setBorderTop(LayoutUnit t)
    {
        m_border.setTop(t);
    }

    void setBorderRight(LayoutUnit t)
    {
        m_border.setRight(t);
    }

    void setBorderBottom(LayoutUnit t)
    {
        m_border.setBottom(t);
    }

    void setBorderLeft(LayoutUnit t)
    {
        m_border.setLeft(t);
    }

    LayoutUnit borderTop() const
    {
        return m_border.top();
    }

    LayoutUnit borderRight() const
    {
        return m_border.right();
    }

    LayoutUnit borderBottom() const
    {
        return m_border.bottom();
    }

    LayoutUnit borderLeft() const
    {
        return m_border.left();
    }

    void setMarginTop(LayoutUnit t)
    {
        m_margin.setTop(t);
    }

    void setMarginRight(LayoutUnit t)
    {
        m_margin.setRight(t);
    }

    void setMarginBottom(LayoutUnit t)
    {
        m_margin.setBottom(t);
    }

    void setMarginLeft(LayoutUnit t)
    {
        m_margin.setLeft(t);
    }

    LayoutUnit marginTop() const
    {
        return m_margin.top();
    }

    LayoutUnit marginRight() const
    {
        return m_margin.right();
    }

    LayoutUnit marginBottom() const
    {
        return m_margin.bottom();
    }

    LayoutUnit marginLeft() const
    {
        return m_margin.left();
    }

    LayoutUnit paddingWidth() const
    {
        return m_padding.left() + m_padding.right();
    }

    LayoutUnit paddingHeight() const
    {
        return m_padding.top() + m_padding.bottom();
    }

    LayoutUnit borderWidth() const
    {
        return m_border.left() + m_border.right();
    }

    LayoutUnit borderHeight() const
    {
        return m_border.top() + m_border.bottom();
    }

    LayoutUnit marginWidth() const
    {
        return m_margin.left() + m_margin.right();
    }

    LayoutUnit marginHeight() const
    {
        return m_margin.top() + m_margin.bottom();
    }

    virtual LayoutUnit leftMBPWidth()
    {
        return m_margin.left() + m_border.left() + m_padding.left();
    }

    virtual LayoutUnit rightMBPWidth()
    {
        return m_margin.right() + m_border.right() + m_padding.right();
    }

    LayoutUnit mbpWidth()
    {
        return marginWidth() + borderWidth() + paddingWidth();
    }

    LayoutUnit contentWidth() const
    {
        return m_frameRect.width() - paddingWidth() - borderWidth();
    }

    LayoutUnit contentHeight() const
    {
        return m_frameRect.height() - paddingHeight() - borderHeight();
    }

    LayoutUnit boxWidth() const
    {
        LayoutUnit boxWidth = width() + marginWidth();
        if (boxWidth < 0) {
            return 0;
        }
        return boxWidth;
    }

    LayoutUnit boxHeight() const
    {
        LayoutUnit boxHeight = height() + marginHeight();
        if (boxHeight < 0) {
            return 0;
        }
        return boxHeight;
    }

    LayoutUnit lineHeight();

    void setMarginCollapseResult(const MarginCollapseResult& r)
    {
        m_marginCollapseResult = r;
    }

    const MarginCollapseResult& marginCollapseResult()
    {
        return m_marginCollapseResult;
    }

    virtual void paintChildrenWith(PaintingContext& ctx)
    {
        Frame* child = firstChild();
        while (child) {
            ctx.m_canvas->save();
            ctx.m_canvas->translate(child->asFrameBox()->x(),
                                    child->asFrameBox()->y());
            child->paint(ctx);
            ctx.m_canvas->restore();
            child = child->next();
        }
    }

    static void paintBackground(Canvas* canvas, ComputedStyle* style,
                                LayoutRect imageRect, LayoutRect colorRect,
                                bool isRootElement)
    {
        if (!style->backgroundColor().isTransparent() &&
            style->visibility() == VisibilityValue::VisibleVisibilityValue) {
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
                canvas->clip(
                    Unit::Rect(0, 0, colorRect.width(), colorRect.height()));
            } else {
                canvas->translate(imageRect.x(), imageRect.y());
                canvas->clip(
                    Unit::Rect(0, 0, imageRect.width(), imageRect.height()));
            }

            float bw = imageRect.width();
            float bh = imageRect.height();

            float w = bw;
            float h = bh;

            float boxR = bw / bh;
            float imgR = id->width() / (float)id->height();
            if (style->bgSizeType() == BackgroundSizeType::Cover) {
                if (boxR < imgR) {
                    w = bh * imgR;
                } else {
                    h = bw / imgR;
                }
            } else if (style->bgSizeType() == BackgroundSizeType::Contain) {
                if (boxR > imgR) {
                    w = bh * imgR;
                } else {
                    h = bw / imgR;
                }
            } else if (style->bgSizeType() == BackgroundSizeType::SizeValue) {
                if (style->bgSizeValue().width().isAuto() &&
                    style->bgSizeValue().height().isAuto()) {
                    w = id->width();
                    h = id->height();
                } else if (style->bgSizeValue().width().isAuto() &&
                           !style->bgSizeValue().height().isAuto()) {
                    h = style->bgSizeValue().height().specifiedValue(bh);
                    w = h * id->width() / id->height();
                } else if (!style->bgSizeValue().width().isAuto() &&
                           style->bgSizeValue().height().isAuto()) {
                    w = style->bgSizeValue().width().specifiedValue(bw);
                    h = w * id->height() / id->width();
                } else {
                    w = style->bgSizeValue().width().specifiedValue(bw);
                    h = style->bgSizeValue().height().specifiedValue(bh);
                }
            } else {
                STARFISH_ASSERT(style->bgSizeType() ==
                                BackgroundSizeType::SizeNone);
                STARFISH_ASSERT_NOT_REACHED();
            }

            LayoutUnit x = style->backgroundPositionX().specifiedValue(bw - w);
            LayoutUnit y = style->backgroundPositionY().specifiedValue(bh - h);

            if (isRootElement) {
                x += imageRect.x();
                y += imageRect.y();
                bw = colorRect.width();
                bh = colorRect.height();
            }

            auto repeatX = style->backgroundRepeatX();
            auto repeatY = style->backgroundRepeatY();
            if (repeatX == BackgroundRepeatValue::RepeatRepeatValue &&
                repeatY == BackgroundRepeatValue::RepeatRepeatValue) {
                canvas->drawRepeatImage(id, Unit::Rect(x, y, bw, bh), w, h,
                                        true, true, isRootElement);
            } else if (repeatX == BackgroundRepeatValue::NoRepeatRepeatValue &&
                       repeatY == BackgroundRepeatValue::RepeatRepeatValue) {
                canvas->drawRepeatImage(id, Unit::Rect(x, y, w, bh), w, h,
                                        false, true, isRootElement);
            } else if (repeatX == BackgroundRepeatValue::RepeatRepeatValue &&
                       repeatY == BackgroundRepeatValue::NoRepeatRepeatValue) {
                canvas->drawRepeatImage(id, Unit::Rect(x, y, bw, h), w, h, true,
                                        false, isRootElement);
            } else {
                canvas->drawImage(id, Unit::Rect(x, y, w, h));
            }

            canvas->restore();
        }
    }

    virtual void paintBackgroundAndBorders(Canvas* canvas);

    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage)
    {
        if (x >= 0 && x < m_frameRect.width() && y >= 0 &&
            y < m_frameRect.height()) {
            return this;
        }
        return nullptr;
    }

    virtual Frame* hitTestChildrenWith(LayoutUnit x, LayoutUnit y,
                                       HitTestStage stage)
    {
        Frame* child = lastChild();
        Frame* result = nullptr;
        while (child) {
            LayoutUnit cx = x - child->asFrameBox()->x();
            LayoutUnit cy = y - child->asFrameBox()->y();
            result = child->hitTest(cx, cy, stage);
            if (result) {
                return result;
            }
            child = child->previous();
        }
        return result;
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
        if (style()->paddingLeft().isSpecified() &&
            !m_flags.m_isLeftMBPCleared) {
            setPaddingLeft(
                style()->paddingLeft().specifiedValue(parentContentWidth));
        } else {
            setPaddingLeft(0);
        }
        if (style()->paddingTop().isSpecified()) {
            setPaddingTop(
                style()->paddingTop().specifiedValue(parentContentWidth));
        } else {
            setPaddingTop(0);
        }
        if (style()->paddingRight().isSpecified() &&
            !m_flags.m_isRightMBPCleared) {
            setPaddingRight(
                style()->paddingRight().specifiedValue(parentContentWidth));
        } else {
            setPaddingRight(0);
        }
        if (style()->paddingBottom().isSpecified()) {
            setPaddingBottom(
                style()->paddingBottom().specifiedValue(parentContentWidth));
        } else {
            setPaddingBottom(0);
        }

        // border
        if (style()->hasBorderStyle()) {
            if (style()->borderLeftWidth().isSpecified() &&
                !m_flags.m_isLeftMBPCleared) {
                setBorderLeft(style()->borderLeftWidth().specifiedValue(
                    parentContentWidth));
            } else {
                setBorderLeft(0);
            }
            if (style()->borderTopWidth().isSpecified()) {
                setBorderTop(style()->borderTopWidth().specifiedValue(
                    parentContentWidth));
            } else {
                setBorderTop(0);
            }
            if (style()->borderRightWidth().isSpecified() &&
                !m_flags.m_isRightMBPCleared) {
                setBorderRight(style()->borderRightWidth().specifiedValue(
                    parentContentWidth));
            } else {
                setBorderRight(0);
            }
            if (style()->borderBottomWidth().isSpecified()) {
                setBorderBottom(style()->borderBottomWidth().specifiedValue(
                    parentContentWidth));
            } else {
                setBorderBottom(0);
            }
        } else {
            setBorderLeft(0);
            setBorderTop(0);
            setBorderRight(0);
            setBorderBottom(0);
        }

        // margin
        if (style()->marginLeft().isSpecified() &&
            !m_flags.m_isLeftMBPCleared) {
            setMarginLeft(
                style()->marginLeft().specifiedValue(parentContentWidth));
        } else {
            setMarginLeft(0);
        }
        if (style()->marginTop().isSpecified()) {
            setMarginTop(
                style()->marginTop().specifiedValue(parentContentWidth));
        } else {
            setMarginTop(0);
        }
        if (style()->marginRight().isSpecified() &&
            !m_flags.m_isRightMBPCleared) {
            setMarginRight(
                style()->marginRight().specifiedValue(parentContentWidth));
        } else {
            setMarginRight(0);
        }
        if (style()->marginBottom().isSpecified()) {
            setMarginBottom(
                style()->marginBottom().specifiedValue(parentContentWidth));
        } else {
            setMarginBottom(0);
        }
    }

    void computeHorizontalMargin(LayoutUnit parentContentWidth)
    {
        Length marginLeft = style()->marginLeft();
        Length marginRight = style()->marginRight();

        LayoutUnit remainedWidth = parentContentWidth - FrameBox::width();
        if (marginLeft.isAuto() && marginRight.isAuto()) {
            if (remainedWidth > 0) {
                setMarginLeft(remainedWidth / 2);
                setMarginRight(remainedWidth / 2);
            }
        } else if (marginLeft.isAuto() && !marginRight.isAuto()) {
            remainedWidth -= FrameBox::marginRight();
            if (remainedWidth > 0) {
                setMarginLeft(remainedWidth);
            }
        } else if (!marginLeft.isAuto() && marginRight.isAuto()) {
            remainedWidth -= FrameBox::marginLeft();
            if (remainedWidth > 0) {
                setMarginRight(remainedWidth);
            }
        }
    }

    void applyHorizontalMargin(bool isOpposite = false)
    {
        STARFISH_ASSERT(style()->position() == AbsolutePositionValue);
        if ((style()->direction() == LtrDirectionValue && !isOpposite) ||
            (style()->direction() == RtlDirectionValue && isOpposite)) {
            moveX(marginLeft());
        } else {
            moveX(-marginRight());
        }
    }

    void applyVerticalMargin()
    {
        STARFISH_ASSERT(style()->position() == AbsolutePositionValue);
        Length marginTop = style()->marginTop();
        Length marginBottom = style()->marginBottom();

        if (!marginTop.isAuto() && !marginBottom.isAuto()) {
            moveY(FrameBox::marginTop());
        } else if (!marginTop.isAuto() && marginBottom.isAuto()) {
            moveY(FrameBox::marginTop());
        } else if (marginTop.isAuto() && !marginBottom.isAuto()) {
            moveY(-FrameBox::marginBottom());
        }
    }

    virtual InlineNonReplacedBox* firstInlineNonReplacedBox(FrameInline* f)
    {
        return nullptr;
    }

    virtual void establishesStackingContextIfNeeds()
    {
        if (isEstablishesStackingContext()) {
            STARFISH_ASSERT(isRootElement() || m_stackingContext == nullptr);
            if (!isRootElement()) {
                FrameBox* p = layoutParent()->asFrameBox();
                while (true) {
                    if (p->isEstablishesStackingContext()) {
                        if (p->isRootElement()) {
                            break;
                        }
                        if (p->needsGraphicsBuffer()) {
                            break;
                        }
                        if (!p->isPositioned()) {
                            break;
                        }
                        if (p->style()->IsSpecifiedZIndex()) {
                            break;
                        }
                    }
                    p = p->layoutParent()->asFrameBox();
                }
                m_stackingContext =
                    new StackingContext(this, p->stackingContext());
            } else {
                m_stackingContext = new StackingContext(this, nullptr);
            }
        }
    }

    virtual void computeVisibleRect(StackingContext* sCtx, LayoutLocation& loc);

    bool tryUniteVisibleRect(StackingContext* sCtx, LayoutLocation& loc);

    void clearStackingContextIfNeeds(bool shouldDetachNativeBuffer = true)
    {
        if (m_stackingContext) {
            m_stackingContext->clearOwnBuffer(shouldDetachNativeBuffer);
            m_stackingContext = nullptr;
        }
    }

    virtual void paintStackingContextContent(Canvas* canvas);
    virtual void willCompsiteStackingContext(Canvas* c)
    {
    }

    virtual void didCompsiteStackingContext(Canvas* c)
    {
    }

    // this callback only called (establishesStackingContext &&
    // !needsGraphicsBuffer)
    virtual void compsitingStackingContext(Canvas* c)
    {
    }

    void setInlineBoxIndex(size_t inlineBoxIdx)
    {
        m_inlineBoxIndex = inlineBoxIdx;
    }

    size_t inlineBoxIndex()
    {
        return m_inlineBoxIndex;
    }

protected:
    LayoutUnit minMaxWidthAppliedIfNeeds(LayoutUnit width,
                                         LayoutUnit parentWidth,
                                         bool parentHasFixedValue)
    {
        ComputedStyle* style = Frame::style();
        if (style->minWidth().isSpecified()) {
            if (!parentHasFixedValue && style->minWidth().isPercent()) {
                return width;
            }
            LayoutUnit minWidth = style->minWidth().specifiedValue(parentWidth);
            if (minWidth > width) {
                return minWidth;
            }
        }
        if (style->maxWidth().isSpecified()) {
            if (!parentHasFixedValue && style->maxWidth().isPercent()) {
                return width;
            }
            LayoutUnit maxWidth = style->maxWidth().specifiedValue(parentWidth);
            if (maxWidth < width) {
                return maxWidth;
            }
        }
        return width;
    }

    LayoutUnit minMaxHeightAppliedIfNeeds(LayoutUnit height,
                                          LayoutUnit parentHeight,
                                          bool parentHasFixedValue)
    {
        ComputedStyle* style = Frame::style();
        if (style->minHeight().isSpecified()) {
            if (!parentHasFixedValue && style->minHeight().isPercent()) {
                return height;
            }
            LayoutUnit minHeight =
                style->minHeight().specifiedValue(parentHeight);
            if (minHeight > height) {
                return minHeight;
            }
        }
        if (style->maxHeight().isSpecified()) {
            if (!parentHasFixedValue && style->maxHeight().isPercent()) {
                return height;
            }
            LayoutUnit maxHeight =
                style->maxHeight().specifiedValue(parentHeight);
            if (maxHeight < height) {
                return maxHeight;
            }
        }
        return height;
    }
    // content + padding + border
    LayoutRect m_frameRect;

    LayoutBoxSurroundData m_padding, m_border, m_margin;
    MarginCollapseResult m_marginCollapseResult;
    StackingContext* m_stackingContext;

    size_t m_inlineBoxIndex;
};
}

#endif
