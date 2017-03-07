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
class InlineTextBox;        // TextNode
class InlineNonReplacedBox; // non-replaced element, display: inline

class InlineBox : public FrameBox {
public:
    InlineBox(Node* node, ComputedStyle* style) : FrameBox(node, style)
    {
    }

    Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage)
    {
        if (stage == HitTestStage::HitTestNormalFlowInline) {
            return FrameBox::hitTest(x, y, stage);
        }
        return nullptr;
    }

    virtual bool isInlineBox()
    {
        return true;
    }

    virtual bool isInlineTextBox() const
    {
        return false;
    }

    virtual bool isInlineNonReplacedBox() const
    {
        return false;
    }

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

    TextRun(FrameText* frameText, String* str, size_t startPosition,
            size_t endPosition, CharDirection dir)
        : m_stringView(StringView(str, startPosition, endPosition))
    {
        m_frameText = frameText;
        m_direction = dir;
    }
};

class InlineTextBox : public InlineBox {
public:
    InlineTextBox(FrameText* frame, const TextRun& run)
        : InlineBox(frame->node(), frame->style()), m_textRun(run)
    {
    }

    virtual bool isInlineTextBox() const
    {
        return true;
    }

    virtual void paint(PaintingContext& ctx);
#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth)
    {
        InlineBox::dump(depth);
        printf(" [(%s), dir: %d, start: %d, end %d] ",
               m_textRun.m_stringView.substring()->utf8Data(),
               (int)charDirection(), (int)m_textRun.m_stringView.start(),
               (int)m_textRun.m_stringView.end());
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

template <typename Box>
class InlineBoxLayoutParentBox {
public:
    LayoutUnit ascender() const
    {
        return m_ascender;
    }

    LayoutUnit decender() const
    {
        return m_descender;
    }

    void setAscDescender(LayoutUnit ascender, LayoutUnit descender)
    {
        m_ascender = ascender;
        m_descender = descender;
        ((Box*)this)->setHeight(m_ascender - m_descender);
    }

    GCVector<FrameBox*>& boxes()
    {
        return m_boxes;
    }

    void iterateInlineBoxes(
        const std::function<bool(FrameBox*)>& fn,
        const std::function<void(FrameBox*)>& beforeIterateChild = nullptr,
        const std::function<void(FrameBox*)>& afterIterateChild = nullptr)
    {
        if (!fn((Box*)this)) {
            return;
        }

        if (beforeIterateChild) {
            beforeIterateChild((Box*)this);
        }

        for (size_t i = 0; i < m_boxes.size(); i++) {
            m_boxes[i]->iterateChildBoxes(fn, beforeIterateChild,
                                          afterIterateChild);
        }

        if (afterIterateChild) {
            afterIterateChild((Box*)this);
        }
    }

    FrameBox* firstInlineBox();
    FrameBox* lastInlineBox();
    void removeDanglingSpace(LineFormattingContext* ctx);
    bool containOnlyEmptyInlineNonReplacedBoxes();

    void insertInlineBox(FrameBox* box)
    {
        m_boxes.push_back(box);
        box->setLayoutParent((Box*)this);
    }
    LayoutUnit layoutInlineBoxes(LayoutUnit start);
    void registerRelativePositionedBoxes(LayoutContext& ctx);

    size_t absolutePositionedBoxLayoutParentCnt() const
    {
        return m_absolutePositionedLayoutParentCnt;
    }

    void markAbsolutePositionedBoxLayoutParent()
    {
        m_absolutePositionedLayoutParentCnt++;
    }

    void unMarkAbsolutePositionedBoxLayoutParent()
    {
        STARFISH_ASSERT(m_absolutePositionedLayoutParentCnt != 0);
        m_absolutePositionedLayoutParentCnt--;
    }

    void moveToNewLineBox(FrameBox* box, LineBox* lineBox);

    void setLeftMBPs();
    void setRightMBPs();

protected:
    LayoutUnit m_ascender;
    LayoutUnit m_descender;
    GCVector<FrameBox*> m_boxes;
    size_t m_absolutePositionedLayoutParentCnt;
};

enum MBPStatus {
    None = 0,
    ProcessedStaringMBP = 1,
    ProcessedEndingMBP = 2,
    SetLeftMBP = 4,
    SetRightMBP = 8,
};

class InlineNonReplacedBox
    : public InlineBox,
      public InlineBoxLayoutParentBox<InlineNonReplacedBox> {
    friend class FrameBlockBox;
    friend class LineFormattingContext;

public:
    InlineNonReplacedBox(InlineNonReplacedBox* inlineBox)
        : InlineNonReplacedBox(inlineBox, inlineBox->origin())
    {
        m_mbpStatus = inlineBox->m_mbpStatus;

        m_ascender = inlineBox->m_ascender;
        m_descender = inlineBox->m_descender;

        m_margin = inlineBox->m_margin;
        m_border = inlineBox->m_border;
        m_padding = inlineBox->m_padding;

        m_orgMargin = inlineBox->m_orgMargin;
        m_orgBorder = inlineBox->m_orgBorder;
        m_orgPadding = inlineBox->m_orgPadding;
    }

    InlineNonReplacedBox(FrameInline* frame)
        : InlineNonReplacedBox(frame, frame)
    {
        m_mbpStatus = new (UseGC) unsigned int;
        *m_mbpStatus = 0;

        m_ascender = 0;
        m_descender = 0;
    }

    virtual bool isInlineNonReplacedBox() const
    {
        return true;
    }

    virtual const char* name()
    {
        return "InlineNonReplacedBox";
    }

    void layoutInline(LineFormattingContext* lineFormattingContext);
    virtual void paint(PaintingContext& ctx);
    virtual void paintChildrenWith(PaintingContext& ctx)
    {
        auto iter = boxes().begin();
        while (iter != boxes().end()) {
            FrameBox* child = *iter;
            ctx.m_canvas->save();
            ctx.m_canvas->translate(child->asFrameBox()->x(),
                                    child->asFrameBox()->y());
            child->paint(ctx);
            ctx.m_canvas->restore();
            iter++;
        }
    }
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage);
#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth);
#endif
    virtual void iterateChildBoxes(
        const std::function<bool(FrameBox*)>& fn,
        const std::function<void(FrameBox*)>& beforeIterateChild = nullptr,
        const std::function<void(FrameBox*)>& afterIterateChild = nullptr)
    {
        InlineBoxLayoutParentBox<InlineNonReplacedBox>::iterateInlineBoxes(
            fn, beforeIterateChild, afterIterateChild);
    }

    virtual void paintBackgroundAndBorders(Canvas* canvas);

    FrameInline* origin()
    {
        return m_origin;
    }

    bool isCollapsed() const
    {
        return m_isCollapsed;
    }

    void markCollapsed()
    {
        m_isCollapsed = true;
    }

    bool isProcessedStartingMBP() const
    {
        return ((*m_mbpStatus) & ProcessedStaringMBP) != 0;
    }

    void markProcessedStartingMBP()
    {
        STARFISH_ASSERT(!isProcessedStartingMBP());
        (*m_mbpStatus) |= ProcessedStaringMBP;
    }

    bool isProcessedEndingMBP() const
    {
        return ((*m_mbpStatus) & ProcessedEndingMBP) != 0;
    }

    void markProcessedEndingMBP()
    {
        STARFISH_ASSERT(!isProcessedEndingMBP());
        (*m_mbpStatus) |= ProcessedEndingMBP;
    }

    bool isSetLeftMBP() const
    {
        return ((*m_mbpStatus) & SetLeftMBP) != 0;
    }

    void markSetLeftMBP()
    {
        STARFISH_ASSERT(!isSetLeftMBP());
        (*m_mbpStatus) |= SetLeftMBP;
    }

    bool isSetRightMBP() const
    {
        return ((*m_mbpStatus) & SetRightMBP) != 0;
    }

    void markSetRightMBP()
    {
        STARFISH_ASSERT(!isSetRightMBP());
        (*m_mbpStatus) |= SetRightMBP;
    }

    void processStartingMBP(LineFormattingContext* lineFormattingContext);
    void processEndingMBP(LineFormattingContext* lineFormattingContext);
    void setOrgLeftMBP();
    void setOrgRightMBP();

    void unsetLeftMBP()
    {
        m_margin.setLeft(0);
        m_border.setLeft(0);
        m_padding.setLeft(0);
    }

    void unsetRightMBP()
    {
        m_margin.setRight(0);
        m_border.setRight(0);
        m_padding.setRight(0);
    }

protected:
    bool m_isCollapsed;
    FrameInline* m_origin;
    unsigned* m_mbpStatus;
    LayoutBoxSurroundData m_orgPadding, m_orgBorder, m_orgMargin;

    InlineNonReplacedBox(Frame* frame, FrameInline* origin)
        : InlineBox(frame->node(), frame->style())
        , m_isCollapsed(false)
        , m_origin(origin)
        , m_mbpStatus(nullptr)
    {
        if (origin->isLeftMBPCleared()) {
            setLeftMBPCleared();
        }

        if (origin->isRightMBPCleared()) {
            setRightMBPCleared();
        }

        m_absolutePositionedLayoutParentCnt = 0;

        // recompute style flags
        // we should re compute flags here
        // because, when ctor of Frame is executed, vtable is not set correctly
        // so we could not consider that what kind of frame is this
        computeStyleFlags();
    }

    void setTopBottomOrgMBP()
    {
        m_orgMargin.setTop(m_margin.top());
        m_orgMargin.setBottom(m_margin.bottom());
        m_orgBorder.setTop(m_border.top());
        m_orgBorder.setBottom(m_border.bottom());
        m_orgPadding.setTop(m_padding.top());
        m_orgPadding.setBottom(m_padding.bottom());
    }

    void unsetTopBottomMBP()
    {
        m_margin.setTop(0);
        m_margin.setBottom(0);
        m_border.setTop(0);
        m_border.setBottom(0);
        m_padding.setTop(0);
        m_padding.setBottom(0);
    }
};

class LineBox : public FrameBox, public InlineBoxLayoutParentBox<LineBox> {
    friend class LineFormattingContext;
    friend class FrameBlockBox;
    friend class InlineNonReplacedBox;

public:
    LineBox(Frame* parent) : FrameBox(nullptr, nullptr)
    {
        setParent(parent);
        m_ascender = 0;
        m_descender = 0;
        m_absolutePositionedLayoutParentCnt = 0;
    }

    virtual bool isLineBox()
    {
        return true;
    }

    virtual const char* name()
    {
        return "LineBox";
    }

    virtual void iterateChildBoxes(
        const std::function<bool(FrameBox*)>& fn,
        const std::function<void(FrameBox*)>& beforeIterateChild = nullptr,
        const std::function<void(FrameBox*)>& afterIterateChild = nullptr)
    {
        InlineBoxLayoutParentBox<LineBox>::iterateInlineBoxes(
            fn, beforeIterateChild, afterIterateChild);
    }
};

class MarginInfo {
public:
    MarginInfo(LayoutUnit topBorderPadding, LayoutUnit bottomBorderPadding,
               bool isNewContext, Length height)
    {
        m_canCollapseWithChildren = !isNewContext;
        m_canCollapseTopWithChildren =
            m_canCollapseWithChildren && !topBorderPadding;
        m_canCollapseBottomWithChildren = m_canCollapseWithChildren &&
                                          !bottomBorderPadding &&
                                          height.isAuto();
        m_atTopSideOfBlock = true;
    }

    void setMaxPositiveMarginTop(LayoutUnit m)
    {
        m_maxPositiveMarginTop = m;
    }

    LayoutUnit maxPositiveMarginTop() const
    {
        return m_maxPositiveMarginTop;
    }

    void setMaxNegativeMarginTop(LayoutUnit m)
    {
        m_maxNegativeMarginTop = m;
    }

    LayoutUnit maxNegativeMarginTop() const
    {
        return m_maxNegativeMarginTop;
    }

    void setPositiveMargin(LayoutUnit m)
    {
        m_positiveMargin = m;
    }

    LayoutUnit positiveMargin() const
    {
        return m_positiveMargin;
    }

    void setNegativeMargin(LayoutUnit m)
    {
        m_negativeMargin = m;
    }

    LayoutUnit negativeMargin() const
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

    bool canCollapseTopWithChildren() const
    {
        return m_canCollapseTopWithChildren;
    }

    void setAtTopSideOfBlock(bool b)
    {
        m_atTopSideOfBlock = b;
    }

    bool atTopSideOfBlock() const
    {
        return m_atTopSideOfBlock;
    }

    bool canCollapseWithMarginTop() const
    {
        return m_atTopSideOfBlock && m_canCollapseTopWithChildren;
    }

    bool canCollapseWithMarginBottom() const
    {
        return m_canCollapseBottomWithChildren;
    }

    bool canCollapseBottomWithChildren() const
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
        : FrameBox(node, style), m_marginInfo(nullptr), m_heightComputed(false)
    {
        STARFISH_ASSERT((node == nullptr && style != nullptr) ||
                        (node != nullptr && style == nullptr));
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

    void setMarginInfo(MarginInfo* marginInfo)
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

    bool heightComputed() const
    {
        return m_heightComputed;
    }

    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat);
    virtual void computePreferredWidth(PreferredWidthContext& ctx);

#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth);
#endif
    virtual void paint(PaintingContext& ctx);
    virtual void paintChildrenWith(PaintingContext& ctx);
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage);
    virtual Frame* hitTestChildrenWith(LayoutUnit x, LayoutUnit y,
                                       HitTestStage stage);

    virtual void iterateChildBoxes(
        const std::function<bool(FrameBox*)>& fn,
        const std::function<void(FrameBox*)>& beforeIterateChild = nullptr,
        const std::function<void(FrameBox*)>& afterIterateChild = nullptr)
    {
        if (hasBlockFlow()) {
            FrameBox::iterateChildBoxes(fn, beforeIterateChild,
                                        afterIterateChild);
            return;
        }

        if (!fn(this)) {
            return;
        }

        if (beforeIterateChild) {
            beforeIterateChild(this);
        }

        for (size_t i = 0; i < m_lineBoxes.size(); i++) {
            m_lineBoxes[i]->iterateChildBoxes(fn, beforeIterateChild,
                                              afterIterateChild);
        }

        if (afterIterateChild) {
            afterIterateChild(this);
        }
    }

    virtual bool hasBlockFlow()
    {
        if (!firstChild()) {
            return true;
        }
        Frame* child = firstChild();
        return (child->isBlockLevel() && child->isNormalFlow());
    }

    virtual bool isSelfCollapsingBlock(LayoutContext& ctx)
    {
        if (isEstablishesBlockFormattingContext()) {
            return false;
        }

        if (!isNecessaryBlockBox()) {
            return true;
        }

        if (heightComputed()) {
            return false;
        }

        if (paddingHeight() || borderHeight()) {
            return false;
        }

        Length heightLength = style()->height();
        // NOTE: In case of percentage height,
        // if containing blocks' height is fixed, the block is not
        // self-collapsing block.
        if (heightLength.isPercent() && !heightLength.isZero() &&
            ctx.parentHasFixedHeight(this)) {
            return false;
        }

        if (heightLength.isAuto() || heightLength.isZero()) {
            Frame* child = firstChild();
            while (child) {
                if (!child->isNormalFlow()) {
                    child = child->next();
                    continue;
                }
                if (!child->isSelfCollapsingBlock(ctx)) {
                    return false;
                }
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
    void computeContentWidth(LayoutContext& ctx,
                             LayoutUnit containgBlockContentWidth,
                             LayoutUnit absX = 0);

    GCVector<LineBox*> m_lineBoxes;
    MarginInfo* m_marginInfo;
    bool m_heightComputed;
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

class LineFormattingContext {
private:
    void resetLineBox();
    void registerInlineContent();
    LayoutUnit inlineBlockAscender(FrameBlockBox* box)
    {
        STARFISH_ASSERT(m_inlineBlockAscender.find(box) !=
                        m_inlineBlockAscender.end());
        return m_inlineBlockAscender[box];
    }

    void computeHorizontalProperties();
    LayoutUnit distanceToNextLineBox(FrameLineBreak* br,
                                     bool hasMoreInlineBoxes);

    void insertPendingFloatingBoxes();
    void insertPendingInlineBoxes();
    void unregisterAbsolutePositionedBoxes();
    void markInlineBoxIndex(FrameBox* box);
    void layoutLineBox(LayoutUnit yDiff, LayoutUnit height);

    void generateInlineBox(FrameBox* box);
    void generateInlineTextBox(TextToken& token);
    void generateInlineNonReplacedBox(FrameInline* f);
    void generateFloatingBoxAndReLayoutLineBoxIfNeeds(FrameBox* box);
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

    bool canInsertToLineBox(Frame* f, LayoutUnit width);
    bool hasFloatingBoxAlreadyInLineBox(Frame* f);
    bool canInsertFloatingBox(FrameBox* f, bool allowPendingFloatingBox);

    void collectComputeDirectionsCandidate(Frame* parent,
                                           std::vector<Frame*>& frames);

public:
    LineFormattingContext(FrameBlockBox* block, LayoutContext& ctx);

    void computeVerticalProperties(FrameBox* parentBox, bool dueToBr);

    void generateInlineBoxes(Frame* origin);

    void finishLineForLineBox(FrameLineBreak* br, bool isLastLine);
    void finishLineForInlineNonReplacedBox(FrameLineBreak* br, bool isLastNode);
    void breakLine(FrameLineBreak* br);
    void makeFloatingBoxLayoutContextDueToClearIfNeeds(FrameBox* box);

    /*
    bool isBreakedLineWithoutBR(size_t idx)
    {
        return m_breakedLinesSet.find(idx) != m_breakedLinesSet.end();
    }
    */

    LineBox* currentLine()
    {
        return m_block->m_lineBoxes.back();
    }

    void handleTextToken(TextToken& token);

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

    void registerInlineBlockAscender(LayoutUnit ascender, FrameBlockBox* box)
    {
        m_inlineBlockAscender[box] = ascender;
    }

    bool dontBreakLine(Frame* f, LayoutUnit width);

    void computeDirection(Frame* parent, DirectionValue direction);

    LayoutUnit m_leftBoundary;
    LayoutUnit m_rightBoundary;
    LayoutLocation m_absPosition;
    LayoutUnit m_lineBoxX;
    LayoutUnit m_lineBoxY;
    LayoutUnit m_currentLineWidth;
    LayoutUnit m_lineBoxWidth;
    LayoutUnit m_unprocessedStartingMBPWidth;
    size_t m_currentLine;
    FrameBlockBox* m_block;
    LayoutContext& m_layoutContext;
    // This layout parent should be either LineBox or InlineNonReplacedBox
    FrameBox* m_currentLayoutParent;
    FrameText* m_lastFrameText;
    bool m_isPendingBreakLine;
    bool m_isWhiteSpaceAtLast;
    size_t m_inlineBoxIndex;
    size_t m_pendingFloatingBoxNumsBeforeCurrentLine;
    size_t m_floatingBoxesSizeBeforeCurrentLine;

    // std::set<size_t> m_breakedLinesSet;

    // we don't need gc_allocater here
    // frame tree has strong reference already
    std::unordered_map<FrameBlockBox*, LayoutUnit> m_inlineBlockAscender;

    std::vector<FloatingBoxLayoutContext> m_floatingBoxLayoutContexts;
    std::vector<FrameBox*> m_absolutePositionedBoxes;
    std::vector<FrameBox*> m_pendingFloatingBoxes;
    std::vector<FrameBox*> m_pendingInlineBoxes;

    std::unordered_map<Frame*, DirectionValue> m_computedDirectionValuePerFrame;
    std::unordered_map<FrameText*, std::vector<TextRun>> m_textRunsPerFrameText;
};
}

#endif
