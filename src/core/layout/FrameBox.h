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
class CanvasSurface;
class Compositor;
class LineBox;
class HTMLElement;

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

#define FRAMEBOX_RAREDATA_TAG 0x3
struct FrameBoxRareData : public gc {
    size_t m_frameBoxRareDataTag;
    Frame* m_layoutParent;
    LayoutBoxSurroundData m_padding, m_border, m_margin;
    StackingContext* m_stackingContext;
#if defined(PORT_GRAPHIC_BACKEND_EFL)
    NativeImageData* m_bufferForBorderRadius;
#endif

    FrameBoxRareData(Frame* layoutParent)
        : m_frameBoxRareDataTag(FRAMEBOX_RAREDATA_TAG)
        , m_layoutParent(layoutParent)
        , m_stackingContext(nullptr)
#if defined(PORT_GRAPHIC_BACKEND_EFL)
        , m_bufferForBorderRadius(nullptr)
#endif
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;
};

enum PaintingInlineStage {
    PaintingInlineBox,
    PaintingAtomicInlineBoxButInlineReplaced,
    PaintingInlineReplaced,
    PaintingInlineStageEnd
};

class FrameBox : public Frame {
    friend struct MBPRestorer;
    friend struct MinMaxWidthHeightRestorer;

public:
    FrameBox(Node* node, ComputedStyle* style)
        : Frame(node, style)
        , m_layoutParent(nullptr)
        , m_frameRect(0, 0, 0, 0)
    {
    }

    virtual bool isFrameBox() const
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
        if (hasRareData())
            return frameBoxRareData()->m_stackingContext;
        return nullptr;
    }

    const LayoutRect& frameRect()
    {
        return m_frameRect;
    }

    // FrameRect including outline, box-shadow
    LayoutRect frameVisibleRect();

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

    LayoutUnit minMaxWidthAppliedIfNeeds(LayoutContext& ctx, LayoutUnit width,
                                         LayoutUnit parentWidth,
                                         bool underComputingPreferredWidth);

    LayoutUnit minMaxHeightAppliedIfNeeds(LayoutContext& ctx, LayoutUnit height,
                                          LayoutUnit parentHeight,
                                          bool parentHasFixedValue);

    void applyMinMaxWidthIfNeeds(LayoutContext& ctx, LayoutUnit width,
                                 LayoutUnit parentWidth)
    {
        setContentWidth(
            minMaxWidthAppliedIfNeeds(ctx, width, parentWidth, false));
    }

    void applyMinMaxHeightIfNeeds(LayoutContext& ctx, LayoutUnit height,
                                  LayoutUnit parentHeight,
                                  bool parentHasFixedValue = true)
    {
        setContentHeight(minMaxHeightAppliedIfNeeds(ctx, height, parentHeight,
                                                    parentHasFixedValue));
    }

    LayoutUnit contentWidthApplyingBoxSizing(LayoutUnit width)
    {
        if (style()->boxSizing() == BoxSizingValue::BorderBoxBoxSizingValue) {
            return std::max(width - paddingWidth() - borderWidth(),
                            LayoutUnit(0));
        }

        return width;
    }

    LayoutUnit contentHeightApplyingBoxSizing(LayoutUnit height)
    {
        if (style()->boxSizing() == BoxSizingValue::BorderBoxBoxSizingValue) {
            return std::max(height - paddingHeight() - borderHeight(),
                            LayoutUnit(0));
        }

        return height;
    }

    enum CopyFlag {
        PositionCopy = 1 << 0,
        WidthAndHeightCopy = 1 << 1,
        MarginCopy = 1 << 2,
        BorderCopy = 1 << 3,
        PaddingCopy = 1 << 4,
        BorderBoxCopy = WidthAndHeightCopy | BorderCopy | PaddingCopy
    };

    enum BoxSide { TopSide, RightSide, BottomSide, LeftSide };

    void copyFrom(FrameBox* box, uint8_t flag)
    {
        bool copyPosition = flag & CopyFlag::PositionCopy;
        bool copyWidthAndHeight = flag & CopyFlag::WidthAndHeightCopy;
        bool copyMargin = flag & CopyFlag::MarginCopy;
        bool copyBorder = flag & CopyFlag::BorderCopy;
        bool copyPadding = flag & CopyFlag::PaddingCopy;

        if (copyPosition) {
            setX(box->x());
            setY(box->y());
        }

        if (copyWidthAndHeight) {
            setWidth(box->width());
            setHeight(box->height());
        }

        if (copyMargin) {
            setMarginTop(box->marginTop());
            setMarginBottom(box->marginBottom());
            setMarginLeft(box->marginLeft());
            setMarginRight(box->marginRight());
        }

        if (copyBorder) {
            setBorderTop(box->borderTop());
            setBorderBottom(box->borderBottom());
            setBorderLeft(box->borderLeft());
            setBorderRight(box->borderRight());
        }

        if (copyPadding) {
            setPaddingTop(box->paddingTop());
            setPaddingBottom(box->paddingBottom());
            setPaddingLeft(box->paddingLeft());
            setPaddingRight(box->paddingRight());
        }
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
        if (!hasRareData() && t == 0) {
            return;
        }
        ensureFrameBoxRareData()->m_padding.setTop(t);
    }

    void setPaddingRight(LayoutUnit t)
    {
        if (!hasRareData() && t == 0) {
            return;
        }
        ensureFrameBoxRareData()->m_padding.setRight(t);
    }

    void setPaddingBottom(LayoutUnit t)
    {
        if (!hasRareData() && t == 0) {
            return;
        }
        ensureFrameBoxRareData()->m_padding.setBottom(t);
    }

    void setPaddingLeft(LayoutUnit t)
    {
        if (!hasRareData() && t == 0) {
            return;
        }
        ensureFrameBoxRareData()->m_padding.setLeft(t);
    }

    LayoutUnit paddingTop() const
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_padding.top();
        }
        return 0;
    }

    LayoutUnit paddingRight() const
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_padding.right();
        }
        return 0;
    }

    LayoutUnit paddingBottom() const
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_padding.bottom();
        }
        return 0;
    }

    LayoutUnit paddingLeft() const
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_padding.left();
        }
        return 0;
    }

    void setBorderTop(LayoutUnit t)
    {
        if (!hasRareData() && t == 0) {
            return;
        }
        ensureFrameBoxRareData()->m_border.setTop(t);
    }

    void setBorderRight(LayoutUnit t)
    {
        if (!hasRareData() && t == 0) {
            return;
        }
        ensureFrameBoxRareData()->m_border.setRight(t);
    }

    void setBorderBottom(LayoutUnit t)
    {
        if (!hasRareData() && t == 0) {
            return;
        }
        ensureFrameBoxRareData()->m_border.setBottom(t);
    }

    void setBorderLeft(LayoutUnit t)
    {
        if (!hasRareData() && t == 0) {
            return;
        }
        ensureFrameBoxRareData()->m_border.setLeft(t);
    }

    LayoutUnit borderTop() const
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_border.top();
        }
        return 0;
    }

    LayoutUnit borderRight() const
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_border.right();
        }
        return 0;
    }

    LayoutUnit borderBottom() const
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_border.bottom();
        }
        return 0;
    }

    LayoutUnit borderLeft() const
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_border.left();
        }
        return 0;
    }

    void setMarginTop(LayoutUnit t)
    {
        if (!hasRareData() && t == 0) {
            return;
        }
        ensureFrameBoxRareData()->m_margin.setTop(t);
    }

    void setMarginRight(LayoutUnit t)
    {
        if (!hasRareData() && t == 0) {
            return;
        }
        ensureFrameBoxRareData()->m_margin.setRight(t);
    }

    void setMarginBottom(LayoutUnit t)
    {
        if (!hasRareData() && t == 0) {
            return;
        }
        ensureFrameBoxRareData()->m_margin.setBottom(t);
    }

    void setMarginLeft(LayoutUnit t)
    {
        if (!hasRareData() && t == 0) {
            return;
        }
        ensureFrameBoxRareData()->m_margin.setLeft(t);
    }

    LayoutUnit marginTop() const
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_margin.top();
        }
        return 0;
    }

    LayoutUnit marginRight() const
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_margin.right();
        }
        return 0;
    }

    LayoutUnit marginBottom() const
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_margin.bottom();
        }
        return 0;
    }

    LayoutUnit marginLeft() const
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_margin.left();
        }
        return 0;
    }

    LayoutUnit paddingWidth() const
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_padding.left() +
                   frameBoxRareData()->m_padding.right();
        }
        return 0;
    }

    LayoutUnit paddingHeight() const
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_padding.top() +
                   frameBoxRareData()->m_padding.bottom();
        }
        return 0;
    }

    LayoutUnit borderWidth() const
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_border.left() +
                   frameBoxRareData()->m_border.right();
        }
        return 0;
    }

    LayoutUnit borderHeight() const
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_border.top() +
                   frameBoxRareData()->m_border.bottom();
        }
        return 0;
    }

    LayoutUnit marginWidth() const
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_margin.left() +
                   frameBoxRareData()->m_margin.right();
        }
        return 0;
    }

    LayoutUnit marginHeight() const
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_margin.top() +
                   frameBoxRareData()->m_margin.bottom();
        }
        return 0;
    }

    virtual LayoutUnit leftMBPWidth()
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_margin.left() +
                   frameBoxRareData()->m_border.left() +
                   frameBoxRareData()->m_padding.left();
        } else {
            return 0;
        }
    }

    virtual LayoutUnit rightMBPWidth()
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_margin.right() +
                   frameBoxRareData()->m_border.right() +
                   frameBoxRareData()->m_padding.right();
        } else {
            return 0;
        }
    }

    LayoutUnit startingMBPWidth()
    {
        LayoutUnit w;
        if (style()->direction() == LtrDirectionValue) {
            w = leftMBPWidth();
        } else {
            w = rightMBPWidth();
        }
        return w;
    }

    LayoutUnit endingMBPWidth()
    {
        LayoutUnit w;
        if (style()->direction() == LtrDirectionValue) {
            w = rightMBPWidth();
        } else {
            w = leftMBPWidth();
        }
        return w;
    }

    LayoutUnit mbpWidth()
    {
        return marginWidth() + borderWidth() + paddingWidth();
    }

    LayoutUnit contentWidth() const
    {
        return m_frameRect.width() - paddingWidth() - borderWidth();
    }

    LayoutUnit mbpHeight()
    {
        return marginHeight() + borderHeight() + paddingHeight();
    }

    LayoutUnit contentHeight() const
    {
        return m_frameRect.height() - paddingHeight() - borderHeight();
    }

    HorizontalDataLocToContainingBlock computeHorizontalDataToContainingBlock(
        LayoutContext& ctx, FrameBox* cb);
    VerticalDataLocToContainingBlock computeVerticalDataToContainingBlock(
        LayoutContext& ctx, FrameBox* cb);
    void moveToStaticPositionForAbsolutedPositionedBoxHorizontally(
        FrameBox* box);
    void moveToStaticPositionForAbsolutedPositionedBoxVertically(FrameBox* box);

    LayoutUnit outerWidth() const
    {
        LayoutUnit boxWidth = width() + marginWidth();
        if (boxWidth < 0) {
            return 0;
        }
        return boxWidth;
    }

    LayoutUnit outerHeight() const
    {
        LayoutUnit outerHeight = height() + marginHeight();
        if (outerHeight < 0) {
            return 0;
        }
        return outerHeight;
    }

    virtual void paintContent(PaintingContext& ctx);
    virtual void paintInlineContent(Canvas* canvas, PaintingInlineStage stage,
                                    LayoutUnit dx, LayoutUnit dy)
    {
    }

    bool canSkipPaintingStage(PaintingContext& ctx)
    {
        if (isEstablishesStackingContext()) {
            return true;
        }

        if (ctx.m_paintingStage == PaintingNormalFlowBlock) {
            if (!m_flags.m_seenNormalFlowBlockChild) {
                return true;
            }
        } else if (ctx.m_paintingStage == PaintingNonPositionedFloats) {
            if (!m_flags.m_seenNonPositionedFloats) {
                return true;
            }
        } else if (ctx.m_paintingStage == PaintingReplacedBlock) {
            if (!m_flags.m_seenReplacedBlock) {
                return true;
            }
        } else {
            STARFISH_ASSERT(ctx.m_paintingStage == PaintingNormalFlowInline);
            if (!m_flags.m_seenNormalFlowInline) {
                return true;
            }
        }

        return false;
    }

    virtual void paintChildrenWith(PaintingContext& ctx);
    virtual void paintBackgroundAndBorders(Canvas* canvas);

    virtual Unit::Rect makeRect(BoxValue box);
    static void paintBackground(Canvas* canvas, FrameBox* box,
                                HTMLElement* rootOrBodyelement);
    void paintBorders(Canvas* canvas, const LayoutRect& rect);
    void paintOutline(Canvas* canvas);
    void paintDashedLine(Canvas* canvas, const LayoutLocation& p1,
                         const LayoutLocation& p2, const LayoutLocation& p3,
                         const LayoutLocation& p4, const LayoutLocation& p5,
                         const LayoutLocation& p6, BoxSide side);

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
        if (shouldApplyOverflow()) {
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

    LayoutLocation absolutePointIncludingScroll(FrameBox* top);
    LayoutRect absoluteRectIncludingScroll(FrameBox* top)
    {
        return LayoutRect(absolutePointIncludingScroll(top),
                          frameRect().size());
    }

    void computeBorderMarginPadding(LayoutContext& ctx,
                                    LayoutUnit parentContentWidth);

    void computeHorizontalMargin(LayoutUnit parentContentWidth,
                                 DirectionValue parentDirection);
    void computeVerticalMargin(LayoutUnit parentContentHeight);

    virtual InlineNonReplacedBox* firstInlineNonReplacedBox(FrameInline* f)
    {
        return nullptr;
    }

    virtual void establishesStackingContextIfNeeds();
    virtual void computeVisibleRect(Frame::ComputeVisibleRectContext& ctx);

    bool tryUniteVisibleRect(Frame::ComputeVisibleRectContext& ctx);

    void clearStackingContextIfNeeds(bool shouldDetachNativeBuffer = true);

    virtual void paintStackingContextContent(Canvas* canvas);
    virtual void willCompsiteStackingContext(Compositor* c)
    {
    }

    virtual void didCompsiteStackingContext(Compositor* c)
    {
    }

    // this callback only called (establishesStackingContext &&
    // !needsGraphicsBuffer)
    virtual void compsitingStackingContext(Compositor* c)
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
        setLayoutParent(f);
    }

    void setLayoutParent(Frame* f)
    {
        if (hasRareData()) {
            frameBoxRareData()->m_layoutParent = f;
        } else {
            m_layoutParent = f;
        }
    }

    virtual Frame* layoutParent() const
    {
        if (hasRareData()) {
            return frameBoxRareData()->m_layoutParent;
        }
        return m_layoutParent;
    }

    virtual void createGraphicsBuffer(CanvasSurface** surfaceHolder,
                                      size_t visibleWidth, size_t visibleHeight)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return;
    }

    virtual bool hasOwnGraphicsBufferMethod()
    {
        return false;
    }

protected:
    void applyBorderRadiusClippingIfNeeds(Canvas* canvas);

    bool hasRareData() const
    {
        size_t* ptr = (size_t*)m_layoutParent;
        if (ptr && *ptr == FRAMEBOX_RAREDATA_TAG) {
            return true;
        }
        return false;
    }

    virtual FrameBoxRareData* createRareData()
    {
        return new FrameBoxRareData(m_layoutParent);
    }

    FrameBoxRareData* ensureFrameBoxRareData()
    {
        if (hasRareData()) {
            return (FrameBoxRareData*)m_layoutParent;
        }
        m_layoutParent = (Frame*)createRareData();
        return (FrameBoxRareData*)m_layoutParent;
    }

    FrameBoxRareData* frameBoxRareData() const
    {
        STARFISH_ASSERT(hasRareData());
        return (FrameBoxRareData*)m_layoutParent;
    }

    // outline-width + outline-offset
    LayoutUnit outlineThickness();

    Frame* m_layoutParent;

    // content + padding + border
    LayoutRect m_frameRect;
};

struct MBPRestorer {
    MBPRestorer(FrameBox* b)
        : m_box(b)
        , m_hasRareData(b->hasRareData())
        , m_paddingWidthDamaged(b->paddingWidthDamaged())
        , m_paddingHeightDamaged(b->paddingHeightDamaged())
    {
        if (m_hasRareData) {
            m_margin = m_box->frameBoxRareData()->m_margin;
            m_border = m_box->frameBoxRareData()->m_border;
            m_padding = m_box->frameBoxRareData()->m_padding;
        }
    }

    ~MBPRestorer()
    {
        if (m_hasRareData) {
            m_box->frameBoxRareData()->m_margin = m_margin;
            m_box->frameBoxRareData()->m_border = m_border;
            m_box->frameBoxRareData()->m_padding = m_padding;
        }

        m_box->m_flags.m_paddingWidthDamaged = m_paddingWidthDamaged;
        m_box->m_flags.m_paddingHeightDamaged = m_paddingHeightDamaged;
    }

    FrameBox* m_box;
    bool m_hasRareData;
    bool m_paddingWidthDamaged;
    bool m_paddingHeightDamaged;
    LayoutBoxSurroundData m_margin;
    LayoutBoxSurroundData m_border;
    LayoutBoxSurroundData m_padding;
};
}

#endif
