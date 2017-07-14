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

#include "core/layout/Frame.h"

namespace StarFish {

class Canvas;
class LineBox;

struct HorizontalDataLocToContainingBlock {
    LayoutUnit m_contentWidth;
    LayoutUnit m_absX;
    LayoutUnit m_left;
    LayoutUnit m_right;

    HorizontalDataLocToContainingBlock(LayoutUnit contentWidth, LayoutUnit absX,
                                       LayoutUnit left, LayoutUnit right)
        : m_contentWidth(contentWidth)
        , m_absX(absX)
        , m_left(left)
        , m_right(right)
    {
    }
};

struct VerticalDataLocToContainingBlock {
    LayoutUnit m_contentHeight;
    LayoutUnit m_absY;
    LayoutUnit m_top;
    LayoutUnit m_bottom;

    VerticalDataLocToContainingBlock(LayoutUnit contentHeight, LayoutUnit absY,
                                     LayoutUnit top, LayoutUnit bottom)
        : m_contentHeight(contentHeight)
        , m_absY(absY)
        , m_top(top)
        , m_bottom(bottom)
    {
    }
};

class FrameBox : public Frame {
public:
    FrameBox(Node* node, ComputedStyle* style)
        : Frame(node, style)
        , m_layoutParent(nullptr)
        , m_frameRect(0, 0, 0, 0)
        , m_padding()
        , m_border()
        , m_margin()
        , m_stackingContext(nullptr)
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

    void applyMinMaxWidthIfNeeds(LayoutUnit width, LayoutUnit parentWidth)
    {
        setContentWidth(minMaxWidthAppliedIfNeeds(width, parentWidth));
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

    HorizontalDataLocToContainingBlock computeHorizontalDataToContainingBlock(
        LayoutContext& ctx, FrameBox* cb);
    VerticalDataLocToContainingBlock computeVerticalDataToContainingBlock(
        LayoutContext& ctx, FrameBox* cb);

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

    virtual void paintChildrenWith(PaintingContext& ctx);

    static void paintBackground(Canvas* canvas, ComputedStyle* style,
                                LayoutRect imageRect, LayoutRect colorRect,
                                bool isRootElement = false,
                                bool needsToFillBgColorAtBorderBox = false);

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
        if (style()->overflow() != OverflowValue::VisibleOverflow) {
            if (FrameBox::hitTest(x, y, HitTestStageEnd) == nullptr) {
                return nullptr;
            }
        }

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

    void computeBorderMarginPadding(LayoutUnit parentContentWidth);

    void computeHorizontalMargin(LayoutUnit parentContentWidth);

    void applyVerticalMarginForAbsoluteBox();

    virtual InlineNonReplacedBox* firstInlineNonReplacedBox(FrameInline* f)
    {
        return nullptr;
    }

    virtual void establishesStackingContextIfNeeds();
    virtual void computeVisibleRect(StackingContext* sCtx, LayoutLocation& loc);

    bool tryUniteVisibleRect(StackingContext* sCtx, LayoutLocation& loc);

    void clearStackingContextIfNeeds(bool shouldDetachNativeBuffer = true);

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

    virtual void iterateChildFrameBox(const std::function<void(FrameBox*)>& fn)
    {
        fn(this);
        Frame* box = firstChild();
        while (box) {
            box->asFrameBox()->iterateChildFrameBox(fn);
            box = box->next();
        }
    }

    virtual void setParent(Frame* f)
    {
        Frame::setParent(f);
        m_layoutParent = f;
    }

    void setLayoutParent(Frame* f)
    {
        m_layoutParent = f;
    }

    virtual Frame* layoutParent() const
    {
        return m_layoutParent;
    }

protected:
    LayoutUnit minMaxWidthAppliedIfNeeds(LayoutUnit width,
                                         LayoutUnit parentWidth);

    LayoutUnit minMaxHeightAppliedIfNeeds(LayoutUnit height,
                                          LayoutUnit parentHeight,
                                          bool parentHasFixedValue);
    Frame* m_layoutParent;

    // content + padding + border
    LayoutRect m_frameRect;

    LayoutBoxSurroundData m_padding, m_border, m_margin;
    StackingContext* m_stackingContext;
};
}

#endif
