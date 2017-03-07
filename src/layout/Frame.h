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

#ifndef __StarFishFrame__
#define __StarFishFrame__

#include "dom/DOM.h"

#include "style/Unit.h"
#include "style/ComputedStyle.h"

#include "layout/LayoutUtil.h"
#include "layout/StackingContext.h"

namespace StarFish {

class Node;
class FrameText;
class FrameBox;
class FrameBlockBox;
class FrameReplaced;
class FrameInline;
class FrameLineBreak;
class FrameDocument;
class FrameTableBox;
class FrameTableCaptionBox;
class FrameTableSectionBox;
class FrameTableRowBox;
class FrameTableCellBox;
class FrameTableColBox;
class LineBox;

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
    PaintingInlineLevelElements,
    PaintingInlineBlock,
    PaintingInlineStageEnd
};

class LineBox;
class MarginInfo;
class FloatingBoxInfo;
class TextToken;

class LayoutContext {
public:
    LayoutContext(StarFish* starFish, FrameDocument* frameDocument)
        : m_starFish(starFish), m_frameDocument(frameDocument)
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
            m_blockFormattingContextInfo.emplace_back(isNormalFlow, isRoot, s,
                                                      s2, s3);
        } else {
            BlockFormattingContext& back = m_blockFormattingContextInfo.back();
            std::vector<FloatingBoxInfo>* s =
                new std::vector<FloatingBoxInfo>();
            m_blockFormattingContextInfo.emplace_back(
                isNormalFlow, isRoot, back.m_inlineBlockBoxStack, s,
                back.m_registeredYPositionPerVAInlineBlock);
        }
    }

    void removeBlockFormattingContext()
    {
        if (m_blockFormattingContextInfo.back().m_isRoot ||
            !m_blockFormattingContextInfo.back().m_isNormalFlow) {
            delete m_blockFormattingContextInfo.back().m_inlineBlockBoxStack;
            delete m_blockFormattingContextInfo.back()
                .m_registeredYPositionPerVAInlineBlock;
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
    Frame* blockContainer(Frame* currentFrame);
    Frame* containingFrameBlockBox(
        Frame* currentFrame); // this function returns most near blockContainer
    Frame* containingBlock(
        Frame* currentFrame); // this function returns real containing block

    void pushInlineBlockBox(FrameBlockBox* ib)
    {
        m_blockFormattingContextInfo.back().m_inlineBlockBoxStack->push_back(
            ib);
    }

    void popInlineBlockBox()
    {
        m_blockFormattingContextInfo.back().m_inlineBlockBoxStack->pop_back();
    }

    void registerYPositionPerVAInlineBlock(LineBox* lb);
    std::pair<bool, LayoutUnit> registeredLastLineBoxYPosition(
        FrameBlockBox* box);
    void registerAbsolutePositionedBox(Frame* frm);

    template <typename Fn>
    void layoutRegisteredAbsolutePositionedBoxes(Frame* containgBlock, Fn f)
    {
        auto iter = m_absolutePositionedBoxes.find(containgBlock);
        if (iter == m_absolutePositionedBoxes.end()) {
            return;
        } else {
            f(iter->second);
            m_absolutePositionedBoxes.erase(iter);
        }
    }

    void registerRelativePositionedBox(Frame* frm, bool dueToSelf);

    template <typename Fn>
    void layoutRegisteredRelativePositionedBoxes(Frame* containingBox, Fn f)
    {
        auto iter = m_relativePositionedBoxes.find(containingBox);
        if (iter == m_relativePositionedBoxes.end()) {
            return;
        } else {
            f(iter->second);
            m_relativePositionedBoxes.erase(iter);
        }
    }

    void propagatePositionedBoxes(LayoutContext& to)
    {
        {
            auto iter = m_absolutePositionedBoxes.begin();

            while (iter != m_absolutePositionedBoxes.end()) {
                auto iter2 = to.m_absolutePositionedBoxes.find(iter->first);
                if (iter2 == to.m_absolutePositionedBoxes.end()) {
                    to.m_absolutePositionedBoxes.insert(*iter);
                } else {
                    iter2->second.insert(iter2->second.end(),
                                         iter->second.begin(),
                                         iter->second.end());
                }
                iter++;
            }

            m_absolutePositionedBoxes.clear();
        }
        {
            auto iter = m_relativePositionedBoxes.begin();

            while (iter != m_relativePositionedBoxes.end()) {
                auto iter2 = to.m_relativePositionedBoxes.find(iter->first);
                if (iter2 == to.m_relativePositionedBoxes.end()) {
                    to.m_relativePositionedBoxes.insert(*iter);
                } else {
                    iter2->second.insert(iter2->second.end(),
                                         iter->second.begin(),
                                         iter->second.end());
                }
                iter++;
            }

            m_relativePositionedBoxes.clear();
        }
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

private:
    struct BlockFormattingContext {
        BlockFormattingContext(bool isNormalFlow, bool isRoot,
                               std::vector<FrameBlockBox*>* inlineBlockBoxStack,
                               std::vector<FloatingBoxInfo>* floatBoxes,
                               std::unordered_map<FrameBlockBox*, LayoutUnit>*
                                   registeredYPositionPerVAInlineBlock)
        {
            m_isRoot = isRoot;
            m_isNormalFlow = isNormalFlow;
            m_inlineBlockBoxStack = inlineBlockBoxStack;
            m_floatBoxes = floatBoxes;
            m_registeredYPositionPerVAInlineBlock =
                registeredYPositionPerVAInlineBlock;
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
        std::unordered_map<FrameBlockBox*, LayoutUnit>*
            m_registeredYPositionPerVAInlineBlock;
    };

    StarFish* m_starFish;
    FrameDocument* m_frameDocument;

    // NOTE. we dont need gc_allocator here. because, FrameTree already has
    // referenece for Frames
    std::vector<BlockFormattingContext> m_blockFormattingContextInfo;
    std::map<Frame*, std::vector<FrameBox*>> m_absolutePositionedBoxes;
    std::map<Frame*, std::vector<std::pair<FrameBox*, bool>>>
        m_relativePositionedBoxes;
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
    PreferredWidthContext(LayoutContext& lc, LayoutUnit lastKnownWidth)
        : m_layoutContext(lc)
        , m_preferredWidthSoFar(0)
        , m_preferredMinWidthSoFar(0)
        , m_currentLineWidth(0)
        , m_unprocessedStartingMBPWidth(0)
        , m_lastWhiteSpaceWidth(0)
        , m_remainedWidth(lastKnownWidth)
        , m_hasFloat(HasNone)
        , m_isWhiteSpaceAtLast(true)
        , m_isPendingBreakLine(false)
    {
    }

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

    LayoutUnit remainedWidth() const
    {
        return m_remainedWidth;
    }

    void setRemainedWidth(LayoutUnit w)
    {
        m_remainedWidth = w;
    }

    LayoutUnit preferredMinWidth() const
    {
        return m_preferredMinWidthSoFar;
    }

    LayoutUnit unprocessedStartingMBPWidth() const
    {
        return m_unprocessedStartingMBPWidth;
    }

    static LayoutUnit computeMinimumWidthDueToMBP(ComputedStyle* style)
    {
        LayoutUnit minWidth;
        if (style->borderLeftWidth().isFixed()) {
            minWidth += style->borderLeftWidth().fixed();
        }
        if (style->borderRightWidth().isFixed()) {
            minWidth += style->borderRightWidth().fixed();
        }
        if (style->paddingLeft().isFixed()) {
            minWidth += style->paddingLeft().fixed();
        }
        if (style->paddingRight().isFixed()) {
            minWidth += style->paddingRight().fixed();
        }
        if (style->marginLeft().isFixed()) {
            minWidth += style->marginLeft().fixed();
        }
        if (style->marginRight().isFixed()) {
            minWidth += style->marginRight().fixed();
        }
        return minWidth;
    }

    static LayoutUnit preferredWidthWidthNewContext(PreferredWidthContext& ctx,
                                                    Frame* f);

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

    bool isPendingBreakLine() const
    {
        return m_isPendingBreakLine;
    }

    void breakLine(bool wrapped)
    {
        finishLine(wrapped);
        setIsWhiteSpaceAtLast(true, 0);
        m_currentLineWidth = 0;
        m_isPendingBreakLine = false;
    }

    void finishLine(bool wrapped)
    {
        if (wrapped) {
            updatePreferredWidth(m_remainedWidth);
        } else {
            removeDanglingSpace();
            updatePreferredWidth(m_currentLineWidth);
        }
    }

    void handleTextToken(TextToken& token);

    void handleFloatingBox(Frame* f, LayoutUnit w);
    void updateCurrentLineWidth(Frame* f, LayoutUnit w,
                                WordType type = General);
    void updateUnprocessedStartingMBPWidth(Frame* f);

private:
    LayoutContext& m_layoutContext;
    LayoutUnit m_preferredWidthSoFar;
    LayoutUnit m_preferredMinWidthSoFar;
    LayoutUnit m_currentLineWidth;
    LayoutUnit m_unprocessedStartingMBPWidth;
    LayoutUnit m_lastWhiteSpaceWidth;
    LayoutUnit m_remainedWidth;
    int m_hasFloat;
    bool m_isWhiteSpaceAtLast;
    bool m_isPendingBreakLine;

    bool canInsertToLineBox(LayoutUnit width);
    bool hasFloatingBoxAlreadyInLineBox(Frame* f) const
    {
        return m_hasFloat != HasNone;
    }

    bool canInsertFloatingBox(Frame* f);
    bool dontBreakLine(Frame* f, LayoutUnit width);

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
        , m_paintingInlineStage(PaintingInlineLevelElements)
    {
    }

    Canvas* m_canvas;
    PaintingStage m_paintingStage;
    PaintingInlineStage m_paintingInlineStage;
};

class Frame : public gc {
    friend class LayoutContext;

public:
    Frame(Node* node, ComputedStyle* s)
        : m_node(node), m_styleWhenNodeIsAnonymous(s)
    {
        m_firstChild = m_lastChild = m_next = m_previous = m_parent = nullptr;
        m_flags.m_needsLayout = true;

        bool isRootElement =
            node && node->isElement() && node->asElement()->isHTMLElement() &&
            node->asElement()->asHTMLElement()->isHTMLHtmlElement();
        m_flags.m_isRootElement = isRootElement;

        m_flags.m_isLeftMBPCleared = false;
        m_flags.m_isRightMBPCleared = false;

        m_flags.m_isEstablishesBlockFormattingContext = isRootElement;
        m_flags.m_isPositioned = false;
        m_flags.m_isEstablishesStackingContext = isRootElement;
        m_flags.m_needsGraphicsBuffer = false;
        m_flags.m_isNormalFlow = true;
        m_flags.m_isFloating = false;

        computeStyleFlags();
    }

    bool isOverflowPropagatedToViewPort()
    {
        if (m_node && m_node->isElement() &&
            m_node->asElement()->isHTMLElement() &&
            m_node->asElement()->asHTMLElement()->isHTMLHtmlElement()) {
            HTMLBodyElement* bodyElement = m_node->document()->bodyElement();
            if (bodyElement) {
                return bodyElement->style()->overflow() != style()->overflow();
            }
            return style()->overflow() != OverflowValue::VisibleOverflow;
        }

        if (m_node && m_node->isElement() &&
            m_node->asElement()->isHTMLElement() &&
            m_node->asElement()->asHTMLElement()->isHTMLBodyElement() &&
            m_node->document()->rootElement()) {
            HTMLHtmlElement* rootElement = m_node->document()->rootElement();
            return rootElement->style()->overflow() != style()->overflow();
        }

        return false;
    }

    bool shouldApplyOverflow()
    {
        return isOverflowPropagatedToViewPort()
                   ? false
                   : style()->overflow() != OverflowValue::VisibleOverflow;
    }

    virtual void computeStyleFlags()
    {
        ComputedStyle* style = Frame::style();
        if (!style) {
            return;
        }

        // TODO add condition
        // https://www.w3.org/TR/CSS21/visuren.html#block-formatting
        // Block formatting context is established when the element is either
        // float, absolute positioned, or block boxes with 'overflow' other than
        // 'visible'.
        // Especially, the last condition should be met another requirement,
        // which is, the overflow should not be propagated to viewport.
        // There are 2 possible cases that overflow property can propagate to
        // viewport, in other words, containing block is viewport.
        // 1. By giving a position of absoulte value, which is already included
        // as one of forming block formatting context conditions.
        // 2. <html> and <body> element, so we should check first overflow
        // values of <head> and <body> are equal.
        m_flags.m_isEstablishesBlockFormattingContext |=
            (shouldApplyOverflow());
        m_flags.m_isEstablishesBlockFormattingContext |=
            (style->originalDisplay() == DisplayValue::InlineBlockDisplayValue);
        m_flags.m_isEstablishesBlockFormattingContext |=
            (style->position() == PositionValue::AbsolutePositionValue);
        m_flags.m_isEstablishesBlockFormattingContext |=
            (style->floating() != FloatValue::NoneFloatValue);

        m_flags.m_isPositioned =
            (style->position() != PositionValue::StaticPositionValue);

        // TODO add condition
        // NOTE
        // https://www.w3.org/TR/CSS2/zindex.html
        // Appendix E. Elaborate description of Stacking Contexts
        // All positioned descendants with 'z-index: auto' or 'z-index: 0', in
        // tree order. For those with 'z-index: auto', treat the element as if
        // it created a new stacking context,
        m_flags.m_isEstablishesStackingContext |= m_flags.m_isPositioned;
        m_flags.m_isEstablishesStackingContext |= (style->opacity() != 1);
        m_flags.m_isEstablishesStackingContext |= (style->hasTransforms(this));

        // TODO add condition
        m_flags.m_needsGraphicsBuffer |= (style->opacity() != 1);
        m_flags.m_needsGraphicsBuffer |= (style->hasTransforms(this));

        if ((style->position() == PositionValue::AbsolutePositionValue) ||
            (style->floating() != FloatValue::NoneFloatValue)) {
            m_flags.m_isNormalFlow = false;
        }

        m_flags.m_isFloating =
            (style->floating() != FloatValue::NoneFloatValue);
    }

    virtual ~Frame()
    {
    }

    virtual bool isFrameBox()
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

    FrameText* asFrameText()
    {
        STARFISH_ASSERT(isFrameText());
        return (FrameText*)this;
    }

    FrameBox* asFrameBox()
    {
        STARFISH_ASSERT(isFrameBox() || isFrameTableBox() ||
                        isFrameTableCaptionBox() || isFrameTableSectionBox() ||
                        isFrameTableRowBox() || isFrameTableCellBox());
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

    ComputedStyle* style()
    {
        if (UNLIKELY(isAnonymous())) {
            return m_styleWhenNodeIsAnonymous;
        } else {
            return node()->style();
        }
    }

    void updateComputedStyle(Node* refNode)
    {
        STARFISH_ASSERT(isAnonymous());
        STARFISH_ASSERT(m_styleWhenNodeIsAnonymous);
        ComputedStyle* newStyle = new ComputedStyle(refNode->style());
        newStyle->setDisplay(m_styleWhenNodeIsAnonymous->display());
        newStyle->loadResources(refNode, m_styleWhenNodeIsAnonymous);
        newStyle->arrangeStyleValues(refNode->style(), refNode);
        m_styleWhenNodeIsAnonymous = newStyle;
    }

    Node* node()
    {
        return m_node;
    }

    virtual LayoutUnit leftMBPWidth()
    {
        LayoutUnit w;
        if (style()->marginLeft().isFixed()) {
            w += style()->marginLeft().fixed();
        }
        if (style()->borderLeftWidth().isFixed()) {
            w += style()->borderLeftWidth().fixed();
        }
        if (style()->paddingLeft().isFixed()) {
            w += style()->paddingLeft().fixed();
        }
        return w;
    }

    virtual LayoutUnit rightMBPWidth()
    {
        LayoutUnit w;
        if (style()->marginRight().isFixed()) {
            w += style()->marginRight().fixed();
        }
        if (style()->borderRightWidth().isFixed()) {
            w += style()->borderRightWidth().fixed();
        }
        if (style()->paddingRight().isFixed()) {
            w += style()->paddingRight().fixed();
        }
        return w;
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

    void setParent(Frame* f)
    {
        m_layoutParent = m_parent = f;
    }

    void setLayoutParent(Frame* f)
    {
        m_layoutParent = f;
    }

    Frame* parent() const
    {
        return m_parent;
    }

    Frame* layoutParent() const
    {
        return m_layoutParent;
    }

    Frame* next() const
    {
        return m_next;
    }

    Frame* previous() const
    {
        return m_previous;
    }

    Frame* firstChild() const
    {
        return m_firstChild;
    }

    Frame* lastChild() const
    {
        return m_lastChild;
    }

    void appendChild(Frame* newChild)
    {
        STARFISH_ASSERT(newChild->parent() == nullptr);

        newChild->setParent(this);
        Frame* lChild = lastChild();

        if (lChild) {
            newChild->m_previous = lChild;
            lChild->m_next = newChild;
        } else {
            m_firstChild = newChild;
        }

        m_lastChild = newChild;
    }

    void insertBefore(Frame* nextChild, Frame* newChild)
    {
        STARFISH_ASSERT(!newChild->parent());

        if (!nextChild) {
            appendChild(newChild);
            return;
        }

        Frame* prev = nextChild->m_previous;
        if (prev) {
            prev->m_next = newChild;
        } else {
            m_firstChild = newChild;
        }
        newChild->setParent(this);
        newChild->m_previous = prev;
        newChild->m_next = nextChild;
        nextChild->m_previous = newChild;
    }

    void removeChild(Frame* oldChild)
    {
        STARFISH_ASSERT(oldChild);
        STARFISH_ASSERT(oldChild->parent() == this);

        if (oldChild->m_previous) {
            oldChild->m_previous->m_next = oldChild->next();
        }
        if (oldChild->m_next) {
            oldChild->m_next->m_previous = oldChild->previous();
        }

        if (m_firstChild == oldChild) {
            m_firstChild = oldChild->next();
        }
        if (m_lastChild == oldChild) {
            m_lastChild = oldChild->previous();
        }

        oldChild->m_previous = nullptr;
        oldChild->m_next = nullptr;
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

    bool isFloating() const
    {
        return m_flags.m_isFloating;
    }

    bool isDocumentElement() const
    {
        return m_node->document() == m_node;
    }

    bool isBodyElement() const
    {
        return m_node->isElement() && m_node->asElement()->isHTMLElement() &&
               m_node->asElement()->asHTMLElement()->isHTMLBodyElement();
    }

    bool isAnonymous() const
    {
        return m_node == nullptr;
    }

    bool isBlockLevel()
    {
        // block, table, list-item
        return (style()->display() == DisplayValue::BlockDisplayValue) ||
               (style()->display() == DisplayValue::TableDisplayValue);
    }

    Element* offsetParent();

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

protected:
    struct {
        bool m_needsLayout : 1;

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

        bool m_isFloating : 1;
    } m_flags;

private:
    Node* m_node;
    // TODO implement FrameRareData
    ComputedStyle* m_styleWhenNodeIsAnonymous;

    Frame* m_parent;
    Frame* m_layoutParent;

    Frame* m_previous;
    Frame* m_next;

    Frame* m_firstChild;
    Frame* m_lastChild;
};
}

#endif
