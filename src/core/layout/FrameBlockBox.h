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

#ifndef __StarFishFrameBlockBox__
#define __StarFishFrameBlockBox__

#include "core/layout/FrameBox.h"
#include "core/layout/FrameReplaced.h"
#include "core/layout/FrameInline.h"
#include "core/layout/FrameLineBreak.h"
#include "core/layout/FrameText.h"

namespace StarFish {

class FrameBlockBox;
class LineFormattingContext;

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
#ifndef NDEBUG
    void dump() const
    {
        std::string str = m_stringView.substring()->utf8Data();
        str = FrameText::replaceAll(str, "\n", "\\n");
        printf("%s", m_direction == Ltr
                         ? "L"
                         : (m_direction == Rtl
                                ? "R"
                                : (m_direction == Neutral ? "N" : "M")));
        printf("[%s]", str.data());
        printf(":%zu\n", m_stringView.length());
    }
#endif
};

class InlineTextBox : public FrameBox {
public:
    InlineTextBox(FrameText* frame, const TextRun& run, bool isFirstLine)
        : FrameBox(frame->node(), frame->style())
        , m_textRun(run)
        , m_isFirstLine(isFirstLine)
    {
    }

    virtual bool isInlineBox() const
    {
        return true;
    }

    virtual bool isInlineTextBox() const
    {
        return true;
    }

    virtual void paint(PaintingContext& ctx);
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage)
    {
        if (stage == HitTestStage::HitTestNormalFlowInline) {
            return FrameBox::hitTest(x, y, stage);
        }
        return nullptr;
    }

#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth)
    {
        FrameBox::dump(depth);
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

    void unmarkFirstLine()
    {
        m_isFirstLine = false;
        setWidth(style()->font()->measureText(textRun().m_stringView));
        setHeight(style()->font()->metrics().m_fontHeight);
    }

    bool isFirstLine()
    {
        return m_isFirstLine;
    }

    ComputedStyle* style()
    {
        Frame* parent = layoutParent();
        while (parent->isLineBox()) {
            parent = parent->layoutParent();
        }

        return Frame::style(parent, parent->style(), m_isFirstLine);
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
    TextRun m_textRun;
    bool m_isFirstLine;
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

    virtual bool isInlineBoxLayoutParentBox() const
    {
        return true;
    }

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
        setHeight(m_ascender - m_descender);
    }

    GCVector<FrameBox*>& boxes()
    {
        return m_boxes;
    }

    virtual void establishesStackingContextIfNeeds()
    {
        for (size_t i = 0; i < m_boxes.size(); i++) {
            m_boxes[i]->establishesStackingContextIfNeeds();
        }
    }

    virtual InlineNonReplacedBox* firstInlineNonReplacedBox(FrameInline* f)
    {
        InlineNonReplacedBox* ret = nullptr;
        for (size_t i = 0; i < m_boxes.size(); i++) {
            if ((ret = m_boxes[i]->firstInlineNonReplacedBox(f))) {
                return ret;
            }
        }

        return ret;
    }

    virtual void computeVisibleRect(StackingContext* sCtx, LayoutLocation& loc);

    FrameBox* firstInlineBox();
    FrameBox* lastInlineBox();
    void removeDanglingSpace(LineFormattingContext* ctx);
    bool containOnlyEmptyInlineNonReplacedBoxes();
    bool isAbsolutePositionedBoxLayoutParent();

    void insertInlineBox(FrameBox* box)
    {
        m_boxes.push_back(box);
        box->setLayoutParent(this);
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

    virtual void iterateChildFrameBox(const std::function<void(FrameBox*)>& fn)
    {
        fn(this);
        for (size_t i = 0; i < m_boxes.size(); i++) {
            m_boxes[i]->iterateChildFrameBox(fn);
        }
    }

protected:
    LayoutUnit m_ascender;
    LayoutUnit m_descender;
    GCVector<FrameBox*> m_boxes;
    size_t m_absolutePositionedLayoutParentCnt;

    InlineBoxLayoutParentBox(Node* node, ComputedStyle* style)
        : FrameBox(node, style)
        , m_ascender(0)
        , m_descender(0)
        , m_absolutePositionedLayoutParentCnt(0)
    {
    }
};

class InlineNonReplacedBox : public InlineBoxLayoutParentBox {
    friend class FrameBlockBox;
    friend class LineFormattingContext;

    enum MBPStatus {
        None = 0,
        ProcessedStaringMBP = 1,
        ProcessedEndingMBP = 2,
        SetLeftMBP = 4,
        SetRightMBP = 8,
    };

public:
    InlineNonReplacedBox(InlineNonReplacedBox* inlineBox, bool isFirstLine)
        : InlineNonReplacedBox(inlineBox, inlineBox->origin(), isFirstLine)
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

    InlineNonReplacedBox(FrameInline* frame, bool isFirstLine)
        : InlineNonReplacedBox(frame, frame, isFirstLine)
    {
        m_mbpStatus = new (UseGC) unsigned int;
        *m_mbpStatus = 0;

        m_ascender = 0;
        m_descender = 0;
    }

    virtual bool isInlineBox() const
    {
        return true;
    }

    virtual bool isInlineNonReplacedBox() const
    {
        return true;
    }

    virtual const char* name()
    {
        return "InlineNonReplacedBox";
    }

    virtual void layoutInline(LineFormattingContext& lineFormattingContext);
    virtual void paint(PaintingContext& ctx);
    virtual void paintChildrenWith(PaintingContext& ctx);
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage);

    ComputedStyle* style()
    {
        return Frame::style(this, Frame::style(), m_isFirstLine);
    }
#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth);
#endif
    virtual void establishesStackingContextIfNeeds()
    {
        FrameBox::establishesStackingContextIfNeeds();

        InlineBoxLayoutParentBox::establishesStackingContextIfNeeds();
    }

    virtual InlineNonReplacedBox* firstInlineNonReplacedBox(FrameInline* f)
    {
        if (origin() == f) {
            return this;
        }

        return InlineBoxLayoutParentBox::firstInlineNonReplacedBox(f);
    }

    virtual void computeVisibleRect(StackingContext* sCtx, LayoutLocation& loc)
    {
        if (!tryUniteVisibleRect(sCtx, loc)) {
            return;
        }

        InlineBoxLayoutParentBox::computeVisibleRect(sCtx, loc);
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
    bool m_isFirstLine;

    InlineNonReplacedBox(Frame* frame, FrameInline* origin, bool isFirstLine)
        : InlineBoxLayoutParentBox(frame)
        , m_isCollapsed(false)
        , m_origin(origin)
        , m_mbpStatus(nullptr)
        , m_isFirstLine(isFirstLine)
    {
        if (origin->isLeftMBPCleared()) {
            setLeftMBPCleared();
        }

        if (origin->isRightMBPCleared()) {
            setRightMBPCleared();
        }

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

class LineBox : public InlineBoxLayoutParentBox {
    friend class LineFormattingContext;
    friend class FrameBlockBox;
    friend class InlineNonReplacedBox;

public:
    LineBox(Frame* parent)
        : InlineBoxLayoutParentBox()
    {
        setParent(parent);
    }

    virtual bool isLineBox()
    {
        return true;
    }

    virtual const char* name()
    {
        return "LineBox";
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
        : FrameBox(node, style)
        , m_marginInfo(nullptr)
        , m_heightComputed(false)
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
    virtual void layoutInline(LineFormattingContext& ctx);

#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth);
#endif
    virtual void paint(PaintingContext& ctx);
    virtual void paintChildrenWith(PaintingContext& ctx);
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage);
    virtual Frame* hitTestChildrenWith(LayoutUnit x, LayoutUnit y,
                                       HitTestStage stage);

    virtual InlineNonReplacedBox* firstInlineNonReplacedBox(FrameInline* f);
    virtual LineBox* firstLineBox();

    virtual void establishesStackingContextIfNeeds();
    virtual void computeVisibleRect(StackingContext* sCtx, LayoutLocation& loc);

    virtual bool hasBlockFlow()
    {
        if (!firstChild()) {
            return true;
        }
        Frame* child = firstChild();
        return (child->isBlockLevel() && child->isNormalFlow());
    }

    virtual bool isSelfCollapsingBlock(LayoutContext& ctx);
    GCVector<LineBox*>& lineBoxes()
    {
        return m_lineBoxes;
    }

    virtual void iterateChildFrameBox(const std::function<void(FrameBox*)>& fn)
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
                               (*m_boxes.begin())->boxWidth(),
                               [](LayoutUnit w, FrameBox* box) {
                                   if (box->isNormalFlow()) {
                                       return w + box->boxWidth();
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
    void layoutLineBox(LayoutUnit yDiff, LayoutUnit height);

    void generateInlineTextBox(TextToken& token);
    void insertFloatingBoxAndReLayoutLineBoxIfNeeds(FrameBox* box);
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

    bool canInsertToLineBox(Frame* f, LayoutUnit width);
    bool hasFloatingBoxAlreadyInLineBox(Frame* f);
    bool canInsertFloatingBox(FrameBox* f, bool allowPendingFloatingBox);

    void collectComputeDirectionsCandidate(Frame* parent,
                                           std::vector<Frame*>& frames);

public:
    LineFormattingContext(FrameBlockBox* block, LayoutContext& ctx);

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
    void registerRelativePositionedBoxes();
    LayoutUnit contentHeightForBlock();

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

    bool isFirstLineBox() const
    {
        return m_block->m_lineBoxes.size() == 1 && !m_isPendingBreakLine;
    }

    bool isWordProcessing() const
    {
        return !m_word.isEmpty();
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

    void tokenizeText(StarFish* sf, FrameText* f);

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
    InlineBoxLayoutParentBox* m_currentLayoutParent;
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
    GCVector<FrameBox*> m_pendingInlineBoxes;
    Word m_word;

    std::unordered_map<Frame*, DirectionValue> m_computedDirectionValuePerFrame;
    std::unordered_map<FrameText*, std::vector<TextRun>> m_textRunsPerFrameText;
};
}

#endif
