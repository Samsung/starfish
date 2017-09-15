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

#ifndef __StarFishFrame__
#define __StarFishFrame__

#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"

namespace StarFish {

class Canvas;
class ComputedStyle;
class Document;
class Frame;
class FrameBox;
class FrameBlockBox;
class FrameFlexibleBox;
class FrameInline;
class FrameLineBreak;
class FrameReplaced;
class FrameDocument;
class FrameTableObjectBox;
class FrameTableBox;
class FrameTableCaptionBox;
class FrameTableCellBox;
class FrameTableColBox;
class FrameTableRowBox;
class FrameTableSectionBox;
class FrameInputBox;
class FrameSVGBox;
class FrameText;
class InlineTextBox;
class InlineNonReplacedBox;
class InlineBoxLayoutParentBox;
class LineBox;
class LineFormattingContext;
class Node;
class StackingContext;

enum PaintingStage {
    PaintingNormalFlowBlock,     // the in-flow, non-inline-level,
                                 // non-positioned descendants.
    PaintingNonPositionedFloats, // the non-positioned float
    PaintingNormalFlowInline,    // the in-flow, inline-level, non-positioned
                                 // descendants, including inline tables and
                                 // inline blocks.
    PaintingPositionedElements,  // the child stacking contexts with stack level
                                 // 0 and the positioned descendants with stack
                                 // level 0.
    PaintingStageEnd
};

enum HitTestStage {
    HitTestPositionedElements,
    HitTestNormalFlowInline,
    HitTestNonPositionedFloats,
    HitTestNormalFlowBlock,
    HitTestStageEnd,
};

enum PaintingInlineStage {
    PaintingInlineBox,
    PaintingBlockBox,
    PaintingReplaced,
    PaintingInlineStageEnd
};

class LineBox;
class MarginInfo;
class FloatingBoxInfo;
class TextToken;

struct MarginCollapseResult {
    LayoutUnit m_advanceY;
    LayoutUnit m_normalFlowHeightAdvance;
};

FrameBlockBox* blockContainer(Frame* currentFrame);
FrameBlockBox* containingFrameBlockBox(Frame* currentFrame);
FrameBox* containingBlock(Frame* currentFrame);

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

struct PreferredWidthKey {
    LayoutUnit m_availableWidth;
    Frame* m_frame;

    PreferredWidthKey()
        : PreferredWidthKey(0, nullptr)
    {
    }

    PreferredWidthKey(LayoutUnit availableWidth, Frame* frame)
        : m_availableWidth(availableWidth)
        , m_frame(frame)
    {
    }
};
};

namespace std {
template <>
struct hash<StarFish::PreferredWidthKey> {
    size_t operator()(StarFish::PreferredWidthKey const& x) const
    {
        std::size_t seed = 0;
        hash_combine(seed, x.m_availableWidth.toInt());
        hash_combine(seed, x.m_frame);
        return seed;
    }
};

template <>
struct equal_to<StarFish::PreferredWidthKey> {
    bool operator()(StarFish::PreferredWidthKey const& a,
                    StarFish::PreferredWidthKey const& b) const
    {
        return a.m_availableWidth == b.m_availableWidth &&
               a.m_frame == b.m_frame;
    }
};
}

namespace StarFish {

struct PreferredWidthValue {
    LayoutUnit m_preferredWidth;
    LayoutUnit m_preferredMinWidth;

    PreferredWidthValue()
        : PreferredWidthValue(0, 0)
    {
    }

    PreferredWidthValue(LayoutUnit preferredWidth, LayoutUnit preferredMinWidth)
        : m_preferredWidth(preferredWidth)
        , m_preferredMinWidth(preferredMinWidth)
    {
    }
};

class LayoutContext {
public:
    LayoutContext(StarFish* starFish, FrameDocument* frameDocument)
        : m_starFish(starFish)
        , m_frameDocument(frameDocument)
    {
        establishBlockFormattingContext(true, true);
    }

    ~LayoutContext()
    {
        removeBlockFormattingContext();
        STARFISH_ASSERT(m_blockFormattingContextInfo.size() == 0);
        STARFISH_ASSERT(m_absolutePositionedBoxes.size() == 0);
        STARFISH_ASSERT(m_relativePositionedBoxes.size() == 0);
    }

    StarFish* starFish()
    {
        return m_starFish;
    }

    FrameDocument* frameDocument()
    {
        return m_frameDocument;
    }

    void establishBlockFormattingContext(bool isNormalFlow, bool isRoot = false)
    {
        if (!isNormalFlow || isRoot) {
            std::vector<FrameBlockBox*>* s = new std::vector<FrameBlockBox*>();
            std::vector<FloatingBoxInfo>* s2 =
                new std::vector<FloatingBoxInfo>();
            std::unordered_map<FrameBlockBox*, LayoutUnit>* s3 =
                new std::unordered_map<FrameBlockBox*, LayoutUnit>();
            std::unordered_map<Frame*, FrameBlockBox*>* s4 =
                new std::unordered_map<Frame*, FrameBlockBox*>();
            std::vector<FrameBlockBox*>* s5 = new std::vector<FrameBlockBox*>();
            std::unordered_map<FrameBlockBox*, std::pair<LineBox*, LayoutUnit>>*
                s6 = new std::unordered_map<FrameBlockBox*,
                                            std::pair<LineBox*, LayoutUnit>>();
            std::unordered_map<FrameTableCellBox*,
                               std::pair<LineBox*, LayoutUnit>>* s7 =
                new std::unordered_map<FrameTableCellBox*,
                                       std::pair<LineBox*, LayoutUnit>>();
            std::unordered_map<PreferredWidthKey, PreferredWidthValue>* s8 =
                new std::unordered_map<PreferredWidthKey,
                                       PreferredWidthValue>();
            m_blockFormattingContextInfo.emplace_back(
                isNormalFlow, isRoot, s, s2, s3, s4, s5, s6, s7, s8);
        } else {
            BlockFormattingContext& back = m_blockFormattingContextInfo.back();
            std::vector<FloatingBoxInfo>* s =
                new std::vector<FloatingBoxInfo>();
            m_blockFormattingContextInfo.emplace_back(
                isNormalFlow, isRoot, back.m_inlineBlockBoxStack, s,
                back.m_lineBoxAscenders, back.m_firstLineCandidates,
                back.m_blockBoxAligningAtFirstBaselineStack,
                back.m_firstLineAscenders, back.m_tempAscenders,
                back.m_preferredWidthValues);
        }
    }

    void removeBlockFormattingContext()
    {
        if (m_blockFormattingContextInfo.back().m_isRoot ||
            !m_blockFormattingContextInfo.back().m_isNormalFlow) {
            delete m_blockFormattingContextInfo.back().m_inlineBlockBoxStack;
            delete m_blockFormattingContextInfo.back().m_lineBoxAscenders;
            delete m_blockFormattingContextInfo.back().m_firstLineCandidates;
            delete m_blockFormattingContextInfo.back()
                .m_blockBoxAligningAtFirstBaselineStack;
            delete m_blockFormattingContextInfo.back().m_firstLineAscenders;
            delete m_blockFormattingContextInfo.back().m_tempAscenders;
            delete m_blockFormattingContextInfo.back().m_preferredWidthValues;
        }
        delete m_blockFormattingContextInfo.back().m_floatBoxes;
        m_blockFormattingContextInfo.pop_back();
    }

    void registerFloatingBox(FrameBox* box);
    void unregisterFloatingBoxes(size_t from);
    LayoutUnit clearedDistanceToFloatBottom(LayoutUnit yPosition,
                                            ClearValue clearValue,
                                            size_t* idx = nullptr);
    LayoutUnit nextDistanceToFloatBottom(LayoutUnit yPosition,
                                         LayoutUnit height);
    void resetLastTopLoc();
    LayoutUnit lastTopLoc();
    std::pair<LayoutUnit, LayoutUnit> horizontalBoundaryBetweenFloatingBoxes(
        LayoutUnit yPosition, LayoutUnit height, LayoutUnit left,
        LayoutUnit right);
    bool isCollidedWithFloatingBoxes(LayoutLocation loc, FrameBox* box,
                                     LayoutUnit leftBoundary,
                                     LayoutUnit rightBoundary);
    size_t floatingBoxesSize();
    void reCacheFloatingBoxes(size_t from);
    void reCacheFloatingBoxesByXDiff(size_t from, LayoutUnit xDiff);
    LayoutUnit parentContentWidth(Frame* currentFrame);
    bool parentHasFixedHeight(Frame* currentFrame);
    LayoutUnit parentFixedHeight(Frame* currentFrame);

    void pushInlineBlockBox(FrameBlockBox* blockBox)
    {
        m_blockFormattingContextInfo.back().m_inlineBlockBoxStack->push_back(
            blockBox);
    }

    void popInlineBlockBox()
    {
        m_blockFormattingContextInfo.back().m_inlineBlockBoxStack->pop_back();
    }

    void registerLineBoxAscender(FrameBlockBox* blockBox, LineBox* lb,
                                 LayoutUnit ascender);
    Nullable<LayoutUnit> lineBoxAscender(FrameBlockBox* box);

    void pushBlockBoxAligningAtFirstBaseline(FrameBlockBox* blockBox)
    {
        m_blockFormattingContextInfo.back()
            .m_blockBoxAligningAtFirstBaselineStack->push_back(blockBox);
    }

    void popBlockBoxAligningAtFirstBaseline()
    {
        m_blockFormattingContextInfo.back()
            .m_blockBoxAligningAtFirstBaselineStack->pop_back();
    }

    void registerFirstLineAscender(FrameBlockBox* owner, LineBox* lineBox,
                                   LayoutUnit ascender);
    Nullable<std::pair<LineBox*, LayoutUnit>> firstLineAscender(
        FrameBlockBox* blockBox);

    void tempReigsterFirstLineAscender(
        FrameTableCellBox* cellBox,
        std::pair<LineBox*, LayoutUnit> ascenderInfo);
    Nullable<std::pair<LineBox*, LayoutUnit>> tempFirstLineAscender(
        FrameTableCellBox* cellBox);

    Nullable<PreferredWidthValue> preferredWidthInfo(PreferredWidthKey key);
    void registerPreferredWidthInfo(PreferredWidthKey key,
                                    PreferredWidthValue value);

    void registerAbsolutePositionedBox(FrameBox* box);

    void layoutRegisteredAbsolutePositionedBoxes(
        FrameBlockBox* containingBlock);

    void registerRelativePositionedBox(FrameBox* box, bool dueToSelf);

    void layoutRegisteredRelativePositionedBoxes(
        FrameBlockBox* containingBlock);

    void setMarginCollapseResult(FrameBox* f, const MarginCollapseResult& r)
    {
        m_marginCollapseResult[f] = r;
    }

    const MarginCollapseResult& marginCollapseResult(FrameBox* f)
    {
        return m_marginCollapseResult[f];
    }

    void setMarginInfo(FrameBox* f, MarginInfo* marginInfo)
    {
        m_marginInfo[f] = marginInfo;
    }

    MarginInfo* marginInfo(FrameBox* f)
    {
        return m_marginInfo[f];
    }

    void setMaxPositiveMarginTop(LayoutUnit m)
    {
        m_blockFormattingContextInfo.back().m_maxPositiveMarginTop = m;
    }

    LayoutUnit maxPositiveMarginTop()
    {
        return m_blockFormattingContextInfo.back().m_maxPositiveMarginTop;
    }

    void setMaxNegativeMarginTop(LayoutUnit m)
    {
        m_blockFormattingContextInfo.back().m_maxNegativeMarginTop = m;
    }

    LayoutUnit maxNegativeMarginTop()
    {
        return m_blockFormattingContextInfo.back().m_maxNegativeMarginTop;
    }

    void setMaxMarginTop(LayoutUnit pos, LayoutUnit neg)
    {
        STARFISH_ASSERT(pos >= 0 && neg >= 0);
        m_blockFormattingContextInfo.back().m_maxPositiveMarginTop = pos;
        m_blockFormattingContextInfo.back().m_maxNegativeMarginTop = neg;
    }

    void setMaxPositiveMarginBottom(LayoutUnit m)
    {
        m_blockFormattingContextInfo.back().m_maxPositiveMarginBottom = m;
    }

    LayoutUnit maxPositiveMarginBottom()
    {
        return m_blockFormattingContextInfo.back().m_maxPositiveMarginBottom;
    }

    void setMaxNegativeMarginBottom(LayoutUnit m)
    {
        m_blockFormattingContextInfo.back().m_maxNegativeMarginBottom = m;
    }

    LayoutUnit maxNegativeMarginBottom()
    {
        return m_blockFormattingContextInfo.back().m_maxNegativeMarginBottom;
    }

    void setMaxMarginBottom(LayoutUnit pos, LayoutUnit neg)
    {
        STARFISH_ASSERT(pos >= 0 && neg >= 0);
        m_blockFormattingContextInfo.back().m_maxPositiveMarginBottom = pos;
        m_blockFormattingContextInfo.back().m_maxNegativeMarginBottom = neg;
    }

    bool canFloatCollapseWithMarginTop(size_t idx);

    bool checkIfThisIsFirstLineCandidate(FrameBlockBox* blockBox);

    LayoutUnit viewportWidth();
    LayoutUnit viewportHeight();

private:
    struct BlockFormattingContext {
        BlockFormattingContext(
            bool isNormalFlow, bool isRoot,
            std::vector<FrameBlockBox*>* inlineBlockBoxStack,
            std::vector<FloatingBoxInfo>* floatBoxes,
            std::unordered_map<FrameBlockBox*, LayoutUnit>* lineBoxAscenders,
            std::unordered_map<Frame*, FrameBlockBox*>* firstLineCandidates,
            std::vector<FrameBlockBox*>* blockBoxAligningFirstLineStack,
            std::unordered_map<FrameBlockBox*, std::pair<LineBox*, LayoutUnit>>*
                firstLineAscenders,
            std::unordered_map<FrameTableCellBox*,
                               std::pair<LineBox*, LayoutUnit>>* tempAscenders,
            std::unordered_map<PreferredWidthKey, PreferredWidthValue>*
                preferredWidthValues)
            : m_isRoot(isRoot)
            , m_isNormalFlow(isNormalFlow)
            , m_inlineBlockBoxStack(inlineBlockBoxStack)
            , m_floatBoxes(floatBoxes)
            , m_lineBoxAscenders(lineBoxAscenders)
            , m_firstLineCandidates(firstLineCandidates)
            , m_blockBoxAligningAtFirstBaselineStack(
                  blockBoxAligningFirstLineStack)
            , m_firstLineAscenders(firstLineAscenders)
            , m_tempAscenders(tempAscenders)
            , m_preferredWidthValues(preferredWidthValues)
        {
        }
        bool m_isRoot;
        bool m_isNormalFlow;
        LayoutUnit m_maxPositiveMarginTop;
        LayoutUnit m_maxNegativeMarginTop;
        LayoutUnit m_maxPositiveMarginBottom;
        LayoutUnit m_maxNegativeMarginBottom;
        LayoutUnit m_topLocOfFloatBox;
        std::vector<FrameBlockBox*>* m_inlineBlockBoxStack;
        std::vector<FloatingBoxInfo>* m_floatBoxes;
        std::unordered_map<FrameBlockBox*, LayoutUnit>* m_lineBoxAscenders;
        std::unordered_map<Frame*, FrameBlockBox*>* m_firstLineCandidates;
        std::vector<FrameBlockBox*>* m_blockBoxAligningAtFirstBaselineStack;
        std::unordered_map<FrameBlockBox*, std::pair<LineBox*, LayoutUnit>>*
            m_firstLineAscenders;
        std::unordered_map<FrameTableCellBox*, std::pair<LineBox*, LayoutUnit>>*
            m_tempAscenders;
        std::unordered_map<PreferredWidthKey, PreferredWidthValue>*
            m_preferredWidthValues;
    };

    StarFish* m_starFish;
    FrameDocument* m_frameDocument;

    // NOTE. we don't need gc_allocator here. because, FrameTree already has
    // a reference for Frames
    std::vector<BlockFormattingContext> m_blockFormattingContextInfo;
    std::map<FrameBlockBox*, std::vector<FrameBox*>> m_absolutePositionedBoxes;
    std::map<FrameBlockBox*, std::vector<std::pair<FrameBox*, bool>>>
        m_relativePositionedBoxes;
    // TODO move these maps into BlockFormattingContext
    std::unordered_map<FrameBox*, MarginCollapseResult> m_marginCollapseResult;
    std::unordered_map<FrameBox*, MarginInfo*> m_marginInfo;

    void applyRelativePosition(FrameBox* box);
    void applyRelativePositionInlineCase(Frame* refF, FrameBox* box);
};

class FloatingBoxInfo {
public:
    FloatingBoxInfo(FrameBox* box, LayoutContext* ctx);
    void reCache(LayoutContext* ctx);

    FrameBox* box()
    {
        return m_box;
    }

    bool canLayoutParentCollapseWithMarginTop()
    {
        return m_canLayoutParentCollapseWithMarginTop;
    }

    bool isLeft()
    {
        return m_isLeft;
    }

    LayoutLocation loc()
    {
        return m_loc;
    }

    LayoutUnit top()
    {
        return m_top;
    }

    LayoutUnit bottom()
    {
        return m_bottom;
    }

    LayoutUnit horizontalBoundary()
    {
        return m_horizontalBoundary;
    }

private:
    FrameBox* m_box;
    bool m_isLeft;
    bool m_canLayoutParentCollapseWithMarginTop;
    LayoutLocation m_loc;
    LayoutUnit m_top;
    LayoutUnit m_bottom;
    LayoutUnit m_horizontalBoundary;
};

enum HasFloat {
    HasNone,
    HasLeft,
    HasRight,
};

enum WordType {
    CollapsibleWhiteSpace,
    NonCollapsibleWhiteSpace,
    ForcedNewline,
    General,
};

class PreferredWidthContext {
public:
    PreferredWidthContext(LayoutContext& lc, Frame* frame,
                          LayoutUnit lastKnownWidth)
        : m_layoutContext(lc)
        , m_frame(frame)
        , m_preferredWidthSoFar(0)
        , m_preferredMinWidthSoFar(0)
        , m_currentLineWidth(0)
        , m_textIndentWidth(0)
        , m_unprocessedStartingMBPWidth(0)
        , m_lastWhiteSpaceWidth(0)
        , m_wordWidth(0)
        , m_remainingWidth(std::max(lastKnownWidth, LayoutUnit(0)))
        , m_hasFloat(HasNone)
        , m_isWhiteSpaceAtLast(true)
        , m_isPendingWrapLine(false)
        , m_hasAppliedTextIndent(false)
    {
    }

    void computePreferredWidth();

    LayoutContext& layoutContext()
    {
        return m_layoutContext;
    }

    void updatePreferredWidth(LayoutUnit r)
    {
        m_preferredWidthSoFar = std::max(m_preferredWidthSoFar, r);
    }

    LayoutUnit preferredWidth() const
    {
        return std::max(m_preferredWidthSoFar, m_preferredMinWidthSoFar);
    }

    void updatePreferredMinWidth(LayoutUnit w)
    {
        m_preferredMinWidthSoFar = std::max(m_preferredMinWidthSoFar, w);
    }

    LayoutUnit currentLineWidth() const
    {
        return m_currentLineWidth;
    }

    void setCurrentLineWidth(LayoutUnit w)
    {
        m_currentLineWidth = w;
    }

    LayoutUnit widthAppliedByTextIndent(LayoutUnit w)
    {
        if (m_textIndentWidth != 0) {
            m_hasAppliedTextIndent |= true;
        }

        if (m_textIndentWidth >= 0 || -m_textIndentWidth < w) {
            w += m_textIndentWidth;
            m_textIndentWidth = 0;
        } else {
            m_textIndentWidth += w;
            w = 0;
        }

        return w;
    }

    void setTextIndentWidth(LayoutUnit w)
    {
        m_textIndentWidth = w;
    }

    LayoutUnit remainingWidth() const
    {
        return m_remainingWidth;
    }

    LayoutUnit preferredMinWidth() const
    {
        return m_preferredMinWidthSoFar;
    }

    LayoutUnit unprocessedStartingMBPWidth() const
    {
        return m_unprocessedStartingMBPWidth;
    }

    LayoutUnit mbpWidth(ComputedStyle* style);
    LayoutUnit leftMBPWidth(ComputedStyle* style);
    LayoutUnit rightMBPWidth(ComputedStyle* style);
    LayoutUnit startingMBPWidth(ComputedStyle* style);
    LayoutUnit endingMBPWidth(ComputedStyle* style);
    LayoutUnit preferredWidthWithNewContext(Frame* f);

    int hasFloat() const
    {
        return m_hasFloat;
    }

    void setIsWhiteSpaceAtLast(bool isWhiteSpaceAtLast, LayoutUnit width)
    {
        m_isWhiteSpaceAtLast = isWhiteSpaceAtLast;
        if (isWhiteSpaceAtLast) {
            m_lastWhiteSpaceWidth = width;
        } else {
            m_lastWhiteSpaceWidth = 0;
        }
    }

    bool isWhiteSpaceAtLast() const
    {
        return m_isWhiteSpaceAtLast;
    }

    bool isPendingWrapLine() const
    {
        return m_isPendingWrapLine;
    }

    bool isFirstLineBox() const
    {
        return false;
    }

    void breakLine(bool wrapped, bool dueToBr)
    {
        finishLine(wrapped);
        setIsWhiteSpaceAtLast(true, 0);
        if (m_hasAppliedTextIndent || dueToBr) {
            m_textIndentWidth = 0;
        }
        m_currentLineWidth = 0;
        m_isPendingWrapLine = false;
    }

    void finishLine(bool wrapped)
    {
        if (wrapped) {
            removeDanglingSpace();
            updatePreferredWidth(
                std::max(m_remainingWidth, m_currentLineWidth));
        } else {
            updateCurrentLineWidthByWordWidth();
            removeDanglingSpace();
            updatePreferredWidth(m_currentLineWidth);
        }
    }

    void handleTextToken(TextToken& token);

    void handleFloatingBox(Frame* f, LayoutUnit w);

    void updateCurrentLineWidthByWordWidth()
    {
        if (m_wordWidth == 0) {
            return;
        }

        if (dontBreakLine(m_wordWidth)) {
            m_currentLineWidth += m_wordWidth;
        } else {
            breakLine(true, false);
            m_currentLineWidth = m_wordWidth;
        }
        m_wordWidth = 0;
    }

    void updateCurrentLineWidth(Frame* f, LayoutUnit w,
                                WordType type = General);

    void updateUnprocessedStartingMBPWidth(Frame* f);

    void computePreferredWidthInline(Frame* prent);

private:
    LayoutContext& m_layoutContext;
    Frame* m_frame;
    LayoutUnit m_preferredWidthSoFar;
    LayoutUnit m_preferredMinWidthSoFar;
    LayoutUnit m_currentLineWidth;
    LayoutUnit m_textIndentWidth;
    LayoutUnit m_unprocessedStartingMBPWidth;
    LayoutUnit m_lastWhiteSpaceWidth;
    LayoutUnit m_wordWidth;
    LayoutUnit m_remainingWidth;
    int m_hasFloat;
    bool m_isWhiteSpaceAtLast;
    bool m_isPendingWrapLine;
    bool m_hasAppliedTextIndent;

    bool canInsertToLineBox(LayoutUnit width);
    bool hasFloatingBoxAlreadyInLineBox() const
    {
        return m_hasFloat != HasNone;
    }

    bool canInsertFloatingBox(Frame* f);
    bool dontBreakLine(LayoutUnit width);

    void removeDanglingSpace()
    {
        m_currentLineWidth -= m_lastWhiteSpaceWidth;
    }
};

class PaintingContext {
public:
    PaintingContext(Canvas* canvas)
        : m_canvas(canvas)
        , m_paintingStage(PaintingNormalFlowBlock)
        , m_paintingInlineStage(PaintingInlineBox)
    {
    }

    Canvas* m_canvas;
    PaintingStage m_paintingStage;
    PaintingInlineStage m_paintingInlineStage;
};

class FrameTreeItemModel {
public:
    FrameTreeItemModel()
    {
        m_firstChild = m_lastChild = m_next = m_previous = m_parent = nullptr;
    }

    Frame* m_parent;

    Frame* m_previous;
    Frame* m_next;

    Frame* m_firstChild;
    Frame* m_lastChild;
};

class Frame : public gc {
    friend class LayoutContext;

public:
    Frame(Node* node, ComputedStyle* s);

    bool shouldApplyOverflow();

    virtual void computeStyleFlags();

    virtual ~Frame()
    {
    }

    virtual bool isFrameBox() const
    {
        return false;
    }

    virtual bool isLineBox()
    {
        return false;
    }

    virtual bool isFrameBlockBox()
    {
        return false;
    }

    virtual bool isFrameFlexibleBox()
    {
        return false;
    }

    virtual bool isFrameDocument()
    {
        return false;
    }

    virtual bool isFrameText()
    {
        return false;
    }

    virtual bool isFrameInline()
    {
        return false;
    }

    virtual bool isFrameLineBreak()
    {
        return false;
    }

    virtual bool isFrameReplaced()
    {
        return false;
    }

    virtual bool isInlineBox() const
    {
        return false;
    }

    virtual bool isInlineTextBox() const
    {
        return false;
    }

    virtual bool isInlineNonReplacedBox() const
    {
        return false;
    }

    virtual bool isInlineBoxLayoutParentBox() const
    {
        return false;
    }

    virtual bool isFrameTableObjectBox()
    {
        return false;
    }

    virtual bool isFrameTableBox()
    {
        return false;
    }

    virtual bool isFrameTableCaptionBox()
    {
        return false;
    }

    virtual bool isFrameTableColBox()
    {
        return false;
    }

    virtual bool isFrameTableSectionBox()
    {
        return false;
    }

    virtual bool isFrameTableRowBox()
    {
        return false;
    }

    virtual bool isFrameTableCellBox()
    {
        return false;
    }

    virtual bool isFrameInputBox()
    {
        return false;
    }

    virtual bool isFrameSVGBox()
    {
        return false;
    }

    virtual bool isFrameSelectBox()
    {
        return false;
    }

    virtual bool isFrameOptGroupBox()
    {
        return false;
    }

    virtual bool isFrameOptionBox()
    {
        return false;
    }

    FrameText* asFrameText()
    {
        STARFISH_ASSERT(isFrameText());
        return (FrameText*)this;
    }

    FrameBox* asFrameBox()
    {
        STARFISH_ASSERT(isFrameBox());
        return (FrameBox*)this;
    }

    FrameReplaced* asFrameReplaced()
    {
        STARFISH_ASSERT(isFrameReplaced());
        return (FrameReplaced*)this;
    }

    FrameBlockBox* asFrameBlockBox()
    {
        STARFISH_ASSERT(isFrameBlockBox());
        return (FrameBlockBox*)this;
    }

    FrameDocument* asFrameDocument()
    {
        STARFISH_ASSERT(isFrameDocument());
        return (FrameDocument*)this;
    }

    FrameFlexibleBox* asFrameFlexibleBox()
    {
        STARFISH_ASSERT(isFrameFlexibleBox());
        return (FrameFlexibleBox*)this;
    }

    FrameInline* asFrameInline()
    {
        STARFISH_ASSERT(isFrameInline());
        return (FrameInline*)this;
    }

    FrameLineBreak* asFrameLineBreak()
    {
        STARFISH_ASSERT(isFrameLineBreak());
        return (FrameLineBreak*)this;
    }

    LineBox* asLineBox()
    {
        STARFISH_ASSERT(isLineBox());
        return (LineBox*)this;
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

    InlineBoxLayoutParentBox* asInlineBoxLayoutParentBox()
    {
        STARFISH_ASSERT(isInlineBoxLayoutParentBox());
        return (InlineBoxLayoutParentBox*)this;
    }
    FrameTableObjectBox* asFrameTableObjectBox()
    {
        STARFISH_ASSERT(isFrameTableObjectBox());
        return (FrameTableObjectBox*)this;
    }

    FrameTableBox* asFrameTableBox()
    {
        STARFISH_ASSERT(isFrameTableBox());
        return (FrameTableBox*)this;
    }

    FrameTableCaptionBox* asFrameTableCaptionBox()
    {
        STARFISH_ASSERT(isFrameTableCaptionBox());
        return (FrameTableCaptionBox*)this;
    }

    FrameTableSectionBox* asFrameTableSectionBox()
    {
        STARFISH_ASSERT(isFrameTableSectionBox());
        return (FrameTableSectionBox*)this;
    }

    FrameTableRowBox* asFrameTableRowBox()
    {
        STARFISH_ASSERT(isFrameTableRowBox());
        return (FrameTableRowBox*)this;
    }

    FrameTableCellBox* asFrameTableCellBox()
    {
        STARFISH_ASSERT(isFrameTableCellBox());
        return (FrameTableCellBox*)this;
    }

    FrameTableColBox* asFrameTableColBox()
    {
        STARFISH_ASSERT(isFrameTableColBox());
        return (FrameTableColBox*)this;
    }

    FrameInputBox* asFrameInputBox()
    {
        STARFISH_ASSERT(isFrameInputBox());
        return (FrameInputBox*)this;
    }

    FrameSVGBox* asFrameSVGBox()
    {
        STARFISH_ASSERT(isFrameSVGBox());
        return (FrameSVGBox*)this;
    }

    bool isDirectDescendantOfTableCellBox()
    {
        for (Frame* p = this; p; p = p->parent()) {
            if (p->isFrameTableCellBox()) {
                return true;
            } else if (p->isFrameBlockBox()) {
                return false;
            }
        }
        return false;
    }

    virtual ComputedStyle* style();

    Frame* enclosingFirstLineStyle();
    ComputedStyle* pseudoStyleForFirstLine(
        StyleResolver::PseudoElementType pseudoId, ComputedStyle* parentStyle);
    ComputedStyle* cachedPseudoStyle(StyleResolver::PseudoElementType pseudo,
                                     ComputedStyle* parentStyle);
    ComputedStyle* firstLineStyle(Frame* frame, ComputedStyle* frameStyle);
    OverflowValue appliedOverflowX();
    OverflowValue appliedOverflowY();

    void updateComputedStyle(Node* refNode);

    Node* node()
    {
        if (isAnonymous()) {
            return nullptr;
        } else {
            return m_node;
        }
    }

    virtual void setParent(Frame* f)
    {
        frameTreeItemModel()->m_parent = f;
    }

    Frame* parent() const
    {
        return frameTreeItemModel()->m_parent;
    }

    virtual Frame* layoutParent() const
    {
        return parent();
    }

    Frame* next() const
    {
        return frameTreeItemModel()->m_next;
    }

    Frame* previous() const
    {
        return frameTreeItemModel()->m_previous;
    }

    Frame* firstChild() const
    {
        return frameTreeItemModel()->m_firstChild;
    }

    Frame* lastChild() const
    {
        return frameTreeItemModel()->m_lastChild;
    }

    void appendChild(Frame* newChild)
    {
        STARFISH_ASSERT(newChild->parent() == nullptr);

        newChild->setParent(this);
        Frame* lChild = lastChild();

        if (lChild) {
            newChild->frameTreeItemModel()->m_previous = lChild;
            lChild->frameTreeItemModel()->m_next = newChild;
        } else {
            frameTreeItemModel()->m_firstChild = newChild;
        }

        frameTreeItemModel()->m_lastChild = newChild;
    }

    void insertBefore(Frame* nextChild, Frame* newChild)
    {
        STARFISH_ASSERT(!newChild->parent());

        if (!nextChild) {
            appendChild(newChild);
            return;
        }

        Frame* prev = nextChild->frameTreeItemModel()->m_previous;
        if (prev) {
            prev->frameTreeItemModel()->m_next = newChild;
        } else {
            frameTreeItemModel()->m_firstChild = newChild;
        }
        newChild->setParent(this);
        newChild->frameTreeItemModel()->m_previous = prev;
        newChild->frameTreeItemModel()->m_next = nextChild;
        nextChild->frameTreeItemModel()->m_previous = newChild;
    }

    void removeChild(Frame* oldChild)
    {
        STARFISH_ASSERT(oldChild);
        STARFISH_ASSERT(oldChild->parent() == this);

        if (oldChild->frameTreeItemModel()->m_previous) {
            oldChild->frameTreeItemModel()
                ->m_previous->frameTreeItemModel()
                ->m_next = oldChild->next();
        }
        if (oldChild->frameTreeItemModel()->m_next) {
            oldChild->frameTreeItemModel()
                ->m_next->frameTreeItemModel()
                ->m_previous = oldChild->previous();
        }

        if (frameTreeItemModel()->m_firstChild == oldChild) {
            frameTreeItemModel()->m_firstChild = oldChild->next();
        }
        if (frameTreeItemModel()->m_lastChild == oldChild) {
            frameTreeItemModel()->m_lastChild = oldChild->previous();
        }

        oldChild->frameTreeItemModel()->m_previous = nullptr;
        oldChild->frameTreeItemModel()->m_next = nullptr;
        oldChild->setParent(nullptr);
    }
#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth)
    {
        printf("%s [%p]", name(), this);
    }
#endif

    virtual const char* name()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    enum LayoutWantToResolve {
        ResolveWidth = 1,
        ResolveHeight = 1 << 1,
        ResolveAll = ResolveWidth | ResolveHeight,
    };
    virtual void layout(LayoutContext& ctx, LayoutWantToResolve resolveWhat)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual bool isSelfCollapsingBlock(LayoutContext& ctx)
    {
        return false;
    }

    virtual void computePreferredWidth(PreferredWidthContext& ctx)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void layoutInline(LineFormattingContext& ctx)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual InlineNonReplacedBox* firstInlineNonReplacedBox(FrameInline* f)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void establishesStackingContextIfNeeds()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void computeVisibleRect(StackingContext* sCtx, LayoutLocation& loc,
                                    LayoutRect& result)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void paint(PaintingContext& ctx)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    bool isAncestorOf(Frame* f)
    {
        while (f) {
            if (f == this) {
                return true;
            }
            f = f->layoutParent();
        }
        return false;
    }

    bool isEstablishesBlockFormattingContext() const
    {
        return m_flags.m_isEstablishesBlockFormattingContext;
    }

    bool isEstablishesStackingContext() const
    {
        return m_flags.m_isEstablishesStackingContext;
    }

    bool isPositioned() const
    {
        return m_flags.m_isPositioned;
    }

    bool isNormalFlow() const
    {
        return m_flags.m_isNormalFlow;
    }

    bool isRootElement() const
    {
        return m_flags.m_isRootElement;
    }

    bool isLeftMBPCleared() const
    {
        return m_flags.m_isLeftMBPCleared;
    }

    bool isRightMBPCleared() const
    {
        return m_flags.m_isRightMBPCleared;
    }

    void setLeftMBPCleared()
    {
        m_flags.m_isLeftMBPCleared = true;
    }

    void setRightMBPCleared()
    {
        m_flags.m_isRightMBPCleared = true;
    }

    bool needsGraphicsBuffer() const
    {
        return m_flags.m_needsGraphicsBuffer;
    }

    bool isAbsolutePositioned() const
    {
        return m_flags.m_isAbsolutePositioned;
    }

    bool isFloating() const
    {
        return m_flags.m_isFloating;
    }

    void markFlexItem();

    bool isFlexItem() const
    {
        return m_flags.m_isFlexItem;
    }

    bool isDocumentElement() const;

    bool isAnonymous() const
    {
        return m_flags.m_isAnonymous;
    }

    bool isBlockLevel()
    {
        // block, table, list-item
        return (style()->display() == DisplayValue::BlockDisplayValue) ||
               (style()->display() == DisplayValue::TableDisplayValue) ||
               (style()->display() == DisplayValue::FlexDisplayValue);
    }

    bool isInlineLevel()
    {
        return (style()->display() == DisplayValue::InlineDisplayValue) ||
               (style()->display() == DisplayValue::InlineBlockDisplayValue) ||
               (style()->display() == DisplayValue::InlineTableDisplayValue) ||
               (style()->display() == DisplayValue::InlineFlexDisplayValue);
    }

    bool isAtomicInlineLevel()
    {
        return (isFrameReplaced()) ||
               (style()->display() == DisplayValue::InlineBlockDisplayValue) ||
               (style()->display() == DisplayValue::InlineTableDisplayValue);
    }

    bool canBeContainingBlockOfAbsolutePositionedBox(Frame* child)
    {
        STARFISH_ASSERT(child->isAbsolutePositioned());
        return isFrameDocument() ||
               (child->style()->position() != FixedPositionValue &&
                isPositioned()) ||
               style()->hasTransforms(this);
    }

    bool canHaveFirstLineOrFirstLetterStyle()
    {
        if (isFrameBlockBox()) {
            if (isFrameTableBox() || isFrameTableRowBox() ||
                isFrameTableSectionBox() || isFrameTableColBox() ||
                isFrameFlexibleBox()) {
                return false;
            }
            return true;
        }
        return false;
    }

    bool shouldResetTextDecoration()
    {
        return !isNormalFlow() ||
               (style()->display() == DisplayValue::InlineBlockDisplayValue) ||
               (style()->display() == DisplayValue::InlineTableDisplayValue);
    }

    Element* offsetParent() const;
    LayoutUnit offsetLeft()
    {
        return adjustedPositionRelativeToOffsetParent().x();
    }

    LayoutUnit offsetTop()
    {
        return adjustedPositionRelativeToOffsetParent().y();
    }

    bool shouldWrapLines()
    {
        // When true, break lines as necessary to fill line boxes
        return style()->whiteSpace() & WhiteSpaceValue::PreLineWhiteSpaceValue;
    }

    bool shouldPreserveWhiteSpaces()
    {
        // When true, preserve sequences of white space (Anti-collapsing)
        return style()->whiteSpace() & WhiteSpaceValue::PreWhiteSpaceValue;
    }

    bool shouldIgnoreNewlineChar()
    {
        // When true, newline characters are handled as other whitespace
        return style()->whiteSpace() & WhiteSpaceValue::NoWrapWhiteSpaceValue;
    }

    virtual ComputedStyle* style(Frame* parent, ComputedStyle* parentStyle,
                                 bool isFirstLine)
    {
        if (isFirstLine) {
            if (ComputedStyle* style = firstLineStyle(parent, parentStyle)) {
                return style;
            }
        }
        return Frame::style();
    }

    Document* document();
    LayoutLocation adjustedPositionRelativeToOffsetParent();

    FrameBox* findNearestAssociateBox();

protected:
    virtual bool hasFrameTreeItemModel()
    {
        return false;
    }

    virtual FrameTreeItemModel* frameTreeItemModel()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    const FrameTreeItemModel* frameTreeItemModel() const
    {
        return const_cast<Frame*>(this)->frameTreeItemModel();
    }

    struct FrameFlags {
        bool m_needsLayout : 1;
        bool m_isAnonymous : 1;

        // https://www.w3.org/TR/CSS21/visuren.html#block-formatting
        // Floats, absolutely positioned elements, block containers (such as
        // inline-blocks, table-cells, and table-captions) that are not block
        // boxes, and block boxes with 'overflow' other than 'visible' (except
        // when that value has been propagated to the viewport) establish new
        // block formatting contexts for their contents.
        bool m_isEstablishesBlockFormattingContext : 1;
        bool m_needsGraphicsBuffer : 1;

        // https://www.w3.org/TR/CSS21/visuren.html#propdef-z-index
        // Other stacking contexts are generated by any positioned element
        // (including relatively positioned elements) having a computed value of
        // 'z-index' other than 'auto'. Stacking contexts are not necessarily
        // related to containing blocks. In future levels of CSS, other
        // properties may introduce stacking contexts, for example 'opacity'
        // [CSS3COLOR].
        bool m_isEstablishesStackingContext : 1;

        // https://www.w3.org/TR/CSS21/visuren.html#positioning-scheme
        // 9.3.2
        // An element is said to be positioned if its 'position' property has a
        // value other than 'static'. Positioned elements generate positioned
        // boxes, laid out according to four properties:
        bool m_isPositioned : 1;

        bool m_isNormalFlow : 1;
        bool m_isRootElement : 1;

        bool m_isLeftMBPCleared : 1;
        bool m_isRightMBPCleared : 1;

        bool m_isAbsolutePositioned : 1;
        bool m_isFloating : 1;
        bool m_isFlexItem : 1;

        // special flag for FrameBlockBox
        bool m_heightComputed : 1;
        bool m_hasBiggerContentThanFrameWidth : 1;
        bool m_hasBiggerContentThanFrameHeight : 1;
        // special flag for InlineBox
        bool m_isFirstLine : 1;
        // special flag for InlineTextBox
        CharDirection m_direction : 2;
        bool m_gotLongString : 1;
        // special flag for InlineNonReplacedBox
        bool m_isCollapsed : 1;
    } m_flags;

    STARFISH_COMPILE_ASSERT(sizeof(FrameFlags) <= sizeof(uint32_t),
                            "keep FrameFlags small");
    union {
        Node* m_node;
        ComputedStyle* m_styleWhenNodeIsAnonymous;
    };
};
}

#endif
