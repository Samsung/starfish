#ifndef __StarFishFrame__
#define __StarFishFrame__

#include "dom/DOM.h"

#include "style/Unit.h"
#include "style/ComputedStyle.h"

#include "layout/LayoutUtil.h"
#include "layout/StackingContext.h"

namespace StarFish
{

class Node;
class FrameText;
class FrameBox;
class FrameBlockBox;
class FrameReplaced;
class FrameInline;

enum PaintingStage
{
    PaintingStackingContext,
    PaintingNormalFlowBlock, // the in-flow, non-inline-level, non-positioned descendants.
    PaintingNonPositionedFloats, // the non-positioned floats.
    PaintingNormalFlowInline, // the in-flow, inline-level, non-positioned descendants, including inline tables and inline blocks.
    PaintingPositionedElements, // the child stacking contexts with stack level 0 and the positioned descendants with stack level 0.
    PaintingStageEnd
};

enum HitTestStage
{
    HitTestPositionedElements,
    HitTestNormalFlowInline,
    HitTestNonPositionedFloats,
    HitTestNormalFlowBlock,
    HitTestStackingContext,
    HitTestStageEnd,
};

class LineBox;

class LayoutContext
{
public:
    LayoutContext(Frame* rootFrame)
    {
        m_rootFrame = rootFrame;
        m_lastLineBox = nullptr;
    }

    ~LayoutContext()
    {
        STARFISH_ASSERT(m_absolutePositionedFrames.size() == 0);
        STARFISH_ASSERT(m_relativePositionedFrames.size() == 0);
    }

    LayoutUnit parentContentWidth(Frame* currentFrame);
    bool parentHasFixedHeight(Frame* currentFrame);
    LayoutUnit parentFixedHeight(Frame* currentFrame);
    Frame* blockContainer(Frame* currentFrame);
    Frame* containingBlock(Frame* currentFrame);

    void setLastLineBox(LineBox* l)
    {
        m_lastLineBox = l;
    }

    LineBox* lastLineBox()
    {
        return m_lastLineBox;
    }

    void registerAbsolutePositionedFrames(Frame* frm)
    {
        Frame* cb = containingBlock(frm);
        m_absolutePositionedFrames.insert(std::make_pair(cb, std::vector<Frame*>()));
        std::vector<Frame*>& vec = m_absolutePositionedFrames[cb];
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

    void registerRelativePositionedFrames(Frame* frm)
    {
        Frame* cb = containingBlock(frm);
        m_relativePositionedFrames.insert(std::make_pair(cb, std::vector<Frame*>()));
        std::vector<Frame*>& vec = m_relativePositionedFrames[cb];
        vec.push_back(frm);
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

private:
    Frame* m_rootFrame;
    LineBox* m_lastLineBox;
    // NOTE. we dont need gc_allocator here. because, FrameTree already has referenece for Frames
    std::map<Frame*, std::vector<Frame*>> m_absolutePositionedFrames;
    std::map<Frame*, std::vector<Frame*>> m_relativePositionedFrames;
};

class ComputePreferredWidthContext
{
public:
    ComputePreferredWidthContext(LayoutContext& lc, LayoutUnit lastKnownWidth)
        : m_layoutContext(lc)
    {
        m_result = 0;
        m_lastKnownWidth = lastKnownWidth;
    }

    LayoutContext& layoutContext()
    {
        return m_layoutContext;
    }

    void setResult(LayoutUnit r)
    {
        m_result = std::min(std::max(m_result, r), m_lastKnownWidth);
    }

    LayoutUnit result()
    {
        return m_result;
    }

    LayoutUnit lastKnownWidth()
    {
        return m_lastKnownWidth;
    }
private:
    LayoutContext& m_layoutContext;
    LayoutUnit m_result;
    LayoutUnit m_lastKnownWidth;
};

class Frame: public gc
{
    friend class LayoutContext;
public:
    Frame(Node* node, ComputedStyle* style) :
            m_node(node), m_style(style)
    {
        m_firstChild = m_lastChild = m_next = m_previous = m_parent = nullptr;
        m_flags.m_needsLayout = true;

        bool isRootElement = node && node->isElement() && node->asElement()->isHTMLElement() && node->asElement()->asHTMLElement()->isHTMLHtmlElement();

        // TODO add condition
        m_flags.m_isEstablishesBlockFormattingContext = isRootElement;
        if (m_style) {
            m_flags.m_isEstablishesBlockFormattingContext = m_flags.m_isEstablishesBlockFormattingContext || (m_style->overflow() != OverflowValue::VisibleOverflow);
        }

        m_flags.m_isPositionedElement = m_style && m_style->position() != PositionValue::StaticPositionValue;

        // TODO add condition
        m_flags.m_isEstablishesStackingContext = isRootElement;
        if (m_style) {
            m_flags.m_isEstablishesStackingContext = m_flags.m_isEstablishesStackingContext || (m_flags.m_isPositionedElement && m_style->zIndex() != 0);
        }

        if (m_style && m_style->width().isAuto()) {
           if (m_style->display() == InlineBlockDisplayValue) {
               m_flags.m_shouldComputePreferredWidth = true;
           } else if (m_style->position() == AbsolutePositionValue) {
               m_flags.m_shouldComputePreferredWidth = true;
           } else {
               m_flags.m_shouldComputePreferredWidth = false;
           }
        } else {
            m_flags.m_shouldComputePreferredWidth = false;
        }

        if (m_style && m_style->position() == PositionValue::AbsolutePositionValue) {
            m_flags.m_isNormalFlow = false;
        } else {
            m_flags.m_isNormalFlow = true;
        }

        m_stackingContext = nullptr;
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
        return (FrameText*) this;
    }

    FrameBox* asFrameBox()
    {
        STARFISH_ASSERT(isFrameBox());
        return (FrameBox*) this;
    }

    FrameReplaced* asFrameReplaced()
    {
        STARFISH_ASSERT(isFrameReplaced());
        return (FrameReplaced*) this;
    }

    FrameBlockBox* asFrameBlockBox()
    {
        STARFISH_ASSERT(isFrameBlockBox());
        return (FrameBlockBox*) this;
    }

    FrameInline* asFrameInline()
    {
        STARFISH_ASSERT(isFrameInline());
        return (FrameInline*) this;
    }

    ComputedStyle* style()
    {
        return m_style;
    }

    void setStyle(ComputedStyle* s)
    {
        m_style = s;
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

    virtual void dump(int depth)
    {

    }

    virtual const char* name()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void layout(LayoutContext& ctx)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void computePreferredWidth(ComputePreferredWidthContext& ctx)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void paint(Canvas* canvas, PaintingStage = PaintingStackingContext)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage = HitTestStackingContext)
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

    bool shouldComputePreferredWidth()
    {
        return m_flags.m_shouldComputePreferredWidth;
    }

protected:
    struct
    {
        bool m_needsLayout :1;

        // https://www.w3.org/TR/CSS21/visuren.html#block-formatting
        // Floats, absolutely positioned elements, block containers (such as inline-blocks, table-cells, and table-captions) that are not block boxes, and block boxes with 'overflow' other than 'visible' (except when that value has been propagated to the viewport) establish new block formatting contexts for their contents.
        bool m_isEstablishesBlockFormattingContext :1;

        // https://www.w3.org/TR/CSS21/visuren.html#propdef-z-index
        // Other stacking contexts are generated by any positioned element (including relatively positioned elements) having a computed value of 'z-index' other than 'auto'. Stacking contexts are not necessarily related to containing blocks. In future levels of CSS, other properties may introduce stacking contexts, for example 'opacity' [CSS3COLOR].
        bool m_isEstablishesStackingContext :1;

        // https://www.w3.org/TR/CSS21/visuren.html#positioning-scheme
        // 9.3.2
        // An element is said to be positioned if its 'position' property has a value other than 'static'. Positioned elements generate positioned boxes, laid out according to four properties:
        bool m_isPositionedElement :1;

        bool m_shouldComputePreferredWidth :1;
        bool m_isNormalFlow :1;
    } m_flags;

    Node* m_node;
    ComputedStyle* m_style;
    StackingContext* m_stackingContext;
private:
    Frame* m_parent;
    Frame* m_layoutParent;

    Frame* m_previous;
    Frame* m_next;

    Frame* m_firstChild;
    Frame* m_lastChild;
};

}

#endif
