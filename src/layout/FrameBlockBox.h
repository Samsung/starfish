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

#ifndef __StarFishBlockBox__
#define __StarFishBlockBox__

#include "layout/FrameBox.h"
#include "layout/FrameReplaced.h"
#include "layout/FrameInline.h"
#include "layout/FrameLineBreak.h"
#include "layout/FrameText.h"

namespace StarFish {

class FrameBlockBox;
class LineFormattingContext;
class InlineTextBox; // TextNode
class InlineNonReplacedBox; // non-replaced element, display: inline

class InlineBox : public FrameBox {
public:
    InlineBox(Node* node, ComputedStyle* style, Frame* parent)
        : FrameBox(node, style)
    {
        setParent(parent);
    }

    Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage)
    {
        if (stage == HitTestStage::HitTestNormalFlowInline) {
            return FrameBox::hitTest(x, y, stage);
        }
        return nullptr;
    }

    virtual bool isInlineBox() { return true; }
    virtual bool isInlineTextBox() const { return false; }
    virtual bool isInlineNonReplacedBox() const { return false; }

    InlineTextBox* asInlineTextBox()
    {
        STARFISH_ASSERT(isInlineTextBox());
        return (InlineTextBox*)this;
    }

    InlineNonReplacedBox* asInlineNonReplacedBox()
    {
        STARFISH_ASSERT(isInlineNonReplacedBox());
        return (InlineNonReplacedBox*)this;
    }
};

struct TextRun {
    FrameText* m_frameText;
    StringView m_stringView;
    CharDirection m_direction;

    TextRun(FrameText* frameText, String* str, size_t startPosition, size_t endPosition, CharDirection dir)
        : m_stringView(StringView(str, startPosition, endPosition))
    {
        m_frameText = frameText;
        m_direction = dir;
    }
};

class InlineTextBox : public InlineBox {
public:
    InlineTextBox(Node* node, ComputedStyle* style, Frame* parent, const TextRun& run)
        : InlineBox(node, style, parent)
        , m_textRun(run)
    {
    }

    virtual bool isInlineTextBox() const { return true; }

    virtual void paint(PaintingContext& ctx);
#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth)
    {
        InlineBox::dump(depth);
        printf(" [(%s), dir: %d, start: %d, end %d] ", m_textRun.m_stringView.substring()->utf8Data(), (int)charDirection(),
            (int)m_textRun.m_stringView.start(), (int)m_textRun.m_stringView.end());
    }
#endif
    virtual const char* name()
    {
        return "InlineTextBox";
    }

    void setText(String* t)
    {
        m_textRun.m_stringView = StringView(t, 0, t->length());
    }

    CharDirection charDirection()
    {
        return m_textRun.m_direction;
    }

    void setCharDirection(CharDirection dir)
    {
        m_textRun.m_direction = dir;
    }

    FrameText* origin()
    {
        return m_textRun.m_frameText;
    }

    const TextRun& textRun()
    {
        return m_textRun;
    }

protected:
    TextRun m_textRun;
};

class InlineNonReplacedBox : public InlineBox {
    friend class FrameBlockBox;
    friend void splitInlineBoxesAndMarkDirectionForResolveBidi(LineFormattingContext& ctx, DirectionValue parentDir, std::vector<FrameBox*, gc_allocator_ignore_off_page<FrameBox*>>& boxes);
    friend void reassignLeftRightMBPOfInlineNonReplacedBox(LineFormattingContext& ctx, std::vector<FrameBox*, gc_allocator_ignore_off_page<FrameBox*>>& boxes);
public:
    InlineNonReplacedBox(Node* node, ComputedStyle* style, Frame* parent, FrameInline* origin)
        : InlineBox(node, style, parent)
        , m_isCollapsed(false)
    {
        if (origin->isLeftMBPCleared())
            setLeftMBPCleared();
        if (origin->isRightMBPCleared())
            setRightMBPCleared();
        m_origin = origin;
        m_descender = m_ascender = 0;

        // recompute style flags
        // we should re compute flgas here
        // because, when ctor of Frame is Executed, vtable is not setted correctly
        // so we could not consider that what kind of frame is this
        computeStyleFlags();
    }

    virtual bool isInlineNonReplacedBox() const { return true; }
    virtual const char* name()
    {
        return "InlineNonReplacedBox";
    }
    static InlineNonReplacedBox* layoutInline(InlineNonReplacedBox* self, LayoutContext& ctx, FrameBlockBox* blockBox,
        LineFormattingContext* lineFormattingContext, InlineNonReplacedBox* layoutParentBox, bool freshStart);
    virtual void paint(PaintingContext& ctx);
    virtual void paintChildrenWith(PaintingContext& ctx)
    {
        auto iter = boxes().begin();
        while (iter != boxes().end()) {
            FrameBox* child = *iter;
            ctx.m_canvas->save();
            ctx.m_canvas->translate(child->asFrameBox()->x(), child->asFrameBox()->y());
            child->paint(ctx);
            ctx.m_canvas->restore();
            iter++;
        }
    }
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage);
#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth);
#endif
    virtual void iterateChildBoxes(const std::function<bool(FrameBox*)>& fn, const std::function<void(FrameBox*)>& beforeIterateChild = nullptr, const std::function<void(FrameBox*)>& afterIterateChild = nullptr)
    {
        if (!fn(this))
            return;

        if (beforeIterateChild)
            beforeIterateChild(this);
        for (size_t i = 0; i < m_boxes.size(); i ++) {
            m_boxes[i]->iterateChildBoxes(fn, beforeIterateChild, afterIterateChild);
        }
        if (afterIterateChild)
            afterIterateChild(this);
    }

    virtual void paintBackgroundAndBorders(Canvas* canvas);

    LayoutUnit ascender()
    {
        return m_ascender;
    }

    LayoutUnit decender()
    {
        return m_descender;
    }

    void setAscender(const LayoutUnit& a)
    {
        m_ascender = a;
    }

    void setDecender(const LayoutUnit& a)
    {
        m_descender = a;
    }

    FrameInline* origin()
    {
        return m_origin;
    }

    std::vector<FrameBox*, gc_allocator_ignore_off_page<FrameBox*> >& boxes()
    {
        return m_boxes;
    }

    bool isCollapsed()
    {
        return m_isCollapsed;
    }

    void markCollapsed()
    {
        m_isCollapsed = true;
    }

protected:
    bool m_isCollapsed;
    LayoutUnit m_ascender;
    LayoutUnit m_descender;
    FrameInline* m_origin;
    LayoutBoxSurroundData m_orgPadding, m_orgBorder, m_orgMargin;
    std::vector<FrameBox*, gc_allocator_ignore_off_page<FrameBox*> > m_boxes;
};

class LineBox : public FrameBox {
    friend class LineFormattingContext;
    friend class FrameBlockBox;
    friend class InlineNonReplacedBox;

public:
    LineBox(Frame* parent)
        : FrameBox(nullptr, nullptr)
        , m_ascender(0)
        , m_descender(0)
        , m_heightComputed(false)
    {
        setParent(parent);
    }

    virtual bool isLineBox()
    {
        return true;
    }

    LayoutUnit ascender()
    {
        return m_ascender;
    }

    LayoutUnit decender()
    {
        return m_descender;
    }

    std::vector<FrameBox*, gc_allocator_ignore_off_page<FrameBox*> >& boxes()
    {
        return m_boxes;
    }

    virtual const char* name()
    {
        return "LineBox";
    }

    virtual void iterateChildBoxes(const std::function<bool(FrameBox*)>& fn, const std::function<void(FrameBox*)>& beforeIterateChild = nullptr, const std::function<void(FrameBox*)>& afterIterateChild = nullptr)
    {
        if (!fn(this))
            return;

        if (beforeIterateChild)
            beforeIterateChild(this);

        for (size_t i = 0; i < m_boxes.size(); i ++) {
            m_boxes[i]->iterateChildBoxes(fn, beforeIterateChild, afterIterateChild);
        }

        if (afterIterateChild)
            afterIterateChild(this);
    }

    void markHeightComputed()
    {
        m_heightComputed = true;
    }

    bool heightComputed()
    {
        return m_heightComputed;
    }

protected:
    std::vector<FrameBox*, gc_allocator_ignore_off_page<FrameBox*> > m_boxes;
    // FIXME
    // we use these value only for vertical-align of inline-block
    // in layout, we use only 'ascender'
    // should we delete m_decender?
    LayoutUnit m_ascender;
    LayoutUnit m_descender;
    bool m_heightComputed;
};

class MarginInfo {
public:
    MarginInfo(LayoutUnit topBorderPadding, LayoutUnit bottomBorderPadding, bool isNewContext, Length height)
    {
        m_canCollapseWithChildren = !isNewContext;
        m_canCollapseTopWithChildren = m_canCollapseWithChildren && !topBorderPadding;
        m_canCollapseBottomWithChildren = m_canCollapseWithChildren && !bottomBorderPadding && height.isAuto();
        m_atTopSideOfBlock = true;
    }

    void setMaxPositiveMarginTop(LayoutUnit m)
    {
        m_maxPositiveMarginTop = m;
    }

    LayoutUnit maxPositiveMarginTop()
    {
        return m_maxPositiveMarginTop;
    }

    void setMaxNegativeMarginTop(LayoutUnit m)
    {
        m_maxNegativeMarginTop = m;
    }

    LayoutUnit maxNegativeMarginTop()
    {
        return m_maxNegativeMarginTop;
    }

    void setPositiveMargin(LayoutUnit m)
    {
        m_positiveMargin = m;
    }

    LayoutUnit positiveMargin()
    {
        return m_positiveMargin;
    }

    void setNegativeMargin(LayoutUnit m)
    {
        m_negativeMargin = m;
    }

    LayoutUnit negativeMargin()
    {
        return m_negativeMargin;
    }

    void setMargin(LayoutUnit pos, LayoutUnit neg)
    {
        STARFISH_ASSERT(pos >= 0 && neg >= 0);
        m_positiveMargin = pos;
        m_negativeMargin = neg;
    }

    void setMargin(LayoutUnit val)
    {
        if (val >= 0) {
            setMargin(val, 0);
        } else {
            setMargin(0, -val);
        }
    }

    bool canCollapseTopWithChildren()
    {
        return m_canCollapseTopWithChildren;
    }

    void setAtTopSideOfBlock(bool b) { m_atTopSideOfBlock = b; }

    bool atTopSideOfBlock() { return m_atTopSideOfBlock; }

    bool canCollapseWithMarginTop()
    {
        return m_atTopSideOfBlock && m_canCollapseTopWithChildren;
    }

    bool canCollapseWithMarginBottom()
    {
        return m_canCollapseBottomWithChildren;
    }

    bool canCollapseBottomWithChildren()
    {
        return m_canCollapseBottomWithChildren;
    }

    void setCanCollapseBottomWithChildren(bool v)
    {
        m_canCollapseBottomWithChildren = v;
    }

    bool m_canCollapseWithChildren;
    bool m_canCollapseTopWithChildren;
    bool m_canCollapseBottomWithChildren;
    bool m_atTopSideOfBlock;
    LayoutUnit m_maxPositiveMarginTop;
    LayoutUnit m_maxNegativeMarginTop;
    LayoutUnit m_positiveMargin;
    LayoutUnit m_negativeMargin;
};

class FrameBlockBox : public FrameBox {
    friend class LineFormattingContext;
    friend class InlineNonReplacedBox;
    friend class FrameDocument;
public:
    FrameBlockBox(Node* node, ComputedStyle* style)
        : FrameBox(node, style)
        , m_marginInfo(nullptr)
        , m_heightComputed(false)
    {
        STARFISH_ASSERT((node == nullptr && style != nullptr) || (node != nullptr && style == nullptr));
    }

    virtual bool isFrameBlockBox()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameBlockBox";
    }

    FrameDocument* asFrameDocument()
    {
        return (FrameDocument*)this;
    }

    bool isNecessaryBlockBox()
    {
        if (!node()) {
            if (firstChild() && firstChild()->isFrameInline()) {
                FrameInline* fi = firstChild()->asFrameInline();
                if (fi->isLeftMBPCleared() && fi->isRightMBPCleared()) {
                    if (!fi->firstChild())
                        return false;
                    if (fi->firstChild() == fi->lastChild() && fi->firstChild()->isFrameText()) {
                        if (fi->firstChild()->asFrameText()->text()->containsOnlyWhitespace())
                            return false;
                    }
                }
            }
        }
        return true;
    }

    void setMarginInfo(MarginInfo * marginInfo)
    {
        m_marginInfo = marginInfo;
    }

    MarginInfo* marginInfo()
    {
        return m_marginInfo;
    }

    void markHeightComputed(bool b)
    {
        m_heightComputed = b;
    }

    bool heightComputed()
    {
        return m_heightComputed;
    }

    virtual void layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat);
    virtual void computePreferredWidth(PreferredWidthContext& ctx);

#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth);
#endif
    virtual void paint(PaintingContext& ctx);
    virtual void paintChildrenWith(PaintingContext& ctx);
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage);
    virtual Frame* hitTestChildrenWith(LayoutUnit x, LayoutUnit y, HitTestStage stage);

    virtual void iterateChildBoxes(const std::function<bool(FrameBox*)>& fn, const std::function<void(FrameBox*)>& beforeIterateChild = nullptr, const std::function<void(FrameBox*)>& afterIterateChild = nullptr)
    {
        if (hasBlockFlow()) {
            FrameBox::iterateChildBoxes(fn, beforeIterateChild, afterIterateChild);
            return;
        }

        if (!fn(this))
            return;

        if (beforeIterateChild)
            beforeIterateChild(this);

        for (size_t i = 0; i < m_lineBoxes.size(); i ++) {
            m_lineBoxes[i]->iterateChildBoxes(fn, beforeIterateChild, afterIterateChild);
        }

        if (afterIterateChild)
            afterIterateChild(this);
    }

    virtual bool hasBlockFlow()
    {
        if (!firstChild())
            return true;

        Frame* child = firstChild();
        return (child->style()->originalDisplay() == BlockDisplayValue) && child->isNormalFlow();
    }

    virtual bool isSelfCollapsingBlock(LayoutContext& ctx)
    {
        if (isEstablishesBlockFormattingContext())
            return false;

        if (!isNecessaryBlockBox())
            return true;

        if (heightComputed())
            return false;

        if (paddingHeight() || borderHeight()) {
            return false;
        }

        Length heightLength = style()->height();
        // NOTE: In case of percentage height,
        // if containing blocks' height is fixed, the block is not self-collaping block.
        if (heightLength.isPercent() && !heightLength.isZero() && ctx.parentHasFixedHeight(this)) {
            return false;
        }

        if (heightLength.isAuto() || heightLength.isZero()) {
            Frame* child = firstChild();
            while (child) {
                if (!child->isNormalFlow()) {
                    child = child->next();
                    continue;
                }
                if (!child->isSelfCollapsingBlock(ctx))
                    return false;
                child = child->next();
            }
            return true;
        }
        return false;
    }

    GCVector<LineBox*>& lineBoxes()
    {
        return m_lineBoxes;
    }

protected:
    LayoutUnit layoutBlock(LayoutContext& ctx);
    LayoutUnit layoutInline(LayoutContext& ctx);

    std::vector<LineBox*, gc_allocator_ignore_off_page<LineBox*> > m_lineBoxes;
    MarginInfo* m_marginInfo;
    bool m_heightComputed;
};

struct DataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine {
    DataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine()
    {
        m_isFirstEdgeProcessed = false;
    }
    LayoutBoxSurroundData m_margin;
    LayoutBoxSurroundData m_border;
    LayoutBoxSurroundData m_padding;
    LayoutBoxSurroundData m_orgMargin;
    LayoutBoxSurroundData m_orgBorder;
    LayoutBoxSurroundData m_orgPadding;
    bool m_isFirstEdgeProcessed;
};

struct FloatingBoxLayoutContext {
    int m_hasFloat;
    LayoutUnit m_y;
    LayoutUnit m_accumulatedLeftFloatBoxWidth;
    LayoutUnit m_accumulatedRightFloatBoxWidth;
    LayoutUnit m_originalLineBoxX;
    LayoutUnit m_originalLineBoxWidth;

    FloatingBoxLayoutContext(int hasFloat, LayoutUnit y, LayoutUnit originalLineBoxX, LayoutUnit originalLineBoxWidth)
        : m_hasFloat(hasFloat), m_y(y), m_originalLineBoxX(originalLineBoxX), m_originalLineBoxWidth(originalLineBoxWidth)
    { }
};

struct FontHeights {
    LayoutUnit m_ascender;
    LayoutUnit m_descender;

    FontHeights(LayoutUnit ascender, LayoutUnit descender)
        : m_ascender(ascender), m_descender(descender)
    { }
};

class LineFormattingContext {
private:
    void resetLineBox();
    void registerInlineContent();
    LayoutUnit inlineBlockAscender(FrameBlockBox* box)
    {
        STARFISH_ASSERT(m_inlineBlockAscender.find(box) != m_inlineBlockAscender.end());
        return m_inlineBlockAscender[box];
    }

    void computeHorizontalProperties();
    LayoutUnit computeLineBoxHeight(FrameLineBreak* br, bool hasMoreInlineBoxes);

    void insertPendingFloatingBoxes(bool isLastLine, bool skipFinishLine);
    void insertPendingAbsolutePositionedBoxes();
    void insertPendingInlineBoxesDueToFloatingBoxes();
    void layoutLineBox(LayoutUnit yDiff, LayoutUnit height);

    void removeDanglingSpaceFromLine();
    void removeAllInlineBoxes();

    void reCacheFloatingBoxes(LayoutUnit xDiff);
    void makeFloatingBoxLayoutContext(LayoutUnit yDiff);
public:
    LineFormattingContext(FrameBlockBox& block, LayoutContext& ctx, const LayoutUnit& lineBoxX, const LayoutUnit& lineBoxY, const LayoutUnit& lineBoxWidth);

    FontHeights computeVerticalProperties(FrameBox* parentBox, ComputedStyle* parentStyle, bool dueToBr);

    void markInlineBoxIndex(FrameBox* box);
    void finishLine(FrameLineBreak* br, bool isLastLine);
    void breakLine(FrameLineBreak* br, bool isLastLine, bool skipFinishLine);
    void makeFloatingBoxLayoutContextDueToClearIfNeeds(FrameBox* box);
    template <typename Box>
    LayoutUnit layoutInlineBoxes(Box* parent, LayoutUnit start);

    /*
    bool isBreakedLineWithoutBR(size_t idx)
    {
        return m_breakedLinesSet.find(idx) != m_breakedLinesSet.end();
    }
    */

    void insertFloatingBoxAndReLayoutLineBoxIfNeeds(FrameBox* box);
    LineBox* currentLine()
    {
        return m_block.m_lineBoxes.back();
    }

    void registerInlineBlockAscender(LayoutUnit ascender, FrameBlockBox* box)
    {
        m_inlineBlockAscender[box] = ascender;
    }

    bool hasFloatingBoxAlreadyInLineBox(Frame* f)
    {
        if (f->style()->position() != AbsolutePositionValue && f->style()->floating() != NoneFloatValue) {
            FloatingBoxLayoutContext& fbCtx = (*m_floatingBoxLayoutContexts.rbegin());
            return fbCtx.m_hasFloat != HasNone;
        } else {
            FloatingBoxLayoutContext& fbCtx = (*m_floatingBoxLayoutContexts.begin());
            return fbCtx.m_hasFloat != HasNone;
        }
    }

    LayoutUnit m_leftBoundary;
    LayoutUnit m_rightBoundary;
    LayoutLocation m_absPosition;
    LayoutUnit m_lineBoxX;
    LayoutUnit m_lineBoxY;
    LayoutUnit m_currentLineWidth;
    LayoutUnit m_lineBoxWidth;
    size_t m_currentLine;
    FrameBlockBox& m_block;
    LayoutContext& m_layoutContext;
    bool m_shouldLineBreakForAbsolutePositionedBox;
    bool m_shouldLineBreakForBr;
    enum HasFloat {
        HasNone,
        HasLeft,
        HasRight,
    };
    size_t m_inlineBoxIndex;
    size_t m_pendingFloatingBoxNumsBeforeCurrentLine;
    size_t m_floatingBoxesSizeBeforeCurrentLine;

    // std::set<size_t> m_breakedLinesSet;

    // we dont need gc_allocater here
    // frame tree has strong reference already
    std::unordered_map<FrameBlockBox*, LayoutUnit> m_inlineBlockAscender;

    std::vector<FloatingBoxLayoutContext> m_floatingBoxLayoutContexts;
    std::vector<std::pair<FrameBox*, bool> > m_absolutePositionedBoxes;
    std::vector<FrameBox*> m_pendingFloatingBoxes;
    std::vector<FrameBox*> m_pendingInlineBoxes;

    std::unordered_map<FrameInline*, DataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine> m_dataForRestoreLeftRightOfMBPAfterResolveBidiLinePerLine;
    std::unordered_map<FrameInline*, InlineNonReplacedBox*> m_checkLastInlineNonReplacedPerLine;
    std::unordered_map<Frame*, DirectionValue> m_computedDirectionValuePerFrame;
    std::unordered_map<FrameText* , std::vector<TextRun>> m_textRunsPerFrameText;
};
}

#endif
