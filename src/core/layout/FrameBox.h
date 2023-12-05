/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifndef __StarfishFrameBox__
#define __StarfishFrameBox__

#include "core/layout/Frame.h"

namespace Starfish {

class Canvas;
class CanvasSurface;
class Compositor;
class LineBox;
class HTMLElement;

struct HorizontalInfoForAbsoluteBlockBox {
    LayoutUnit m_contentWidth;
    LayoutUnit m_absX;
    LayoutUnit m_left;
    LayoutUnit m_right;

    HorizontalInfoForAbsoluteBlockBox(LayoutUnit contentWidth, LayoutUnit absX,
                                      LayoutUnit left, LayoutUnit right)
        : m_contentWidth(contentWidth)
        , m_absX(absX)
        , m_left(left)
        , m_right(right)
    {
    }
};

struct VerticalInfoForAbsoluteBlockBox {
    LayoutUnit m_contentHeight;
    LayoutUnit m_absY;
    LayoutUnit m_top;
    LayoutUnit m_bottom;

    VerticalInfoForAbsoluteBlockBox(LayoutUnit contentHeight, LayoutUnit absY,
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

    FrameBoxRareData(Frame* layoutParent)
        : m_frameBoxRareDataTag(FRAMEBOX_RAREDATA_TAG)
        , m_layoutParent(layoutParent)
        , m_stackingContext(nullptr)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(FrameBoxRareData, m_layoutParent));
        GC_set_bit(desc, GC_WORD_OFFSET(FrameBoxRareData, m_stackingContext));
    }
};

enum PaintingInlineStage {
    PaintingInlineBox,
    PaintingAtomicInlineBoxButInlineReplaced,
    PaintingInlineReplaced,
    PaintingInlineStageEnd
};

LayoutRect computeBoxExtent(LayoutRect rt, const SkMatrix& m);

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
        printf(" LYTP[%p] ", layoutParent());
        printf("Box[%g,%g,%g,%g] ", x().toFloat(), y().toFloat(),
               width().toFloat(), height().toFloat());
        printf("PDD[%g,%g,%g,%g] ", paddingTop().toFloat(),
               paddingRight().toFloat(), paddingBottom().toFloat(),
               paddingLeft().toFloat());
        printf("BDR[%g,%g,%g,%g] ", borderTop().toFloat(),
               borderRight().toFloat(), borderBottom().toFloat(),
               borderLeft().toFloat());
        printf("MGN[%g,%g,%g,%g]", marginTop().toFloat(),
               marginRight().toFloat(), marginBottom().toFloat(),
               marginLeft().toFloat());
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
    LayoutRect frameVisibleOutlineRect(OutlineData* outline);
    LayoutRect frameVisibleShadowsRect(ShadowDataList* boxShadow);
    LayoutRect frameVisibleFilterRect(FilterFunctions* filter);

    LayoutRect frameScrollingRect();

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

    LayoutUnit widthAfterApplyingMinMaxWidths(
        LayoutContext& ctx, LayoutUnit width, LayoutUnit parentWidth,
        bool underComputingPreferredWidth);

    LayoutUnit heightAfterApplyingMinMaxHeights(LayoutContext& ctx,
                                                LayoutUnit height,
                                                LayoutUnit parentHeight,
                                                bool parentHasFixedValue);

    void setContentWidthConsideringMinMaxWidths(LayoutContext& ctx,
                                                LayoutUnit width,
                                                LayoutUnit parentWidth)
    {
        setContentWidth(
            widthAfterApplyingMinMaxWidths(ctx, width, parentWidth, false));
    }

    void setContentHeightConsideringMinMaxHeights(
        LayoutContext& ctx, LayoutUnit height, LayoutUnit parentHeight,
        bool parentHasFixedValue = true)
    {
        setContentHeight(heightAfterApplyingMinMaxHeights(
            ctx, height, parentHeight, parentHasFixedValue));
    }

    LayoutUnit contentWidthAfterApplyingBoxSizing(LayoutUnit width)
    {
        if (style()->boxSizing() == BoxSizingValue::BorderBoxBoxSizingValue) {
            return std::max(width - paddingWidth() - borderWidth(),
                            LayoutUnit(0));
        }

        return width;
    }

    LayoutUnit contentHeightAfterApplyingBoxSizing(LayoutUnit height)
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

    HorizontalInfoForAbsoluteBlockBox
    calHorizontalInfoRelativeToContainingBlock(LayoutContext& ctx,
                                               FrameBox* cb);
    VerticalInfoForAbsoluteBlockBox calVerticalInfoRelativeToContainingBlock(
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

    bool canOwnsStackingContext()
    {
        if (isRootElement()) {
            return true;
        } else if (needsGraphicsBuffer()) {
            return true;
        } else if (style()->hasTransforms(this)) {
            return true;
        } else if (style()->position() == FixedPositionValue) {
            return true;
        } else if ((isPositioned() || isFlexItem()) &&
                   style()->isSpecifiedZIndex()) {
            return true;
        } else if (style()->opacity() != 1 || isRunningOpacityAnimation()) {
            return true;
        } else if (style()->hasAvailableFilter()) {
            return true;
        } else if (style()->maskLayerSize()) {
            return true;
        }

        return false;
    }
    bool needsToPaintBackgroundOrBorderOrBoxShadow();
    virtual void paintContent(PaintingContext& ctx);
    virtual void paintInlineContent(Canvas* canvas, PaintingInlineStage stage,
                                    LayoutUnit dx, LayoutUnit dy)
    {
    }

    bool canSkipPaintingStage(PaintingContext& ctx)
    {
        if (needToEstablishStackingContext()) {
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
    virtual void paintBoxShadows(Canvas* canvas);
    virtual void paintInsetBoxShadows(Canvas* canvas);

    virtual Unit::Rect makeRect(BoxValue box);
    static void paintBackground(Canvas* canvas, FrameBox* box,
                                HTMLElement* rootOrBodyelement);
    static void paintGradient(Canvas* canvas, FrameBox* box,
                              ImageValue* imageValue, Unit::Rect dst,
                              const float& width, const float& height,
                              bool repeatX, bool repeatY,
                              ImageRenderingValue imageRenderingValue);
    void paintBorders(Canvas* canvas, const LayoutRect& rect);
    void paintOutline(Canvas* canvas);
    void paintDashedLine(Canvas* canvas, const LayoutLocation& p1,
                         const LayoutLocation& p2, const LayoutLocation& p3,
                         const LayoutLocation& p4, const LayoutLocation& p5,
                         const LayoutLocation& p6, BoxSide side);
    void paintDottedLine(Canvas* canvas, const LayoutLocation& p1,
                         const LayoutLocation& p2, const LayoutLocation& p3,
                         const LayoutLocation& p4, const LayoutLocation& p5,
                         const LayoutLocation& p6, BoxSide side);

    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage)
    {
        if (x >= 0 && x < m_frameRect.width() && y >= 0 &&
            y < m_frameRect.height()) {
            if (style()->pointerEvents() >=
                PointerEventsValue::PointerEventsAutoValue) {
                return this;
            }
            return nullptr;
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

    LayoutLocation absolutePointIncludingScroll(
        FrameBox* top, bool includesScrollPropertyOfTop = true);
    LayoutRect absoluteRectIncludingScroll(
        FrameBox* top, bool includesScrollPropertyOfTop = true)
    {
        return LayoutRect(
            absolutePointIncludingScroll(top, includesScrollPropertyOfTop),
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

    virtual void establishesStackingContextIfNeedsAndComputingPaintingFlags();
    virtual void computeVisibleRect(Frame::ComputeVisibleRectContext& ctx);
    virtual bool isVisible();

    bool isBoxesInvisibleFromHere();
    bool tryUniteVisibleRect(Frame::ComputeVisibleRectContext& ctx);

    void clearStackingContextIfNeeds();

    virtual void paintStackingContextContent(Canvas* canvas);
    virtual void willCompsiteStackingContext(Compositor* c)
    {
    }

    virtual void didCompsiteStackingContext(Compositor* c)
    {
    }

    SkMatrix computeScreenMatrix();
    LayoutRect computeScreenExtent();

    SkMatrix computeMatrixOnGraphicsBuffer();
    SkMatrix computeMatrixOnWindow();
    SkMatrix computeMatrixOnGraphicsBufferOnGraphicsBuffer();

    static unsigned constexpr maxChildFrameBoxIteratorSize = sizeof(size_t) * 5;
    class ChildFrameBoxIterator {
    public:
        virtual ~ChildFrameBoxIterator()
        {
        }

        virtual FrameBox* next() = 0;
        virtual bool hasNext() = 0;
    };

    class ChildFrameBoxIteratorBase : public ChildFrameBoxIterator {
    public:
        ChildFrameBoxIteratorBase(FrameBox* pos)
            : pos(pos)
        {
        }

        virtual FrameBox* next() override
        {
            auto ret = pos;
            auto next = pos->next();
            if (next) {
                pos = next->asFrameBox();
            } else {
                pos = nullptr;
            }
            return ret;
        }

        virtual bool hasNext() override
        {
            return pos != nullptr;
        }

        FrameBox* pos;
    };
    STARFISH_COMPILE_ASSERT(sizeof(ChildFrameBoxIteratorBase) <
                                maxChildFrameBoxIteratorSize,
                            "");

    class ChildFrameBoxIteratorNull : public ChildFrameBoxIterator {
    public:
        ChildFrameBoxIteratorNull()
        {
        }

        virtual FrameBox* next() override
        {
            return nullptr;
        }

        virtual bool hasNext() override
        {
            return false;
        }
    };
    STARFISH_COMPILE_ASSERT(sizeof(ChildFrameBoxIteratorBase) <
                                maxChildFrameBoxIteratorSize,
                            "");

    // This iterator not using GC alloctor
    // use return value only temporary
    virtual ChildFrameBoxIterator* childFrameBoxIterator(void* mem)
    {
        auto fs = firstChild();
        if (fs) {
            return new (mem) ChildFrameBoxIteratorBase(fs->asFrameBox());
        }
        return new (mem) ChildFrameBoxIteratorNull();
    }

    void iterateChildFrameBox(const std::function<void(FrameBox*)>& fn)
    {
        fn(this);
        auto iter = childFrameBoxIterator(alloca(maxChildFrameBoxIteratorSize));
        while (iter->hasNext()) {
            auto box = iter->next();
            box->iterateChildFrameBox(fn);
        }
    }

    void iterateChildFrameBoxOnCondition(
        const std::function<bool(FrameBox*)>& fn)
    {
        if (fn(this)) {
            auto iter =
                childFrameBoxIterator(alloca(maxChildFrameBoxIteratorSize));
            while (iter->hasNext()) {
                auto box = iter->next();
                box->iterateChildFrameBox(fn);
            }
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
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return;
    }

    virtual bool hasOwnGraphicsBufferMethod()
    {
        return false;
    }

    template <typename T>
    void applyBorderRadiusClippingIfNeeds(T canvas, const LayoutRect& rect,
                                          float spreadDistance = 0,
                                          bool inset = false);
    template <typename T>
    void applyBorderRadius(T canvas, const LayoutRect& rect,
                           float spreadDistance = 0, bool inset = false);

    // outline-width + outline-offset
    LayoutUnit outlineThickness();

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    void computeBorderRadiusProperties(
        const BorderRadiusData& br, const LayoutRect& rect,
        float& topLeftHorizontal, float& topRightHorizontal,
        float& topLeftVertical, float& bottomLeftVertical,
        float& topRightVertical, float& bottomRightVertical,
        float& bottomLeftHorizontal, float& bottomRightHorizontal);

protected:
    static inline void fillGCDescriptor(GC_word* obj_bitmap)
    {
        Frame::fillGCDescriptor(obj_bitmap);
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameBox, m_layoutParent));
    }

    static void paintBackgroundLayers(Canvas* canvas, FrameBox* box,
                                      NULLABLE HTMLElement* rootOrBodyelement,
                                      ComputedStyle* style);

    void applyBorderShapeClippingUsedInPaintingBoxShadow(
        const Unit::Rect& shadowRect, const Unit::Rect& borderRect,
        const Unit::Rect& imageRect, Canvas* canvas);

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
        } else if (m_hasRareData && m_box->hasRareData()) {
            m_box->frameBoxRareData()->m_margin = LayoutBoxSurroundData();
            m_box->frameBoxRareData()->m_border = LayoutBoxSurroundData();
            m_box->frameBoxRareData()->m_padding = LayoutBoxSurroundData();
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
} // namespace Starfish

#endif
