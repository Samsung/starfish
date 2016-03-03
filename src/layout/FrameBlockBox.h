#ifndef __StarFishBlockBox__
#define __StarFishBlockBox__

#include "layout/FrameBox.h"
#include "layout/FrameReplaced.h"

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

class InlineTextBox : public InlineBox {
public:
    InlineTextBox(Node* node, ComputedStyle* style, Frame* parent, String* str, FrameText* origin)
        : InlineBox(node, style, parent)
    {
        m_text = str;
        m_origin = origin;
    }

    virtual bool isInlineTextBox() const { return true; }

    virtual void paint(Canvas* canvas, PaintingStage stage);
    virtual void dump(int depth)
    {
        InlineBox::dump(depth);
        printf(" [%s] ", m_text->utf8Data());
    }

    virtual const char* name()
    {
        return "InlineTextBox";
    }

    String* text()
    {
        return m_text;
    }

protected:
    String* m_text;
    FrameText* m_origin;
};

class InlineNonReplacedBox : public InlineBox {
    friend FrameBlockBox;

public:
    InlineNonReplacedBox(Node* node, ComputedStyle* style, Frame* parent, FrameInline* origin)
        : InlineBox(node, style, parent)
    {
        m_origin = origin;
        m_descender = m_ascender = 0;
    }

    virtual bool isInlineNonReplacedBox() const { return true; }
    virtual const char* name()
    {
        return "InlineNonReplacedBox";
    }
    static InlineNonReplacedBox* layoutInline(InlineNonReplacedBox* self, LayoutContext& ctx, FrameBlockBox* blockBox,
        LineFormattingContext* lineFormattingContext, InlineNonReplacedBox* layoutParentBox, bool freshStart);
    virtual void paint(Canvas* canvas, PaintingStage stage);
    virtual void paintChildrenWith(Canvas* canvas, PaintingStage stage)
    {
        auto iter = boxes().begin();
        while (iter != boxes().end()) {
            FrameBox* child = *iter;
            canvas->save();
            canvas->translate(child->asFrameBox()->x(), child->asFrameBox()->y());
            child->paint(canvas, stage);
            canvas->restore();
            iter++;
        }
    }
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage);
    virtual void dump(int depth);

    virtual void paintBackgroundAndBorders(Canvas* canvas);

    LayoutUnit ascender()
    {
        return m_ascender;
    }

    LayoutUnit decender()
    {
        return m_descender;
    }

    std::vector<FrameBox*, gc_allocator<FrameBox*> >& boxes()
    {
        return m_boxes;
    }

protected:
    LayoutUnit m_ascender;
    LayoutUnit m_descender;
    FrameInline* m_origin;
    LayoutBoxSurroundData m_orgPadding, m_orgBorder, m_orgMargin;
    std::vector<FrameBox*, gc_allocator<FrameBox*> > m_boxes;
};

class LineBox : public FrameBox {
    friend class LineFormattingContext;
    friend class FrameBlockBox;
    friend class InlineNonReplacedBox;

public:
    LineBox(Frame* parent)
        : FrameBox(nullptr, nullptr)
    {
        setParent(parent);
        m_descender = m_ascender = 0;
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

    std::vector<FrameBox*, gc_allocator<FrameBox*> >& boxes()
    {
        return m_boxes;
    }

protected:
    std::vector<FrameBox*, gc_allocator<FrameBox*> > m_boxes;
    // FIXME
    // we use these value only for vertical-align of inline-block
    // in layout, we use only 'ascender'
    // should we delete m_decender?
    LayoutUnit m_ascender;
    LayoutUnit m_descender;
};

class MarginInfo {
public:
    MarginInfo(LayoutUnit topBorderPadding, LayoutUnit bottomBorderPadding, bool isNewContext, Length height)
    {
        m_canCollapseWithChildren = !isNewContext && !topBorderPadding && !bottomBorderPadding;
        m_canCollapseTopWithChildren = m_canCollapseWithChildren && !topBorderPadding;
        m_canCollapseBottomWithChildren = m_canCollapseWithChildren && !bottomBorderPadding && height.isAuto();
        m_atTopSideOfBlock = true;
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
    void setAtBottomSideOfBlock(bool b) { m_atBottomSideOfBlock = b; }
    bool atTopSideOfBlock() { return m_atTopSideOfBlock; }
    bool canCollapseWithMarginTop()
    {
        return m_atTopSideOfBlock && m_canCollapseTopWithChildren;
    }
    bool canCollapseWithMarginBottom()
    {
        return m_atBottomSideOfBlock && m_canCollapseBottomWithChildren;
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
    bool m_atBottomSideOfBlock;
    LayoutUnit m_positiveMargin;
    LayoutUnit m_negativeMargin;
};

class FrameBlockBox : public FrameBox {
    friend class LineFormattingContext;
    friend class InlineNonReplacedBox;
public:
    FrameBlockBox(Node* node, ComputedStyle* style)
        : FrameBox(node, style)
    {
        STARFISH_ASSERT((node == nullptr && style != nullptr) || (node != nullptr && style == nullptr))
    }

    virtual bool isFrameBlockBox()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameBlockBox";
    }

    virtual void layout(LayoutContext& ctx);
    virtual void computePreferredWidth(ComputePreferredWidthContext& ctx);

    virtual void dump(int depth);
    virtual void paint(Canvas* canvas, PaintingStage stage);
    virtual void paintChildrenWith(Canvas* canvas, PaintingStage stage);
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage);
    virtual Frame* hitTestChildrenWith(LayoutUnit x, LayoutUnit y, HitTestStage stage);

    bool hasBlockFlow()
    {
        if (!firstChild())
            return true;

        Frame* child = firstChild();

        while (child && !child->isNormalFlow()) {
            child = child->next();
        }

        if (!child)
            return true;

        return child->style()->display() == BlockDisplayValue;
    }

    bool isSelfCollapsingBlock()
    {
        if (isEstablishesBlockFormattingContext())
            return false;
        if (asFrameBox()->height() > 0)
            return false;
        Length heightLength = style()->height();
        // FIXME(june0cho): consider height auto
        if (heightLength.isZero()) {
            // If the block is inline flow and has any line boxes,
            // this is not self-collapsing.
            if (!hasBlockFlow())
                return !m_lineBoxes.size();
            Frame* child = firstChild();
            while (child) {
                if (!child->isNormalFlow())
                    continue;
                if (!child->asFrameBox()->isSelfCollapsingBlock())
                    return false;
                child = child->next();
            }
            return true;
        }
        return true;
    }

protected:
    LayoutUnit layoutBlock(LayoutContext& ctx);
    LayoutUnit layoutInline(LayoutContext& ctx);
    std::vector<LineBox*, gc_allocator<LineBox*> > m_lineBoxes;
};

class LineFormattingContext {
public:
    LineFormattingContext(FrameBlockBox& block, LayoutContext& ctx, const LayoutUnit& lineBoxWidth)
        : m_block(block)
        , m_layoutContext(ctx)
    {
        m_lineBoxWidth = lineBoxWidth;
        m_block.m_lineBoxes.clear();
        // m_block.m_lineBoxes.shrink_to_fit();
        LineBox* lineBox = new LineBox(&m_block);
        lineBox->setWidth(m_lineBoxWidth);
        m_block.m_lineBoxes.push_back(lineBox);
        m_currentLine = 0;
        m_currentLineWidth = 0;
    }

    void breakLine(bool dueToBr = false);
    void completeLastLine();

    bool isBreakedLineWithoutBR(size_t idx)
    {
        return m_breakedLinesSet.find(idx) != m_breakedLinesSet.end();
    }

    void registerInlineContent()
    {
        m_layoutContext.setLastLineBox(m_block.m_lineBoxes.back());
    }

    LineBox* currentLine()
    {
        return m_block.m_lineBoxes.back();
    }

    void registerInlineBlockAscender(LayoutUnit ascender, FrameBlockBox* box)
    {
        m_inlineBlockAscender[box] = ascender;
    }

    LayoutUnit inlineBlockAscender(FrameBlockBox* box)
    {
        STARFISH_ASSERT(m_inlineBlockAscender.find(box) != m_inlineBlockAscender.end());
        return m_inlineBlockAscender[box];
    }

    LayoutUnit m_currentLineWidth;
    LayoutUnit m_lineBoxWidth;
    size_t m_currentLine;
    FrameBlockBox& m_block;
    LayoutContext& m_layoutContext;

    std::set<size_t> m_breakedLinesSet;
    std::unordered_map<FrameBlockBox*, LayoutUnit> m_inlineBlockAscender;
};
}

#endif
