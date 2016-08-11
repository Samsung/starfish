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
class FrameDocument;
class LineBox;

enum PaintingStage {
    PaintingNormalFlowBlock, // the in-flow, non-inline-level, non-positioned descendants.
    PaintingNormalFlowInline, // the in-flow, inline-level, non-positioned descendants, including inline tables and inline blocks.
    PaintingPositionedElements, // the child stacking contexts with stack level 0 and the positioned descendants with stack level 0.
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
        STARFISH_ASSERT(m_absolutePositionedFrames.size() == 0);
        STARFISH_ASSERT(m_relativePositionedFrames.size() == 0);
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
            std::unordered_map<FrameBlockBox*, LayoutUnit>* s2 = new std::unordered_map<FrameBlockBox*, LayoutUnit>();
            m_blockFormattingContextInfo.push_back(BlockFormattingContext(isNormalFlow, isRoot, s, s2));
        } else {
            BlockFormattingContext& back = m_blockFormattingContextInfo.back();
            m_blockFormattingContextInfo.push_back(BlockFormattingContext(isNormalFlow, isRoot, back.m_inlineBlockBoxStack, back.m_registeredYPositionForVerticalAlignInlineBlock));
        }
    }

    void removeBlockFormattingContext()
    {
        if (m_blockFormattingContextInfo.back().m_isRoot || !m_blockFormattingContextInfo.back().m_isNormalFlow) {
            delete m_blockFormattingContextInfo.back().m_inlineBlockBoxStack;
            delete m_blockFormattingContextInfo.back().m_registeredYPositionForVerticalAlignInlineBlock;
        }
        m_blockFormattingContextInfo.pop_back();
    }

    LayoutUnit parentContentWidth(Frame* currentFrame);
    bool parentHasFixedHeight(Frame* currentFrame);
    LayoutUnit parentFixedHeight(Frame* currentFrame);
    Frame* blockContainer(Frame* currentFrame);
    Frame* containingFrameBlockBox(Frame* currentFrame); // this function returns most near blockContainer
    Frame* containingBlock(Frame* currentFrame); // this function returns real containing block

    void pushInlineBlockBox(FrameBlockBox* ib)
    {
        m_blockFormattingContextInfo.back().m_inlineBlockBoxStack->push_back(ib);
    }

    void popInlineBlockBox()
    {
        m_blockFormattingContextInfo.back().m_inlineBlockBoxStack->pop_back();
    }

    void registerYPositionForVerticalAlignInlineBlock(LineBox* lb);
    std::pair<bool, LayoutUnit> readRegisteredLastLineBoxYPos(FrameBlockBox* box);
    void registerAbsolutePositionedFrames(Frame* frm)
    {
        Frame* cb = containingFrameBlockBox(frm);
        m_absolutePositionedFrames.insert(std::make_pair(cb, std::vector<Frame*>()));
        std::vector<Frame*>& vec = m_absolutePositionedFrames[cb];
        STARFISH_ASSERT(std::find(vec.begin(), vec.end(), frm) == vec.end());
        vec.push_back(frm);
    }

    template <typename Fn>
    void layoutRegisteredAbsolutePositionedFrames(Frame* containgBlock, Fn f)
    {
        auto iter = m_absolutePositionedFrames.find(containgBlock);
        if (iter == m_absolutePositionedFrames.end()) {
            return;
        } else {
            f(iter->second);
            m_absolutePositionedFrames.erase(iter);
        }
    }

    void registerRelativePositionedFrames(Frame* frm, bool dueToSelf)
    {
        Frame* cb = containingFrameBlockBox(frm);
        m_relativePositionedFrames.insert(std::make_pair(cb, std::vector<std::pair<Frame*, bool> >()));
        std::vector<std::pair<Frame*, bool> >& vec = m_relativePositionedFrames[cb];
        vec.push_back(std::make_pair(frm, dueToSelf));
    }

    template <typename Fn>
    void layoutRegisteredRelativePositionedFrames(Frame* containgBlock, Fn f)
    {
        auto iter = m_relativePositionedFrames.find(containgBlock);
        if (iter == m_relativePositionedFrames.end()) {
            return;
        } else {
            f(iter->second);
            m_relativePositionedFrames.erase(iter);
        }
    }

    void propagatePositionedFrames(LayoutContext& to)
    {
        {
            auto iter = m_absolutePositionedFrames.begin();

            while (iter != m_absolutePositionedFrames.end()) {
                auto iter2 = to.m_absolutePositionedFrames.find(iter->first);
                if (iter2 == to.m_absolutePositionedFrames.end()) {
                    to.m_absolutePositionedFrames.insert(std::make_pair(iter->first, iter->second));
                } else {
                    iter2->second.insert(iter2->second.end(), iter->second.begin(), iter->second.end());
                }
                iter++;
            }

            m_absolutePositionedFrames.clear();
        }
        {
            auto iter = m_relativePositionedFrames.begin();

            while (iter != m_relativePositionedFrames.end()) {
                auto iter2 = to.m_relativePositionedFrames.find(iter->first);
                if (iter2 == to.m_relativePositionedFrames.end()) {
                    to.m_relativePositionedFrames.insert(std::make_pair(iter->first, iter->second));
                } else {
                    iter2->second.insert(iter2->second.end(), iter->second.begin(), iter->second.end());
                }
                iter++;
            }

            m_relativePositionedFrames.clear();
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

private:
    struct BlockFormattingContext {
        BlockFormattingContext(bool isNormalFlow, bool isRoot, std::vector<FrameBlockBox*>* inlineBlockBoxStack, std::unordered_map<FrameBlockBox*, LayoutUnit>* registeredYPositionForVerticalAlignInlineBlock)
        {
            m_isRoot = isRoot;
            m_isNormalFlow = isNormalFlow;
            m_inlineBlockBoxStack = inlineBlockBoxStack;
            m_registeredYPositionForVerticalAlignInlineBlock = registeredYPositionForVerticalAlignInlineBlock;
        }
        bool m_isRoot;
        bool m_isNormalFlow;
        LayoutUnit m_maxPositiveMarginTop;
        LayoutUnit m_maxNegativeMarginTop;
        LayoutUnit m_maxPositiveMarginBottom;
        LayoutUnit m_maxNegativeMarginBottom;
        std::vector<FrameBlockBox*>* m_inlineBlockBoxStack;
        std::unordered_map<FrameBlockBox*, LayoutUnit>* m_registeredYPositionForVerticalAlignInlineBlock;
    };

    StarFish* m_starFish;
    FrameDocument* m_frameDocument;

    // NOTE. we dont need gc_allocator here. because, FrameTree already has referenece for Frames
    std::vector<BlockFormattingContext> m_blockFormattingContextInfo;
    std::map<Frame*, std::vector<Frame*> > m_absolutePositionedFrames;
    std::map<Frame*, std::vector<std::pair<Frame*, bool> > > m_relativePositionedFrames;
};

class ComputePreferredWidthContext {
public:
    ComputePreferredWidthContext(LayoutContext& lc, LayoutUnit lastKnownWidth, LayoutUnit minimumWidth)
        : m_layoutContext(lc)
    {
        m_result = 0;
        m_lastKnownWidth = lastKnownWidth;
        m_minimumWidth = minimumWidth;
    }

    LayoutContext& layoutContext()
    {
        return m_layoutContext;
    }

    void setResult(LayoutUnit r)
    {
        m_result = std::max(m_result, r);
    }

    LayoutUnit result() const
    {
        return std::max(m_result, m_minimumWidth);
    }

    void setMinimumWidth(LayoutUnit w)
    {
        m_minimumWidth = std::max(m_minimumWidth, w);
    }

    LayoutUnit lastKnownWidth()
    {
        return m_lastKnownWidth;
    }

    static LayoutUnit computeMinimumWidthDueToMBP(ComputedStyle* style)
    {
        LayoutUnit minWidth;
        if (style->borderLeftWidth().isFixed())
            minWidth += style->borderLeftWidth().fixed();
        if (style->borderRightWidth().isFixed())
            minWidth += style->borderRightWidth().fixed();
        if (style->paddingLeft().isFixed())
            minWidth += style->paddingLeft().fixed();
        if (style->paddingRight().isFixed())
            minWidth += style->paddingRight().fixed();
        if (style->marginLeft().isFixed())
            minWidth += style->marginLeft().fixed();
        if (style->marginRight().isFixed())
            minWidth += style->marginRight().fixed();
        return minWidth;
    }

    void setIsWhiteSpaceAtLast(bool isWhiteSpaceAtLast)
    {
        m_isWhiteSpaceAtLast = isWhiteSpaceAtLast;
    }

    bool isWhiteSpaceAtLast()
    {
        return m_isWhiteSpaceAtLast;
    }

private:
    LayoutContext& m_layoutContext;
    LayoutUnit m_result;
    LayoutUnit m_lastKnownWidth;
    LayoutUnit m_minimumWidth;
    bool m_isWhiteSpaceAtLast;
};



class PaintingContext {
public:
    PaintingContext(Canvas* canvas)
        : m_canvas(canvas)
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
        : m_node(node)
        , m_styleWhenNodeIsNull(s)
    {
        m_firstChild = m_lastChild = m_next = m_previous = m_parent = nullptr;
        m_flags.m_needsLayout = true;

        bool isRootElement = node && node->isElement() && node->asElement()->isHTMLElement() && node->asElement()->asHTMLElement()->isHTMLHtmlElement();
        m_flags.m_isRootElement = isRootElement;

        m_flags.m_isInFrameInlineScope = true;

        m_flags.m_isLeftMBPCleared = false;
        m_flags.m_isRightMBPCleared = false;

        computeStyleFlags();
    }

    bool isOverflowPropagatedToViewPort()
    {
        if (m_node && m_node->isElement() && m_node->asElement()->isHTMLElement() && m_node->asElement()->asHTMLElement()->isHTMLHtmlElement()) {
            HTMLBodyElement* bodyElement = m_node->document()->bodyElement();
            if (bodyElement)
                return bodyElement->style()->overflow() != style()->overflow();
            return style()->overflow() != OverflowValue::VisibleOverflow;
        }

        if (m_node && m_node->isElement() && m_node->asElement()->isHTMLElement() && m_node->asElement()->asHTMLElement()->isHTMLBodyElement() && m_node->document()->rootElement()) {
            HTMLHtmlElement* rootElement = m_node->document()->rootElement();
            return rootElement->style()->overflow() != style()->overflow();
        }

        return false;
    }

    bool shouldApplyOverflow()
    {
        return isOverflowPropagatedToViewPort() ? false : style()->overflow() != OverflowValue::VisibleOverflow;
    }

    void computeStyleFlags()
    {
        bool isRootElement = m_flags.m_isRootElement;

        // TODO add condition
        m_flags.m_isEstablishesBlockFormattingContext = isRootElement;
        ComputedStyle* style = Frame::style();
        if (style) {
            // https://www.w3.org/TR/CSS21/visuren.html#block-formatting
            // Block formatting context is established when the element is either float, absolute positioned, or block boxes with 'overflow' other than 'visible'.
            // Especially, the last condition should be met another requirement, which is, the overflow should not be propagated to viewport.
            // There are 2 possible cases that overflow property can propagate to viewport, in other words, containing block is viewport.
            // 1. By giving a position of absoulte value, which is already included as one of forming block formatting context conditions.
            // 2. <html> and <body> element, so we should check first overflow values of <head> and <body> are equal.
            m_flags.m_isEstablishesBlockFormattingContext = m_flags.m_isEstablishesBlockFormattingContext || (shouldApplyOverflow());
            m_flags.m_isEstablishesBlockFormattingContext = m_flags.m_isEstablishesBlockFormattingContext || (style->originalDisplay() == DisplayValue::InlineBlockDisplayValue);
            m_flags.m_isEstablishesBlockFormattingContext = m_flags.m_isEstablishesBlockFormattingContext || (style->position() == PositionValue::AbsolutePositionValue);
        }

        m_flags.m_isPositionedElement = style && style->position() != PositionValue::StaticPositionValue;

        // TODO add condition
        m_flags.m_isEstablishesStackingContext = isRootElement;
        m_flags.m_needsGraphicsBuffer = false;
        if (style) {
            // NOTE
            // https://www.w3.org/TR/CSS2/zindex.html
            // Appendix E. Elaborate description of Stacking Contexts
            // All positioned descendants with 'z-index: auto' or 'z-index: 0', in tree order. For those with 'z-index: auto', treat the element as if it created a new stacking context,
            m_flags.m_isEstablishesStackingContext = m_flags.m_isEstablishesStackingContext || m_flags.m_isPositionedElement;
            m_flags.m_isEstablishesStackingContext = m_flags.m_isEstablishesStackingContext || (style->opacity() != 1);
            m_flags.m_isEstablishesStackingContext = m_flags.m_isEstablishesStackingContext || (style->hasTransforms(this));

            // FIXME
            // this is not necessery but, we need to compute clip rect when composite used
            m_flags.m_isEstablishesStackingContext = m_flags.m_isEstablishesStackingContext || (shouldApplyOverflow());

            m_flags.m_needsGraphicsBuffer = style->opacity() != 1 || style->hasTransforms(this);
        }

        if (style && style->width().isAuto()) {
            if (style->display() == InlineBlockDisplayValue) {
                m_flags.m_shouldComputePreferredWidth = true;
            } else if (style->position() == AbsolutePositionValue) {
                m_flags.m_shouldComputePreferredWidth = true;
            } else {
                m_flags.m_shouldComputePreferredWidth = false;
            }
        } else {
            m_flags.m_shouldComputePreferredWidth = false;
        }

        if (style && style->position() == PositionValue::AbsolutePositionValue) {
            m_flags.m_isNormalFlow = false;
        } else {
            m_flags.m_isNormalFlow = true;
        }
    }

    virtual ~Frame()
    {
    }

    virtual bool isFrameBox()
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

    FrameInline* asFrameInline()
    {
        STARFISH_ASSERT(isFrameInline());
        return (FrameInline*)this;
    }

    virtual ComputedStyle* style()
    {
        if (LIKELY(node() != nullptr))
            return node()->style();
        else
            return m_styleWhenNodeIsNull;
    }

    Node* node()
    {
        return m_node;
    }

    void setParent(Frame* f)
    {
        m_layoutParent = m_parent = f;
    }

    void setLayoutParent(Frame* f)
    {
        m_layoutParent = f;
    }

    Frame* parent()
    {
        return m_parent;
    }

    Frame* layoutParent()
    {
        return m_layoutParent;
    }

    Frame* next()
    {
        return m_next;
    }

    Frame* previous()
    {
        return m_previous;
    }

    Frame* firstChild()
    {
        return m_firstChild;
    }

    Frame* lastChild()
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

    void removeChild(Frame* oldChild)
    {
        STARFISH_ASSERT(oldChild);
        STARFISH_ASSERT(oldChild->parent() == this);

        if (oldChild->m_previous)
            oldChild->m_previous->m_next = oldChild->next();
        if (oldChild->m_next)
            oldChild->m_next->m_previous = oldChild->previous();

        if (m_firstChild == oldChild)
            m_firstChild = oldChild->next();
        if (m_lastChild == oldChild)
            m_lastChild = oldChild->previous();

        oldChild->m_previous = nullptr;
        oldChild->m_next = nullptr;
        oldChild->setParent(nullptr);
    }
#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth)
    {
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

    virtual void computePreferredWidth(ComputePreferredWidthContext& ctx)
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
            if (f == this)
                return true;
            f = f->layoutParent();
        }
        return false;
    }

    bool isEstablishesBlockFormattingContext()
    {
        return m_flags.m_isEstablishesBlockFormattingContext;
    }

    bool isEstablishesStackingContext()
    {
        return m_flags.m_isEstablishesStackingContext;
    }

    bool isPositionedElement()
    {
        return m_flags.m_isPositionedElement;
    }

    bool isNormalFlow()
    {
        return m_flags.m_isNormalFlow;
    }

    bool isRootElement()
    {
        return m_flags.m_isRootElement;
    }

    bool shouldComputePreferredWidth()
    {
        return m_flags.m_shouldComputePreferredWidth;
    }

    bool isLeftMBPCleared()
    {
        return m_flags.m_isLeftMBPCleared;
    }

    bool isRightMBPCleared()
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

    void setInFrameInlineScope()
    {
        m_flags.m_isInFrameInlineScope = true;
    }

    bool InFrameInlineScope()
    {
        return m_flags.m_isInFrameInlineScope;
    }

    bool needsGraphicsBuffer()
    {
        return m_flags.m_needsGraphicsBuffer;
    }

protected:
    struct {
        bool m_needsLayout : 1;

        // https://www.w3.org/TR/CSS21/visuren.html#block-formatting
        // Floats, absolutely positioned elements, block containers (such as inline-blocks, table-cells, and table-captions) that are not block boxes, and block boxes with 'overflow' other than 'visible' (except when that value has been propagated to the viewport) establish new block formatting contexts for their contents.
        bool m_isEstablishesBlockFormattingContext : 1;
        bool m_needsGraphicsBuffer : 1;

        // https://www.w3.org/TR/CSS21/visuren.html#propdef-z-index
        // Other stacking contexts are generated by any positioned element (including relatively positioned elements) having a computed value of 'z-index' other than 'auto'. Stacking contexts are not necessarily related to containing blocks. In future levels of CSS, other properties may introduce stacking contexts, for example 'opacity' [CSS3COLOR].
        bool m_isEstablishesStackingContext : 1;

        // https://www.w3.org/TR/CSS21/visuren.html#positioning-scheme
        // 9.3.2
        // An element is said to be positioned if its 'position' property has a value other than 'static'. Positioned elements generate positioned boxes, laid out according to four properties:
        bool m_isPositionedElement : 1;

        bool m_shouldComputePreferredWidth : 1;
        bool m_isNormalFlow : 1;
        bool m_isRootElement : 1;
        bool m_isInFrameInlineScope: 1;

        bool m_isLeftMBPCleared: 1;
        bool m_isRightMBPCleared: 1;
    } m_flags;

private:
    Node* m_node;
    // TODO implement FrameRareData
    ComputedStyle* m_styleWhenNodeIsNull;

    Frame* m_parent;
    Frame* m_layoutParent;

    Frame* m_previous;
    Frame* m_next;

    Frame* m_firstChild;
    Frame* m_lastChild;
};
}

#endif
