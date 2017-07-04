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

    void applyHorizontalMargin(bool isOpposite = false);

    void applyVerticalMargin();

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

    void setInlineBoxIndex(size_t inlineBoxIdx)
    {
        m_inlineBoxIndex = inlineBoxIdx;
    }

    size_t inlineBoxIndex()
    {
        return m_inlineBoxIndex;
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

protected:
    LayoutUnit minMaxWidthAppliedIfNeeds(LayoutUnit width,
                                         LayoutUnit parentWidth,
                                         bool parentHasFixedValue);

    LayoutUnit minMaxHeightAppliedIfNeeds(LayoutUnit height,
                                          LayoutUnit parentHeight,
                                          bool parentHasFixedValue);
    // content + padding + border
    LayoutRect m_frameRect;

    LayoutBoxSurroundData m_padding, m_border, m_margin;
    MarginCollapseResult m_marginCollapseResult;
    StackingContext* m_stackingContext;

    size_t m_inlineBoxIndex;
};
}

#endif
