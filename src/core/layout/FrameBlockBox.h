/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifndef __StarFishFrameBlockBox__
#define __StarFishFrameBlockBox__

#include "core/layout/FrameBox.h"
#include "core/layout/FrameReplaced.h"
#include "core/layout/FrameInline.h"
#include "core/layout/FrameLineBreak.h"
#include "core/layout/FrameText.h"
#include "core/util/RefPtr.h"

namespace StarFish {

class FrameBlockBox;
class LineFormattingContext;

struct InlineTextBoxRareData : public gc {
    bool m_needsApplyTextOverflow;
    bool m_isHidedByTextOverflow;
    bool m_isTextOverflowDirectionIsLTR;
    StringView m_text;
    StringView m_nonOverflowText;
    String* m_overflowText;
    LayoutUnit m_nonOverflowTextWidth;

    InlineTextBoxRareData()
    {
        m_isHidedByTextOverflow = m_needsApplyTextOverflow = false;
        m_isTextOverflowDirectionIsLTR = true;
        m_overflowText = String::emptyString;
    }

    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(InlineTextBoxRareData));
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(InlineTextBoxRareData)] = { 0 };
            InlineTextBoxRareData::fillGCDescriptor(desc);
            descr =
                GC_make_descriptor(desc, GC_WORD_LEN(InlineTextBoxRareData));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(InlineTextBoxRareData, m_overflowText));
    }
};

class InlineTextBox final : public FrameBox {
public:
    InlineTextBox(InlineTextBox* box)
        : FrameBox(box->node(), box->style())
        , m_text(nullptr)
        , m_start(0)
        , m_end(std::numeric_limits<uint16_t>::max())
    {
        m_flags.m_isFirstLine = box->isFirstLine();
        m_flags.m_direction = box->charDirection();
    }

    InlineTextBox(FrameText* frame, const TextRun& run, bool isFirstLine)
        : FrameBox(frame->nodeSlowCase(), frame->style())
    {
        setText(run.m_stringView.string(), run.m_stringView.start(),
                run.m_stringView.end());
        m_flags.m_isFirstLine = isFirstLine;
        m_flags.m_direction = run.m_direction;
    }

    virtual bool isInlineBox() const override
    {
        return true;
    }

    virtual bool isInlineTextBox() const override
    {
        return true;
    }

    virtual void paintInlineContent(Canvas* canvas, PaintingInlineStage stage,
                                    LayoutUnit dx, LayoutUnit dy) override;
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y,
                           HitTestStage stage) override;

#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth) override
    {
        FrameBox::dump(depth);
        StringView tv = text();
        auto s = tv.substring()->toUTF8NonGCString();
        printf(" [(%s), dir: %d, start: %d, end %d] ", s.data(),
               (int)charDirection(), (int)tv.start(), (int)tv.end());
    }
#endif
    virtual const char* name() override
    {
        return "InlineTextBox";
    }

    void setText(String* t)
    {
        setText(t, 0, t->length());
    }

    void setText(const StringView& t)
    {
        setText(t.string(), t.start(), t.end());
    }

    CharDirection charDirection()
    {
        return m_flags.m_direction;
    }

    void setCharDirection(CharDirection dir)
    {
        m_flags.m_direction = dir;
    }

    FrameText* origin();

    TextRun textRun()
    {
        StringView tv = text();
        return TextRun(tv.string(), tv.start(), tv.end(), charDirection());
    }

    StringView text()
    {
        if (UNLIKELY(m_end < m_start)) {
            return m_rareData->m_text;
        }
        return StringView(m_text, m_start, m_end);
    }

    void unmarkFirstLine()
    {
        m_flags.m_isFirstLine = false;
        setWidth(style()->font()->measureText(textRun().m_stringView));
        setHeight(style()->font()->metrics().m_fontHeight);
    }

    bool isFirstLine()
    {
        return m_flags.m_isFirstLine;
    }

    bool isHidedByTextOverflow()
    {
        if (!hasInlineTextBoxRareData()) {
            return false;
        }
        return inlineTextBoxRareData()->m_isHidedByTextOverflow;
    }
    void markNeedsConsiderTextOverflow(String* overflowString, bool isLtr,
                                       LayoutUnit clippedWidth);
    ComputedStyle* style()
    {
        Frame* parent = layoutParent();
        while (parent->isLineBox()) {
            parent = parent->layoutParent();
        }

        return Frame::style(parent, parent->style(), m_flags.m_isFirstLine);
    }

    virtual void iterateChildFrameBox(
        const std::function<void(FrameBox*)>& fn) override
    {
        fn(this);
    }

    void* operator new(size_t size);
    void* operator new(size_t /* size */, void* p)
    {
        return p;
    }
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        FrameBox::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(InlineTextBox, m_text));
    }

    void setText(String* str, size_t start, size_t end)
    {
        STARFISH_ASSERT(str);
        if (hasInlineTextBoxRareData()) {
            inlineTextBoxRareData()->m_text = StringView(str, start, end);
            return;
        }
        if (start < std::numeric_limits<uint16_t>::max() &&
            end < std::numeric_limits<uint16_t>::max()) {
            m_text = str;
            m_start = start;
            m_end = end;
        } else {
            ensureInlineTextBoxRareData();
            inlineTextBoxRareData()->m_text = StringView(str, start, end);
        }
    }

    bool hasInlineTextBoxRareData()
    {
        return m_end < m_start;
    }

    InlineTextBoxRareData* inlineTextBoxRareData()
    {
        STARFISH_ASSERT(hasInlineTextBoxRareData());
        return m_rareData;
    }

    void ensureInlineTextBoxRareData()
    {
        if (hasInlineTextBoxRareData()) {
            return;
        }

        InlineTextBoxRareData* rareData = new InlineTextBoxRareData();
        rareData->m_text = StringView(m_text, m_start, m_end);
        m_start = 1;
        m_end = 0;
        m_rareData = rareData;
    }
    union {
        String* m_text;
        InlineTextBoxRareData* m_rareData;
    };
    uint16_t m_start;
    uint16_t m_end;
};

class InlineBoxLayoutParentBox : public FrameBox {
public:
    InlineBoxLayoutParentBox()
        : InlineBoxLayoutParentBox(nullptr, nullptr)
    {
    }

    InlineBoxLayoutParentBox(Frame* frame)
        : InlineBoxLayoutParentBox(frame->node(), frame->style())
    {
    }

    virtual bool isInlineBoxLayoutParentBox() const override
    {
        return true;
    }

    GCVector<FrameBox*>& boxes()
    {
        return m_boxes;
    }

    virtual void establishesStackingContextIfNeeds() override
    {
        for (size_t i = 0; i < m_boxes.size(); i++) {
            m_boxes[i]->establishesStackingContextIfNeeds();
        }
    }

    virtual InlineNonReplacedBox* firstInlineNonReplacedBox(
        FrameInline* f) override
    {
        InlineNonReplacedBox* ret = nullptr;
        for (size_t i = 0; i < m_boxes.size(); i++) {
            if ((ret = m_boxes[i]->firstInlineNonReplacedBox(f))) {
                return ret;
            }
        }

        return ret;
    }

    virtual void computeVisibleRect(
        Frame::ComputeVisibleRectContext& ctx) override;

    FrameBox* firstInlineBox();
    FrameBox* lastInlineBox();
    void removeDanglingSpace(LineFormattingContext* ctx);
    bool containOnlyEmptyInlineNonReplacedBoxes(LineFormattingContext* ctx);
    bool isAbsolutePositionedBoxLayoutParent(LineFormattingContext* ctx);

    void insertInlineBox(FrameBox* box)
    {
        m_boxes.push_back(box);
        box->setLayoutParent(this);
    }
    void layoutInlineBoxes(LineFormattingContext* ctx, LayoutUnit start);
    void coordinateVerticalProperties(LineFormattingContext* ctx,
                                      LayoutUnit yOffset);
    void registerRelativePositionedBoxesAndMarkPaintFlag(LayoutContext& ctx);

    void moveToNewLineBox(LineFormattingContext* ctx, FrameBox* box,
                          LineBox* lineBox);

    void setLeftMBPs(LineFormattingContext* ctx);
    void setRightMBPs(LineFormattingContext* ctx);

    void resetChildrenVerticalPositions(LayoutContext& ctx);
    void quickInlineLayout(LineFormattingContext* ctx);

    virtual void iterateChildFrameBox(
        const std::function<void(FrameBox*)>& fn) override
    {
        fn(this);
        for (size_t i = 0; i < m_boxes.size(); i++) {
            m_boxes[i]->iterateChildFrameBox(fn);
        }
    }

    virtual Frame* hitTestChildrenWith(LayoutUnit x, LayoutUnit y,
                                       HitTestStage stage) override
    {
        Frame* result = nullptr;
        for (size_t i = 0; i < m_boxes.size(); i++) {
            Frame* child = m_boxes[m_boxes.size() - 1 - i];
            LayoutUnit cx = x - child->asFrameBox()->x();
            LayoutUnit cy = y - child->asFrameBox()->y();
            result = child->hitTest(cx, cy, stage);
            if (result) {
                return result;
            }
        }
        return result;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void paintInlineContent(Canvas* canvas, PaintingInlineStage stage,
                                    LayoutUnit dx, LayoutUnit dy) override;

    void seenInlineBox(PaintingInlineStage stage)
    {
        if (stage == PaintingInlineStage::PaintingInlineBox) {
            m_flags.m_seenNormalFlowInlineBox = true;
        } else if (stage == PaintingInlineStage::
                                PaintingAtomicInlineBoxButInlineReplaced) {
            m_flags.m_seenNormalFlowInlineBlockBox = true;
        } else {
            STARFISH_ASSERT(stage ==
                            PaintingInlineStage::PaintingInlineReplaced);
            m_flags.m_seenNormalFlowInlineReplaced = true;
        }

        auto lp = layoutParent();
        if (lp->isInlineBoxLayoutParentBox()) {
            lp->asInlineBoxLayoutParentBox()->seenInlineBox(stage);
        }
    }

    void mergeInlineTextBoxes(LineFormattingContext* ctx);

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        FrameBox::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(InlineBoxLayoutParentBox, m_boxes));
    }

    GCVector<FrameBox*> m_boxes;

    InlineBoxLayoutParentBox(Node* node, ComputedStyle* style)
        : FrameBox(node, style)
    {
    }
};

struct InlineNonReplacedBoxRareData : public gc {
    InlineNonReplacedBoxRareData(FrameInline* origin)
        : m_tag(0x3)
        , m_origin(origin)
    {
    }
    void* operator new(size_t size)
    {
        // We can use atomic malloc
        // because FrameTree already has reference of m_origin
        return GC_MALLOC_ATOMIC(size);
    }
    void* operator new[](size_t size) = delete;

    size_t m_tag;
    FrameInline* m_origin;
    LayoutBoxSurroundData m_orgPadding, m_orgBorder, m_orgMargin;
};

enum InlineNonReplacedBoxMBPStatus {
    MBPStatusNone = 0,
    ProcessedStaringMBP = 1,
    ProcessedEndingMBP = 2,
    SetLeftMBP = 4,
    SetRightMBP = 8,
};

class InlineNonReplacedBoxMBPStatusHolder
    : public RefCounted<InlineNonReplacedBoxMBPStatusHolder> {
public:
    InlineNonReplacedBoxMBPStatusHolder()
    {
        m_status = 0;
    }
    int m_status;
};

class InlineNonReplacedBox final : public InlineBoxLayoutParentBox {
    friend class FrameBlockBox;
    friend class LineFormattingContext;

public:
    InlineNonReplacedBox(LineFormattingContext* ctx,
                         InlineNonReplacedBox* inlineBox, bool isFirstLine);
    InlineNonReplacedBox(LineFormattingContext* ctx, FrameInline* frame,
                         bool isFirstLine);

    virtual bool isInlineBox() const override
    {
        return true;
    }

    virtual bool isInlineNonReplacedBox() const override
    {
        return true;
    }

    virtual const char* name() override
    {
        return "InlineNonReplacedBox";
    }

    virtual void layoutInline(
        LineFormattingContext& lineFormattingContext) override;
    virtual void paintStackingContextContent(Canvas* canvas) override;
    virtual void paintInlineContent(Canvas* canvas, PaintingInlineStage stage,
                                    LayoutUnit dx, LayoutUnit dy) override;
    virtual void paintChildrenWith(PaintingContext& ctx) override;
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y,
                           HitTestStage stage) override;

    ComputedStyle* style()
    {
        return Frame::style(this, Frame::style(), m_flags.m_isFirstLine);
    }
#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth) override;
#endif
    virtual void establishesStackingContextIfNeeds() override
    {
        FrameBox::establishesStackingContextIfNeeds();

        InlineBoxLayoutParentBox::establishesStackingContextIfNeeds();
    }

    virtual InlineNonReplacedBox* firstInlineNonReplacedBox(
        FrameInline* f) override
    {
        if (origin() == f) {
            return this;
        }

        return InlineBoxLayoutParentBox::firstInlineNonReplacedBox(f);
    }

    virtual void computeVisibleRect(
        Frame::ComputeVisibleRectContext& ctx) override
    {
        if (!tryUniteVisibleRect(ctx)) {
            return;
        }

        InlineBoxLayoutParentBox::computeVisibleRect(ctx);
    }

    virtual void paintBackgroundAndBorders(Canvas* canvas) override;

    FrameInline* origin()
    {
        if (rareData()) {
            return rareData()->m_origin;
        }
        return m_origin;
    }

    bool isCollapsed() const
    {
        return m_flags.m_isCollapsedOrDidSpiltFrameInline;
    }

    void markCollapsed()
    {
        m_flags.m_isCollapsedOrDidSpiltFrameInline = true;
    }

    void processStartingMBP(LineFormattingContext* lineFormattingContext);
    void processEndingMBP(LineFormattingContext* lineFormattingContext);
    void setOrgLeftMBP(LineFormattingContext* lineFormattingContext);
    void setOrgRightMBP(LineFormattingContext* lineFormattingContext);

    void unsetLeftMBP()
    {
        setMarginLeft(0);
        setBorderLeft(0);
        setPaddingLeft(0);
        setLeftMBPCleared(true);
    }

    void unsetRightMBP()
    {
        setMarginRight(0);
        setBorderRight(0);
        setPaddingRight(0);
        setRightMBPCleared(true);
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;
    void* operator new(size_t /* size */, void* p)
    {
        return p;
    }

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        InlineBoxLayoutParentBox::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(InlineNonReplacedBox, m_origin));
    }

    union {
        FrameInline* m_origin;
        InlineNonReplacedBoxRareData* m_rareData;
    };

    InlineNonReplacedBoxRareData* rareData()
    {
        if (m_origin == nullptr || 0x3 != *((size_t*)m_origin)) {
            return nullptr;
        }
        return m_rareData;
    }

    InlineNonReplacedBox(Frame* frame, FrameInline* origin, bool isFirstLine)
        : InlineBoxLayoutParentBox(frame)
        , m_origin(origin)
    {
        m_flags.m_isCollapsedOrDidSpiltFrameInline = false;
        m_flags.m_isFirstLine = isFirstLine;
        if (origin->isLeftMBPCleared()) {
            setLeftMBPCleared(true);
        }

        if (origin->isRightMBPCleared()) {
            setRightMBPCleared(true);
        }

        // recompute style flags
        // we should re compute flags here
        // because, when ctor of Frame is executed, vtable is not set correctly
        // so we could not consider that what kind of frame is this
        computeStyleFlags();
    }

    void setTopBottomOrgMBP()
    {
        if (rareData() || marginTop() || marginBottom() || borderTop() ||
            borderBottom() || paddingTop() || paddingBottom()) {
            if (!rareData()) {
                m_rareData = new InlineNonReplacedBoxRareData(m_origin);
            }
            m_rareData->m_orgMargin.setTop(marginTop());
            m_rareData->m_orgMargin.setBottom(marginBottom());
            m_rareData->m_orgBorder.setTop(borderTop());
            m_rareData->m_orgBorder.setBottom(borderBottom());
            m_rareData->m_orgPadding.setTop(paddingTop());
            m_rareData->m_orgPadding.setBottom(paddingBottom());
        }
    }
};

class LineBox final : public InlineBoxLayoutParentBox {
    friend class LineFormattingContext;
    friend class FrameBlockBox;
    friend class InlineNonReplacedBox;

public:
    LineBox(Frame* parent)
        : InlineBoxLayoutParentBox()
    {
        setLayoutParent(parent);
    }

    virtual bool isLineBox() override
    {
        return true;
    }

    virtual const char* name() override
    {
        return "LineBox";
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        InlineBoxLayoutParentBox::fillGCDescriptor(desc);
    }
};

struct FrameBlockBoxRareData : public FrameBoxRareData {
    FrameBlockBoxRareData(Frame* layoutParent)
        : FrameBoxRareData(layoutParent)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    LayoutUnit m_scrollWidth;
    LayoutUnit m_scrollHeight;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        FrameBoxRareData::fillGCDescriptor(desc);
    }
};

class FrameBlockBox : public FrameBox {
    friend class LineFormattingContext;
    friend class InlineNonReplacedBox;
    friend class FrameDocument;

public:
    FrameBlockBox(Node* node, ComputedStyle* style)
        : FrameBox(node, style)
    {
        STARFISH_ASSERT((node == nullptr && style != nullptr) ||
                        (node != nullptr && style == nullptr));
    }

    // Do not access this property before finish layout!
    LayoutUnit scrollWidth()
    {
        updateScrollWidthAndHeightIfNeeds();

        if (m_flags.m_hasBiggerContentThanFrameWidth) {
            return frameBlockBoxRareData()->m_scrollWidth;
        } else {
            return width() - borderWidth();
        }
    }

    // Do not access this property before finish layout!
    LayoutUnit scrollHeight()
    {
        updateScrollWidthAndHeightIfNeeds();

        if (m_flags.m_hasBiggerContentThanFrameHeight) {
            return frameBlockBoxRareData()->m_scrollHeight;
        } else {
            return height() - borderHeight();
        }
    }

    virtual LayoutUnit scrollLeft();
    virtual LayoutUnit scrollTop();

    virtual bool isFrameBlockBox() override
    {
        return true;
    }

    virtual const char* name() override
    {
        return "FrameBlockBox";
    }

    FrameDocument* asFrameDocument()
    {
        return (FrameDocument*)this;
    }

    bool isNecessaryBlockBox()
    {
        if (isAnonymous()) {
            if (firstChild() && firstChild()->isFrameInline()) {
                FrameInline* fi = firstChild()->asFrameInline();
                if (fi->isLeftMBPCleared() && fi->isRightMBPCleared()) {
                    if (!fi->firstChild()) {
                        return false;
                    }
                    if (fi->firstChild() == fi->lastChild() &&
                        fi->firstChild()->isFrameText()) {
                        if (fi->firstChild()
                                ->asFrameText()
                                ->text()
                                ->containsOnlyWhitespace()) {
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    }

    void markHeightComputed(bool b)
    {
        m_flags.m_heightComputed = b;
    }

    bool heightComputed() const
    {
        return m_flags.m_heightComputed;
    }

    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat) override;
    virtual void quickLayout(LayoutContext& ctx) override;
    virtual void computePreferredWidth(PreferredWidthContext& ctx) override;
    virtual void layoutInline(LineFormattingContext& ctx) override;
    void computeContentWidth(LayoutContext& ctx, FrameBox* cb,
                             LayoutUnit containgBlockContentWidth);
    void computeContentHeight(LayoutContext& ctx, LayoutUnit contentHeight);

#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth) override;
#endif
    virtual void paintContent(PaintingContext& ctx) override;
    virtual void paintChildrenWith(PaintingContext& ctx) override;
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y,
                           HitTestStage stage) override;
    virtual Frame* hitTestChildrenWith(LayoutUnit x, LayoutUnit y,
                                       HitTestStage stage) override;

    virtual InlineNonReplacedBox* firstInlineNonReplacedBox(
        FrameInline* f) override;

    virtual void establishesStackingContextIfNeeds() override;
    virtual void computeVisibleRect(
        FrameBox::ComputeVisibleRectContext& ctx) override;

    virtual bool hasBlockFlow()
    {
        if (!firstChild()) {
            return false;
        }
        Frame* child = firstChild();
        return (child->isBlockLevel() && child->isNormalFlow());
    }

    virtual bool isSelfCollapsingBlock(LayoutContext& ctx) override;
    GCVector<LineBox*>& lineBoxes()
    {
        return m_lineBoxes;
    }

    virtual void iterateChildFrameBox(
        const std::function<void(FrameBox*)>& fn) override
    {
        fn(this);
        if (hasBlockFlow()) {
            Frame* box = firstChild();
            while (box) {
                box->asFrameBox()->iterateChildFrameBox(fn);
                box = box->next();
            }
        } else {
            for (size_t i = 0; i < m_lineBoxes.size(); i++) {
                m_lineBoxes[i]->iterateChildFrameBox(fn);
            }
        }
    }

    virtual void inlineLayoutAdditionalPath(LayoutContext& ctx)
    {
    }

    ComputedStyle* notAnonymousBlockStyle()
    {
        ComputedStyle* style = this->style();
        if (isAnonymous()) {
            style = parent()->style();
        }
        return style;
    }

    bool hasBiggerContentThanFrameWidth()
    {
        updateScrollWidthAndHeightIfNeeds();
        return m_flags.m_hasBiggerContentThanFrameWidth;
    }

    bool hasBiggerContentThanFrameHeight()
    {
        updateScrollWidthAndHeightIfNeeds();
        return m_flags.m_hasBiggerContentThanFrameHeight;
    }

    LayoutRect computeVisibleRectForScroll(
        bool isForSpecialValueForTableCell = false);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* obj_bitmap)
    {
        FrameBox::fillGCDescriptor(obj_bitmap);
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameBlockBox, m_treeItemModel.m_parent));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameBlockBox, m_treeItemModel.m_previous));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameBlockBox, m_treeItemModel.m_next));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameBlockBox, m_treeItemModel.m_firstChild));
        GC_set_bit(obj_bitmap,
                   GC_WORD_OFFSET(FrameBlockBox, m_treeItemModel.m_lastChild));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(FrameBlockBox, m_lineBoxes));
    }

    virtual void paintInlineContentBlock(Canvas* canvas);
    void updateScrollWidthAndHeightIfNeeds(OverflowValue overflowX,
                                           OverflowValue overflowY);
    void updateScrollWidthAndHeightIfNeeds();
    LayoutUnit layoutBlock(LayoutContext& ctx);
    LayoutUnit layoutInline(LayoutContext& ctx);
    void registerRelativePositionedBoxesAndMarkPaintFlag(LayoutContext& ctx);
    void computeContentHeight(LayoutContext& ctx, FrameBox* cb);
    void registerRelativePositionIfNeeds(LayoutContext& ctx);

    virtual bool hasFrameTreeItemModel() override
    {
        return true;
    }

    virtual FrameTreeItemModel* frameTreeItemModel() override
    {
        return &m_treeItemModel;
    }

    virtual FrameBlockBoxRareData* createRareData() override
    {
        return new FrameBlockBoxRareData(m_layoutParent);
    }

    FrameBlockBoxRareData* frameBlockBoxRareData() const
    {
        STARFISH_ASSERT(hasRareData());
        return (FrameBlockBoxRareData*)m_layoutParent;
    }

    FrameTreeItemModel m_treeItemModel;
    GCVector<LineBox*> m_lineBoxes;
};

struct FloatingBoxLayoutContext {
    int m_hasFloat;
    LayoutUnit m_y;
    LayoutUnit m_accumulatedLeftFloatingBoxWidth;
    LayoutUnit m_accumulatedRightFloatingBoxWidth;
    LayoutUnit m_originalLineBoxX;
    LayoutUnit m_originalLineBoxWidth;

    FloatingBoxLayoutContext(int hasFloat, LayoutUnit y,
                             LayoutUnit originalLineBoxX,
                             LayoutUnit originalLineBoxWidth)
        : m_hasFloat(hasFloat)
        , m_y(y)
        , m_originalLineBoxX(originalLineBoxX)
        , m_originalLineBoxWidth(originalLineBoxWidth)
    {
    }
};

class Word {
public:
    void concat(FrameBox* box)
    {
        m_boxes.push_back(box);
    }

    void clear()
    {
        m_boxes.clear();
    }

    bool isEmpty() const
    {
        return m_boxes.size() == 0;
    }

    LayoutUnit width() const
    {
        // TODO: consider starting mbp of InlineNonReplacedBox
        return std::accumulate(std::next(m_boxes.begin()), m_boxes.end(),
                               (*m_boxes.begin())->outerWidth(),
                               [](LayoutUnit w, FrameBox* box) {
                                   if (box->isNormalFlow()) {
                                       return w + box->outerWidth();
                                   } else {
                                       return w;
                                   }
                               });
    }

    void unmarkFirstLine()
    {
        unmarkFirstLine(m_boxes);
    }

    GCVector<FrameBox*>& boxes()
    {
        return m_boxes;
    }

private:
    GCVector<FrameBox*> m_boxes;

    void unmarkFirstLine(GCVector<FrameBox*>& boxes)
    {
        std::for_each(boxes.begin(), boxes.end(), [this](FrameBox* box) {
            if (box->isNormalFlow()) {
                if (box->isInlineNonReplacedBox()) {
                    unmarkFirstLine(box->asInlineNonReplacedBox()->boxes());
                } else {
                    box->asInlineTextBox()->unmarkFirstLine();
                }
            }
        });
    }
};

struct LanguageDirection {
    LanguageDirection()
        : m_direction(None)
        , m_parentDirectionValue(DirectionValue::LtrDirectionValue)
    {
    }

    void markDirection(DirectionValue dir)
    {
        m_parentDirectionValue = dir;
    }

    void markDirection(CharDirection dir)
    {
        switch (dir) {
        case CharDirection::Ltr:
            markLtrDirection();
            break;
        case CharDirection::Rtl:
            markRtlDirection();
            break;
        case CharDirection::Mixed:
            markLtrDirection();
            markRtlDirection();
            break;
        default:
            break;
        }
    }

    void markLtrDirection()
    {
        m_direction |= LtrDirection;
    }

    void markRtlDirection()
    {
        m_direction |= RtlDirection;
    }

    bool isLtrOnly()
    {
        return m_direction == LtrDirection;
    }

    bool isRtlOnly()
    {
        return m_direction == RtlDirection;
    }

    bool isMixed()
    {
        return (m_parentDirectionValue == DirectionValue::RtlDirectionValue) ||
               ((m_direction & RtlDirection) != 0);
    }

private:
    enum Direction { None, LtrDirection, RtlDirection };

    size_t m_direction;
    DirectionValue m_parentDirectionValue;
};

class LineFormattingContext {
private:
    void resetLineBox();
    LayoutUnit inlineBlockAscender(FrameBlockBox* box)
    {
        STARFISH_ASSERT(m_inlineBlockAscender.find(box) !=
                        m_inlineBlockAscender.end());
        return m_inlineBlockAscender[box];
    }

    LayoutUnit offsetApplyingTextAlign();
    void computeHorizontalProperties();
    LayoutUnit distanceToNextLineBox(FrameLineBreak* br,
                                     bool hasMoreInlineBoxes);

    void insertPendingFloatingBoxes();
    void insertPendingInlineBoxes();
    void layoutLineBox(LayoutUnit yDiff, LayoutUnit height);

    void generateInlineTextBox(TextToken& token);
    void insertFloatingBoxAndReLayoutLineBoxIfNeeds(FrameBox* box);
    InlineTextBox* splitInlineTextBox(InlineTextBox* textBox);
    void insertInlineBox(FrameBox* box);
    void insertAbsolutePositionedBoxes();

    void registerAbsolutePositionedBox(FrameBox* box);

    template <typename Box>
    LayoutUnit layoutInlineBoxes(Box* parent, LayoutUnit start);
    template <typename Iter>
    void sortInlineBoxes(Iter& iter);

    void removeAllInlineBoxes();

    bool isAnyOfInlineBoxesCollidedWithFloatingBoxes();
    void reCacheFloatingBoxes(LayoutUnit xDiff);
    void makeFloatingBoxLayoutContext(LayoutUnit yDiff);

    void breakLineForLineBox(FrameLineBreak* br, bool isLastLine,
                             bool skipFinishLine);
    void breakLineForInlineNonReplacedBox(FrameLineBreak* br);

    CharDirection contentDir(FrameBox* box);
    void resolveBidi(DirectionValue parentDir, GCVector<FrameBox*>& boxes);
    void splitInlineBoxes(GCVector<FrameBox*>& boxes);

    bool canInsertToLineBox(FrameBox* f, LayoutUnit width);
    bool hasFloatingBoxAlreadyInLineBox(Frame* f);
    bool canInsertFloatingBox(FrameBox* f, bool allowPendingFloatingBox);

    void collectComputeDirectionsCandidate(Frame* parent,
                                           std::vector<Frame*>& frames);

public:
    LineFormattingContext(FrameBlockBox* block, LayoutContext& ctx,
                          bool forQuick);

    void updateCurrentLayoutParent(Frame* parent);
    void computeVerticalProperties(FrameBox* parentBox, bool dueToBr);

    void layoutInline(Frame* origin);

    void finishLineForLineBox(FrameLineBreak* br, bool isLastLine);
    void finishLineForInlineNonReplacedBox(FrameLineBreak* br, bool isLastNode);
    void breakLine(FrameLineBreak* br);
    void makeFloatingBoxLayoutContextDueToClearIfNeeds(FrameBox* box);

    void tryInsertInlineBox(FrameBox* box);
    void tryInsertFloatingBox(FrameBox* box);
    void insertWord(Frame* next);
    void markInlineBoxIndex(FrameBox* box);

    bool removeLastLineBoxIfNeeds();
    LayoutUnit contentHeightForBlock();
    void* allocateInlineTextBox();
    void* allocateInlineNonReplacedBox();

    LineBox* currentLine()
    {
        return m_block->m_lineBoxes.back();
    }

    bool isFirstLineBox() const
    {
        return m_block->m_lineBoxes.size() == 1 && !m_isPendingBreakLine;
    }

    bool isWordProcessing() const
    {
        return !m_word.isEmpty();
    }

    bool breakableWord(FrameBox* box) const;

    void handleAbsoluteBox(FrameBox* box, bool canInsert, bool canRegister);
    void handleTextToken(TextToken& token);
    void handleSoftHyphenate(bool hyphenateOnLine);

    bool isWhiteSpaceAtLast()
    {
        return m_isWhiteSpaceAtLast;
    }

    void setIsWhiteSpaceAtLast(bool isWhiteSpaceAtLast)
    {
        m_isWhiteSpaceAtLast = isWhiteSpaceAtLast;
    }

    LayoutUnit currentLineWidth()
    {
        return m_currentLineWidth;
    }

    void registerInlineContent(FrameLineBreak* br);

    void registerInlineBlockAscender(LayoutUnit ascender, FrameBlockBox* box)
    {
        m_inlineBlockAscender[box] = ascender;
    }

    size_t absolutePositionedBoxLayoutParentCnt(
        InlineBoxLayoutParentBox* box) const
    {
        auto iter = m_absolutePositionedLayoutParentCnt.find(box);
        if (iter != m_absolutePositionedLayoutParentCnt.end()) {
            return iter->second;
        }
        return 0;
    }

    void markAbsolutePositionedBoxLayoutParent(InlineBoxLayoutParentBox* box)
    {
        auto iter = m_absolutePositionedLayoutParentCnt.find(box);
        if (iter == m_absolutePositionedLayoutParentCnt.end()) {
            m_absolutePositionedLayoutParentCnt[box] = 1;
        } else {
            iter->second++;
        }
    }

    void unMarkAbsolutePositionedBoxLayoutParent(InlineBoxLayoutParentBox* box)
    {
        STARFISH_ASSERT(absolutePositionedBoxLayoutParentCnt(box) != 0);
        m_absolutePositionedLayoutParentCnt[box]--;
    }

    bool dontBreakLine(FrameBox* box, LayoutUnit width);

    void computeDirection(Frame* parent, DirectionValue direction);

    LayoutUnit ascender(InlineBoxLayoutParentBox* box)
    {
        return m_ascenderDescenderOfInlineBoxLayoutParentBox[box].first;
    }

    LayoutUnit descender(InlineBoxLayoutParentBox* box)
    {
        return m_ascenderDescenderOfInlineBoxLayoutParentBox[box].second;
    }

    void setAscDescender(InlineBoxLayoutParentBox* box, LayoutUnit ascender,
                         LayoutUnit descender)
    {
        m_ascenderDescenderOfInlineBoxLayoutParentBox[box] =
            std::make_pair(ascender, descender);
        box->setHeight(ascender - descender);
    }

    bool isProcessedStartingMBP(InlineNonReplacedBox* box)
    {
        RefPtr<InlineNonReplacedBoxMBPStatusHolder>& v =
            m_inlineNonReplacedBoxMBPStatus[box];
        return ((v->m_status) & ProcessedStaringMBP) != 0;
    }

    void markProcessedStartingMBP(InlineNonReplacedBox* box)
    {
        STARFISH_ASSERT(!isProcessedStartingMBP(box));
        RefPtr<InlineNonReplacedBoxMBPStatusHolder>& v =
            m_inlineNonReplacedBoxMBPStatus[box];
        (v->m_status) |= ProcessedStaringMBP;
    }

    bool isProcessedEndingMBP(InlineNonReplacedBox* box)
    {
        RefPtr<InlineNonReplacedBoxMBPStatusHolder>& v =
            m_inlineNonReplacedBoxMBPStatus[box];
        return ((v->m_status) & ProcessedEndingMBP) != 0;
    }

    void markProcessedEndingMBP(InlineNonReplacedBox* box)
    {
        STARFISH_ASSERT(!isProcessedEndingMBP(box));
        RefPtr<InlineNonReplacedBoxMBPStatusHolder>& v =
            m_inlineNonReplacedBoxMBPStatus[box];
        (v->m_status) |= ProcessedEndingMBP;
    }

    bool isSetLeftMBP(InlineNonReplacedBox* box)
    {
        RefPtr<InlineNonReplacedBoxMBPStatusHolder>& v =
            m_inlineNonReplacedBoxMBPStatus[box];
        return ((v->m_status) & SetLeftMBP) != 0;
    }

    void markSetLeftMBP(InlineNonReplacedBox* box)
    {
        STARFISH_ASSERT(!isSetLeftMBP(box));
        RefPtr<InlineNonReplacedBoxMBPStatusHolder>& v =
            m_inlineNonReplacedBoxMBPStatus[box];
        (v->m_status) |= SetLeftMBP;
    }

    bool isSetRightMBP(InlineNonReplacedBox* box)
    {
        RefPtr<InlineNonReplacedBoxMBPStatusHolder>& v =
            m_inlineNonReplacedBoxMBPStatus[box];
        return ((v->m_status) & SetRightMBP) != 0;
    }

    void markSetRightMBP(InlineNonReplacedBox* box)
    {
        STARFISH_ASSERT(!isSetRightMBP(box));
        RefPtr<InlineNonReplacedBoxMBPStatusHolder>& v =
            m_inlineNonReplacedBoxMBPStatus[box];
        (v->m_status) |= SetRightMBP;
    }

    void setInlineBoxIndex(FrameBox* f, size_t inlineBoxIdx)
    {
        m_inlineBoxIndexes[f] = inlineBoxIdx;
    }

    size_t inlineBoxIndex(FrameBox* f)
    {
        auto iter = m_inlineBoxIndexes.find(f);
        if (iter == m_inlineBoxIndexes.end())
            return SIZE_MAX;
        return iter->second;
    }

    bool isLastLineBox() const
    {
        return m_isLastLineBox;
    }

    void markIsLastLineBox()
    {
        m_isLastLineBox = true;
    }

    LayoutUnit wordSpacing(FrameBox* box, InlineBoxLayoutParentBox* parentBox);

    LayoutUnit m_leftBoundary;
    LayoutUnit m_rightBoundary;
    LayoutLocation m_absPosition;
    LayoutUnit m_lineBoxX;
    LayoutUnit m_lineBoxY;
    LayoutUnit m_currentLineWidth;
    LayoutUnit m_textIndentWidth;
    LayoutUnit m_lineBoxWidth;
    LayoutUnit m_unprocessedStartingMBPWidth;
    FrameBlockBox* m_block;
    LayoutContext& m_layoutContext;
    InlineBoxLayoutParentBox* m_currentLayoutParent;
    bool m_isPendingBreakLine;
    bool m_isWhiteSpaceAtLast;
    bool m_isSoftHyphenAtLast;
    bool m_canConcatWord;
    bool m_isLastLineBox;
    bool m_isFirstLineCandidate;
    bool m_shouldConsiderTextOverflow;
    size_t m_inlineBoxIndex;
    size_t m_pendingFloatingBoxNumsBeforeCurrentLine;
    size_t m_floatingBoxesSizeBeforeCurrentLine;
    int m_lastLineHasFloatValue;

    // we don't need gc_allocater here
    // frame tree has strong reference already
    std::unordered_map<FrameBlockBox*, LayoutUnit> m_inlineBlockAscender;

    std::vector<FloatingBoxLayoutContext> m_floatingBoxLayoutContexts;
    std::vector<FrameBox*> m_absolutePositionedBoxes;
    std::vector<FrameBox*> m_pendingFloatingBoxes;
    GCVector<FrameBox*> m_pendingInlineBoxes;
    Word m_word;

    std::unordered_map<Frame*, DirectionValue> m_computedDirectionValuePerFrame;
    std::unordered_map<FrameText*, std::vector<TextRun>> m_textRunsPerFrameText;
    std::unordered_map<FrameBox*, size_t> m_inlineBoxIndexes;

    std::unordered_map<InlineBoxLayoutParentBox*, size_t>
        m_absolutePositionedLayoutParentCnt;

    std::unordered_map<InlineNonReplacedBox*,
                       RefPtr<InlineNonReplacedBoxMBPStatusHolder>>
        m_inlineNonReplacedBoxMBPStatus;

    std::unordered_map<InlineBoxLayoutParentBox*,
                       std::pair<LayoutUnit, LayoutUnit>>
        m_ascenderDescenderOfInlineBoxLayoutParentBox;

    LanguageDirection m_currentLanguageDirection;
    std::unordered_map<Frame*, LanguageDirection> m_languageDirections;
};
}

#endif
